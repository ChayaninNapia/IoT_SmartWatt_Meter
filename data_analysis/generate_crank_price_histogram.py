from __future__ import annotations

import csv
from pathlib import Path

import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parent
CSV_PATH = ROOT / "crank_power_meter_price_data.csv"
OUTPUT_PATH = ROOT / "crank_power_meter_price_histogram.png"


def load_prices(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as file:
        return list(csv.DictReader(file))


def main() -> None:
    rows = load_prices(CSV_PATH)
    rows = sorted(rows, key=lambda row: int(row["price_thb"]))
    prices = [int(row["price_thb"]) for row in rows]
    short_names = [row["short_label"].replace(" ", "\n", 1) for row in rows]

    plt.style.use("seaborn-v0_8-whitegrid")
    fig, ax = plt.subplots(figsize=(11, 6), dpi=160)

    bars = ax.bar(short_names, prices, color="#2C7FB8", edgecolor="#0B3954", alpha=0.92)

    ax.set_title("Crank Arm Power Meter Prices", fontsize=14, weight="bold")
    ax.set_xlabel("Short Product Name")
    ax.set_ylabel("Price (THB)")
    ax.set_ylim(0, max(prices) * 1.16)
    ax.tick_params(axis="x", labelrotation=0)

    for bar, price, row in zip(bars, prices, rows):
        x_center = bar.get_x() + bar.get_width() / 2
        ax.text(
            x_center,
            price * 0.5,
            row["brand"],
            ha="center",
            va="center",
            fontsize=8,
            color="white",
            weight="bold",
            rotation=90,
        )
        ax.text(
            x_center,
            price + max(prices) * 0.015,
            f"{price:,.0f}",
            ha="center",
            va="bottom",
            fontsize=8,
            color="#0B3954",
            weight="bold",
        )

    fig.tight_layout()
    fig.savefig(OUTPUT_PATH, bbox_inches="tight")
    plt.close(fig)

    print(f"Loaded {len(rows)} listings from {CSV_PATH.name}")
    print(f"Brands sampled: {', '.join(row['brand'] for row in rows)}")
    print(f"Saved histogram to {OUTPUT_PATH.name}")


if __name__ == "__main__":
    main()
