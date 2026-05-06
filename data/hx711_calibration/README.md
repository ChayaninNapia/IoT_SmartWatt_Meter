# HX711 Calibration Data

This folder is for exported HX711 calibration capture data.

## Folder Layout

```text
data/hx711_calibration/
  loading/
    m0/
      trial01/
      trial02/
      trial03/
    ...
    m4/
      trial01/
      trial02/
      trial03/
  unloading/
    m0/
      trial01/
      trial02/
      trial03/
    ...
    m4/
      trial01/
      trial02/
      trial03/
```

## Raw CSV

Each trial folder stores one exported capture CSV. The current calibration range
is 0-4 kg. Use `loading` while adding mass and `unloading` while removing mass.
The 5 kg point is intentionally skipped because the bicycle can move forward
without a person holding it.

For the current hysteresis experiment, `loading/m4` is the peak point and is
shared as the start of the unloading path. Separate `unloading/m4` captures are
not required; after reaching 4 kg, collect unloading data at `m3`, `m2`, `m1`,
and `m0`.

Columns:

```text
sample_idx,time_ms,raw,delta_raw,zero_raw
```

Copy or upload this CSV to Google Sheets when you are ready to analyze the
capture.
