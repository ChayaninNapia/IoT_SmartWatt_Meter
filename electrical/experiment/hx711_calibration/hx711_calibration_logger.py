
"""Interactive HX711 serial capture logger.

Run this script once, then type commands such as "loading m0 t1" or
"unloading m3 trial02".
Each command asks the ESP32 to collect exactly 100 valid HX711 samples and saves
one CSV into the matching data folder.
"""

from __future__ import annotations

import csv
import queue
import re
import sys
import threading
import time
from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Any

# ===== User config =====
SERIAL_PORT = "COM3"
BAUD_RATE = 115200
PROJECT_ROOT = Path(__file__).resolve().parents[3]
DATA_ROOT = PROJECT_ROOT / "data" / "hx711_calibration"
DEFAULT_OUTPUT_FILENAME = "hx711_capture.csv"
VALID_GROUPS = {"loading", "unloading"}
MIN_MASS_KG = 0
MAX_MASS_KG = 4
SAMPLES_PER_CAPTURE = 100
TRIAL_TIMEOUT_S = 30.0
SERIAL_STARTUP_DELAY_S = 2.0
MONITOR_WINDOW_SAMPLES = 200
MONITOR_REDRAW_INTERVAL_S = 0.2

INSTALL_COMMAND = "pip install pyserial matplotlib"
HEADER_LINE = "sample_idx,time_ms,raw,delta_raw"
BEGIN_RE = re.compile(r"^BEGIN_TRIAL,zero_raw=(?P<zero_raw>-?\d+),samples=(?P<samples>\d+)$")
LIVE_RE = re.compile(r"^LIVE,(?P<time_ms>\d+),(?P<raw>-?\d+),(?P<delta_raw>-?\d+)$")
CAPTURE_RE = re.compile(
    r"^(?P<group>loading|unloading|l|u)\s+m(?P<mass>\d+)\s+(?:t|trial)(?P<trial>\d+)(?:\s+(?P<filename>\S+))?$",
    re.IGNORECASE,
)

try:
    import serial
    from serial import SerialException
except ImportError:
    serial = None
    SerialException = Exception


@dataclass
class CaptureBlock:
    zero_raw: int
    declared_samples: int
    samples: list[dict[str, int]]


@dataclass
class CaptureCommand:
    group: str
    mass: int
    trial: int
    output_csv: Path


@dataclass
class MonitorState:
    active: bool = False
    plt: Any = None
    fig: Any = None
    ax_raw: Any = None
    ax_delta: Any = None
    raw_line: Any = None
    delta_line: Any = None
    time_ms: deque[int] | None = None
    raw_values: deque[int] | None = None
    delta_values: deque[int] | None = None
    last_redraw_s: float = 0.0


def decode_serial_line(raw_line: bytes) -> str:
    """Decode one serial line while tolerating odd bytes."""
    return raw_line.decode("utf-8", errors="replace").strip()


def parse_begin_line(line: str) -> tuple[int, int]:
    """Parse BEGIN_TRIAL metadata from the ESP32."""
    match = BEGIN_RE.match(line)
    if not match:
        raise ValueError(f"Invalid BEGIN_TRIAL line: {line}")
    return int(match.group("zero_raw")), int(match.group("samples"))


def parse_live_line(line: str) -> tuple[int, int, int] | None:
    """Parse one LIVE,time_ms,raw,delta_raw line."""
    match = LIVE_RE.match(line)
    if not match:
        return None
    return (
        int(match.group("time_ms")),
        int(match.group("raw")),
        int(match.group("delta_raw")),
    )


def read_capture_block(port: "serial.Serial") -> CaptureBlock:
    """Read one BEGIN_TRIAL / END_TRIAL block from serial."""
    deadline = time.monotonic() + TRIAL_TIMEOUT_S

    begin_line = ""
    while time.monotonic() < deadline:
        line = decode_serial_line(port.readline())
        if not line:
            continue
        if line.startswith("BEGIN_TRIAL"):
            begin_line = line
            break

    if not begin_line:
        raise TimeoutError("Timed out waiting for BEGIN_TRIAL.")

    zero_raw, declared_samples = parse_begin_line(begin_line)

    while time.monotonic() < deadline:
        line = decode_serial_line(port.readline())
        if not line:
            continue
        if line == HEADER_LINE:
            break
    else:
        raise TimeoutError("Timed out waiting for CSV header.")

    samples: list[dict[str, int]] = []
    while time.monotonic() < deadline:
        line = decode_serial_line(port.readline())
        if not line:
            continue
        if line == "END_TRIAL":
            return CaptureBlock(zero_raw, declared_samples, samples)

        try:
            sample_idx_text, time_ms_text, raw_text, delta_text = next(csv.reader([line]))
            samples.append(
                {
                    "sample_idx": int(sample_idx_text),
                    "time_ms": int(time_ms_text),
                    "raw": int(raw_text),
                    "delta_raw": int(delta_text),
                }
            )
        except Exception:
            print(f"WARNING: ignored malformed sample line: {line}")

    raise TimeoutError("Timed out waiting for END_TRIAL.")


def save_capture(block: CaptureBlock, output_csv: Path) -> None:
    """Overwrite the configured CSV file with one capture."""
    output_csv.parent.mkdir(parents=True, exist_ok=True)

    with output_csv.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(
            csv_file,
            fieldnames=["sample_idx", "time_ms", "raw", "delta_raw", "zero_raw"],
        )
        writer.writeheader()
        for sample in block.samples:
            writer.writerow(
                {
                    "sample_idx": sample["sample_idx"],
                    "time_ms": sample["time_ms"],
                    "raw": sample["raw"],
                    "delta_raw": sample["delta_raw"],
                    "zero_raw": block.zero_raw,
                }
            )


def normalize_group(group_text: str) -> str:
    if group_text.lower() == "l":
        return "loading"
    if group_text.lower() == "u":
        return "unloading"
    return group_text.lower()


def parse_capture_command(command_text: str) -> CaptureCommand | None:
    """Parse commands like 'loading m0 t1' or 'unloading m3 trial02'."""
    match = CAPTURE_RE.match(command_text.strip())
    if not match:
        return None

    group = normalize_group(match.group("group"))
    mass = int(match.group("mass"))
    trial = int(match.group("trial"))
    if group not in VALID_GROUPS:
        return None
    if not (MIN_MASS_KG <= mass <= MAX_MASS_KG):
        print(f"Mass must be m{MIN_MASS_KG} to m{MAX_MASS_KG}.")
        return None

    filename = match.group("filename") or DEFAULT_OUTPUT_FILENAME
    if not filename.lower().endswith(".csv"):
        filename = f"{filename}.csv"

    output_dir = DATA_ROOT / group / f"m{mass}" / f"trial{trial:02d}"
    return CaptureCommand(group=group, mass=mass, trial=trial, output_csv=output_dir / filename)


def open_serial_port() -> "serial.Serial":
    if serial is None:
        print("pyserial is not installed.", file=sys.stderr)
        print(f"Install it with: {INSTALL_COMMAND}", file=sys.stderr)
        raise SystemExit(1)

    try:
        port = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05, write_timeout=5.0)
    except SerialException as exc:
        print(f"Could not open {SERIAL_PORT}: {exc}", file=sys.stderr)
        print("Check the COM port and close Arduino Serial Monitor.", file=sys.stderr)
        raise SystemExit(1)

    print(f"Opened {SERIAL_PORT} at {BAUD_RATE} baud.")
    print(f"Waiting {SERIAL_STARTUP_DELAY_S:g} s for ESP32 startup...")
    time.sleep(SERIAL_STARTUP_DELAY_S)
    port.reset_input_buffer()
    return port


def maybe_tare(port: "serial.Serial") -> None:
    answer = input("Remove all load. Press Enter to tare, or type 'skip': ").strip().lower()
    if answer == "skip":
        print("Skipping tare. ESP32 must already have a valid zero_raw.")
        return

    port.write(b"t\n")
    port.flush()
    print("Sent tare command.")
    time.sleep(1.0)

    while port.in_waiting:
        line = decode_serial_line(port.readline())
        if line:
            print(f"ESP32: {line}")


def print_help() -> None:
    print()
    print("Commands:")
    print("  loading m0 t1          save to data/hx711_calibration/loading/m0/trial01/hx711_capture.csv")
    print("  unloading m3 trial02   save to data/hx711_calibration/unloading/m3/trial02/hx711_capture.csv")
    print("  l m4 t3 data.csv       loading shortcut with a custom CSV filename")
    print("  u m2 t1                unloading shortcut")
    print(f"  Mass range: m{MIN_MASS_KG} to m{MAX_MASS_KG}")
    print("  monitor / mon          open live raw/delta_raw graph")
    print("  status / s             show current ESP32 tare status")
    print("  tare                   send tare command to ESP32")
    print("  help                   show this help")
    print("  q                      quit")
    print()


def start_input_thread(command_queue: "queue.Queue[str]", stop_event: threading.Event) -> threading.Thread:
    """Read terminal commands without blocking matplotlib updates."""
    def worker() -> None:
        while not stop_event.is_set():
            try:
                command_queue.put(input("hx711> "))
            except EOFError:
                command_queue.put("q")
                return

    thread = threading.Thread(target=worker, daemon=True)
    thread.start()
    return thread


def send_tare(port: "serial.Serial") -> None:
    port.reset_input_buffer()
    port.write(b"t\n")
    port.flush()
    print("Sent tare command.")
    time.sleep(1.0)

    while port.in_waiting:
        line = decode_serial_line(port.readline())
        if line:
            print(f"ESP32: {line}")


def request_status(port: "serial.Serial", monitor: MonitorState) -> None:
    stop_monitor(port, monitor)
    port.reset_input_buffer()
    port.write(b"s\n")
    port.flush()

    deadline = time.monotonic() + 2.0
    while time.monotonic() < deadline:
        line = decode_serial_line(port.readline())
        if not line:
            continue
        print(f"ESP32: {line}")
        if line.startswith("STATUS,"):
            return

    print("WARNING: Timed out waiting for STATUS response.")


def start_monitor(port: "serial.Serial", monitor: MonitorState) -> None:
    if monitor.active:
        print("Monitor is already running.")
        return

    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib is not installed.", file=sys.stderr)
        print(f"Install it with: {INSTALL_COMMAND}", file=sys.stderr)
        return

    monitor.time_ms = deque(maxlen=MONITOR_WINDOW_SAMPLES)
    monitor.raw_values = deque(maxlen=MONITOR_WINDOW_SAMPLES)
    monitor.delta_values = deque(maxlen=MONITOR_WINDOW_SAMPLES)

    plt.ion()
    fig, (ax_raw, ax_delta) = plt.subplots(2, 1, sharex=True, figsize=(9, 6))
    raw_line, = ax_raw.plot([], [], label="raw", color="tab:blue")
    delta_line, = ax_delta.plot([], [], label="delta_raw", color="tab:orange")

    ax_raw.set_ylabel("raw")
    ax_raw.grid(True, alpha=0.3)
    ax_raw.legend(loc="upper right")
    ax_delta.set_xlabel("elapsed time (s)")
    ax_delta.set_ylabel("delta_raw")
    ax_delta.grid(True, alpha=0.3)
    ax_delta.legend(loc="upper right")
    fig.suptitle("HX711 live monitor")
    fig.tight_layout()

    monitor.active = True
    monitor.plt = plt
    monitor.fig = fig
    monitor.ax_raw = ax_raw
    monitor.ax_delta = ax_delta
    monitor.raw_line = raw_line
    monitor.delta_line = delta_line
    monitor.last_redraw_s = 0.0

    port.reset_input_buffer()
    port.write(b"r1\n")
    port.flush()
    print("Live monitor started. Collect/tare commands will stop it automatically.")


def stop_monitor(port: "serial.Serial", monitor: MonitorState, close_window: bool = True) -> None:
    if not monitor.active:
        return

    port.write(b"r0\n")
    port.flush()
    time.sleep(0.3)
    port.reset_input_buffer()

    if close_window and monitor.plt is not None and monitor.fig is not None:
        monitor.plt.close(monitor.fig)

    monitor.active = False
    print("Live monitor stopped.")


def poll_monitor(port: "serial.Serial", monitor: MonitorState) -> None:
    if not monitor.active:
        return

    if monitor.plt is not None and monitor.fig is not None:
        if not monitor.plt.fignum_exists(monitor.fig.number):
            stop_monitor(port, monitor, close_window=False)
            return

    while port.in_waiting:
        line = decode_serial_line(port.readline())
        parsed = parse_live_line(line)
        if parsed is None:
            if line and not line.startswith("LIVE_REPORT="):
                print(f"ESP32: {line}")
            continue

        time_ms, raw, delta_raw = parsed
        monitor.time_ms.append(time_ms)
        monitor.raw_values.append(raw)
        monitor.delta_values.append(delta_raw)

    if not monitor.time_ms:
        if monitor.plt is not None:
            monitor.plt.pause(0.001)
        return

    now_s = time.monotonic()
    if now_s - monitor.last_redraw_s < MONITOR_REDRAW_INTERVAL_S:
        if monitor.plt is not None:
            monitor.plt.pause(0.001)
        return

    first_ms = monitor.time_ms[0]
    elapsed_s = [(item - first_ms) / 1000.0 for item in monitor.time_ms]

    monitor.raw_line.set_data(elapsed_s, list(monitor.raw_values))
    monitor.delta_line.set_data(elapsed_s, list(monitor.delta_values))
    monitor.ax_raw.relim()
    monitor.ax_raw.autoscale_view()
    monitor.ax_delta.relim()
    monitor.ax_delta.autoscale_view()
    monitor.fig.canvas.draw_idle()
    monitor.plt.pause(0.001)
    monitor.last_redraw_s = now_s


def collect_and_save(port: "serial.Serial", command: CaptureCommand, monitor: MonitorState) -> None:
    stop_monitor(port, monitor)

    print(f"Collecting {command.group} m{command.mass} trial{command.trial:02d}...")
    print(f"Output: {command.output_csv}")

    port.reset_input_buffer()
    port.write(b"c\n")
    port.flush()
    print("Sent command: c")

    block = read_capture_block(port)
    if block.declared_samples != SAMPLES_PER_CAPTURE:
        print(
            f"WARNING: ESP32 declared {block.declared_samples} samples, "
            f"but Python expects {SAMPLES_PER_CAPTURE}."
        )
    if len(block.samples) != SAMPLES_PER_CAPTURE:
        print(f"WARNING: received {len(block.samples)} samples, expected {SAMPLES_PER_CAPTURE}.")

    save_capture(block, command.output_csv)
    print(f"Saved CSV: {command.output_csv.resolve()}")


def main() -> None:
    print("Interactive HX711 capture logger")
    print(f"  SERIAL_PORT = {SERIAL_PORT}")
    print(f"  BAUD_RATE = {BAUD_RATE}")
    print(f"  DATA_ROOT = {DATA_ROOT}")
    print(f"  DEFAULT_OUTPUT_FILENAME = {DEFAULT_OUTPUT_FILENAME}")
    print(f"  SAMPLES_PER_CAPTURE = {SAMPLES_PER_CAPTURE}")

    with open_serial_port() as port:
        maybe_tare(port)
        print_help()

        command_queue: queue.Queue[str] = queue.Queue()
        stop_event = threading.Event()
        start_input_thread(command_queue, stop_event)
        monitor = MonitorState()

        while True:
            poll_monitor(port, monitor)

            try:
                command_text = command_queue.get(timeout=0.05).strip()
            except queue.Empty:
                continue

            if not command_text:
                continue

            command_lower = command_text.lower()
            if command_lower in {"q", "quit", "exit"}:
                stop_monitor(port, monitor)
                stop_event.set()
                print("Done.")
                return
            if command_lower in {"h", "help", "?"}:
                print_help()
                continue
            if command_lower in {"t", "tare"}:
                stop_monitor(port, monitor)
                send_tare(port)
                continue
            if command_lower in {"s", "status"}:
                request_status(port, monitor)
                continue
            if command_lower in {"monitor", "mon", "live"}:
                start_monitor(port, monitor)
                continue

            command = parse_capture_command(command_text)
            if command is None:
                print("Unknown command. Type 'help' for examples.")
                continue

            try:
                collect_and_save(port, command, monitor)
            except TimeoutError as exc:
                print(f"ERROR: {exc}", file=sys.stderr)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nCancelled by user.")
    except TimeoutError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
