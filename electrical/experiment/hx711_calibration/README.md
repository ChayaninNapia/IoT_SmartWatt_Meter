# Interactive HX711 Capture Logger

This folder contains a simple interactive HX711 data capture flow for the
bicycle watt meter calibration work. Run Python once, then type commands such as
`loading m0 t1` or `unloading m3 trial02`; each command collects 100 valid HX711
samples and saves one raw CSV file.

This version can also show a live raw/delta_raw graph while the ESP32 streams
HX711 readings.

## Files

```text
hx711_calibration_logger.py
esp32_hx711_trial_collector/esp32_hx711_trial_collector.ino
```

The Arduino `.ino` file is inside a folder with the same name so Arduino IDE and
`arduino-cli` can compile it normally.

## ESP32 Firmware

Upload this sketch:

```text
esp32_hx711_trial_collector/esp32_hx711_trial_collector.ino
```

Hardware/config:

```text
HX711 DOUT = GPIO32
HX711 SCK  = GPIO33
HX711 gain = 64
Baud rate  = 115200
Samples    = 100
```

Serial commands:

```text
h   show help
s   show current tare/status
t   tare with no load
c   collect 100 valid ready samples
r1  start live raw report
r0  stop live raw report
```

The ESP32 output block is:

```text
BEGIN_TRIAL,zero_raw=-47264,samples=100
sample_idx,time_ms,raw,delta_raw
1,12345,-60401,-13137
2,12444,-60418,-13154
...
100,22210,-60690,-13426
END_TRIAL
```

The Python script ignores extra startup text before `BEGIN_TRIAL`.

Live monitor lines use:

```text
LIVE,time_ms,raw,delta_raw
```

## Python Setup

Install the only required Python dependency:

```powershell
pip install pyserial matplotlib
```

Edit these constants at the top of `hx711_calibration_logger.py` if needed:

```python
SERIAL_PORT = "COM3"
BAUD_RATE = 115200
PROJECT_ROOT = Path(__file__).resolve().parents[3]
DATA_ROOT = PROJECT_ROOT / "data" / "hx711_calibration"
DEFAULT_OUTPUT_FILENAME = "hx711_capture.csv"
VALID_GROUPS = {"loading", "unloading"}
MIN_MASS_KG = 0
MAX_MASS_KG = 4
SAMPLES_PER_CAPTURE = 100
MONITOR_WINDOW_SAMPLES = 200
```

Run:

```powershell
python hx711_calibration_logger.py
```

Flow:

1. Connect ESP32 over USB.
2. Run the Python script.
3. Remove load and press Enter to tare, or type `skip`.
4. Type a capture command.
5. The script collects 100 samples and saves the matching CSV.

Interactive commands:

```text
loading m0 t1          save to data/hx711_calibration/loading/m0/trial01/hx711_capture.csv
unloading m3 trial02   save to data/hx711_calibration/unloading/m3/trial02/hx711_capture.csv
l m4 t3 data.csv       loading shortcut, custom filename
u m2 t1                unloading shortcut
monitor / mon          open live raw/delta_raw graph
status / s             show current ESP32 tare status
tare              send tare command again
help              show commands
q                 quit
```

Current calibration range is `0-4 kg` (`m0` to `m4`). The 5 kg point is out of
scope for this setup because the bicycle can move forward without a person
holding it.

Recommended hysteresis sequence:

```text
loading:   m0 -> m1 -> m2 -> m3 -> m4
unloading:       m3 -> m2 -> m1 -> m0
```

The `m4` point is the shared peak from the loading sequence. It is not collected
again as `unloading m4` unless you intentionally want a duplicate peak capture.

If the live graph is open and you run a collect or tare command, Python stops
live reporting with `r0`, closes the graph, flushes the serial buffer, and then
runs the requested command.

Use `status` to check whether the ESP32 still has a tare value in RAM:

```text
STATUS,is_tared=1,zero_raw=-47264
```

## CSV Output

Default output:

```text
data/hx711_calibration/<loading-or-unloading>/m<mass>/trial<trial>/hx711_capture.csv
```

Columns:

```text
sample_idx,time_ms,raw,delta_raw,zero_raw
```

- `sample_idx`: sample number from 1 to 100.
- `time_ms`: ESP32 `millis()` timestamp for that sample.
- `raw`: raw HX711 ADC count.
- `delta_raw`: `raw - zero_raw`.
- `zero_raw`: tare baseline stored on the ESP32.

If the script receives fewer than 100 samples, it prints a warning but still
saves whatever data was received.
