# IoT Smart Watt Meter

An ESP32-based bicycle watt meter project for FRA503. The system combines an
MPU6050 IMU, HX711 strain-gauge force sensing, an SH1106 OLED, four physical
buttons, a passive buzzer, BLE telemetry, and a Flutter mobile app for ride
display, GPS, recording, history, and cloud sync.

## Project Status

Current integration focus:

- Full-system ESP32 firmware for bench and field testing.
- BLE telemetry between ESP32 and Flutter.
- Gyroscope-based crank cadence estimation.
- HX711 raw capture and calibration workflow.
- Flutter app screens for live power, device connection, map, history, and ride
  detail.

Hardware evidence has confirmed the OLED, buzzer, four active-high buttons,
MPU6050 raw/gyro readings, and several IMU/OLED/buzzer/button integration
tests. HX711 calibration is still the key remaining measurement step before
force and wattage should be treated as final.

## Repository Layout

```text
electrical/
  unit_test/                         Single-component ESP32 test sketches
  integration_test/
    smart_watt_meter_full_system/    Main full-system ESP32 firmware
  experiment/
    hx711_calibration/               HX711 capture logger and ESP32 collector
    imu_cadence_validation/          Gyro cadence validation firmware

software/
  smart_watt_meter/                  Flutter mobile app
  firmware/smart_watt_meter_mock/    Mock telemetry firmware
  docs/                              Software/report documents
  ui design/                         UI mockups and design references

data/
  hx711_calibration/                 Captured calibration CSV data

data_analysis/                       Analysis scripts and notebooks
diagrams/                            System and wiring diagrams
images/                              Project images
proposal/, report/, draft/           Report and proposal materials
research_papers/                     Reference papers
```

## Hardware Summary

Target board:

```text
ESP32 NodeMCU-32S / ESP-32S Kit
Arduino board target: ESP32 Dev Module
CLI FQBN: esp32:esp32:esp32
```

Current GPIO map:

| Function | GPIO | Notes |
| --- | ---: | --- |
| HX711 DOUT | 32 | Digital input |
| HX711 SCK | 33 | Digital output |
| I2C SDA | 21 | Shared by OLED and MPU6050 |
| I2C SCL | 22 | Shared by OLED and MPU6050 |
| Button 1 | 25 | Active-high, `INPUT_PULLDOWN` |
| Button 2 | 26 | Active-high, `INPUT_PULLDOWN` |
| Button 3 | 27 | Active-high, `INPUT_PULLDOWN` |
| Button 4 | 14 | Active-high, `INPUT_PULLDOWN` |
| Passive buzzer | 13 | Tone/PWM output |

Button 5 / GPIO16 is not part of the current system.

OLED:

- 1.3 inch I2C OLED
- SH1106-compatible
- Address `0x3C`
- Driver: `Adafruit_SH110X` / `Adafruit_SH1106G`

## Main ESP32 Firmware

Full-system firmware:

```text
electrical/integration_test/smart_watt_meter_full_system/smart_watt_meter_full_system.ino
```

It combines:

- SH1106 OLED screen UI.
- MPU6050 cadence estimation.
- HX711 raw/tare/force path.
- Four active-high buttons.
- Passive buzzer feedback.
- BLE telemetry for the Flutter app.

BLE configuration:

```text
Device name: ESP32_PowerMeter_01
Service UUID: 12345678-1234-1234-1234-123456789012
Telemetry notify UUID: 12345678-1234-1234-1234-123456789013
App status / command write UUID: 12345678-1234-1234-1234-123456789014
```

Example telemetry packet:

```json
{"v":1,"type":"telemetry","seq":128,"tMs":128420,"powerW":245.3,"cadenceRpm":82.1,"forceN":34.20,"torqueNm":5.81,"omegaRadS":8.600,"hxRaw":29120,"hxTared":true,"imuOk":true,"hxOk":true,"calOk":true}
```

Example app status write:

```json
{"type":"appStatus","gpsOk":true,"recording":true}
```

Example app commands:

```json
{"type":"command","cmd":"tare"}
{"type":"command","cmd":"gyroBias"}
{"type":"command","cmd":"axisCal"}
```

## Arduino Compile Checks

The Arduino IDE bundled CLI is used because `arduino-cli` is not expected to be
on the system `PATH`.

CLI path on the development machine:

```powershell
& 'C:\Users\Acer\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' version
```

Compile the full-system firmware:

```powershell
& 'C:\Users\Acer\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile --fqbn esp32:esp32:esp32 .\electrical\integration_test\smart_watt_meter_full_system
```

Compile the IMU cadence experiment:

```powershell
& 'C:\Users\Acer\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile --fqbn esp32:esp32:esp32 .\electrical\experiment\imu_cadence_validation
```

Compile the HX711 trial collector:

```powershell
& 'C:\Users\Acer\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile --fqbn esp32:esp32:esp32 .\electrical\experiment\hx711_calibration\esp32_hx711_trial_collector
```

Compile success only proves the firmware builds. Hardware tests must still be
verified with serial logs, OLED output, buzzer response, photos, videos, or
recorded experiment data.

## HX711 Calibration Workflow

Calibration tools:

```text
electrical/experiment/hx711_calibration/
```

Important files:

```text
hx711_calibration_logger.py
esp32_hx711_trial_collector/esp32_hx711_trial_collector.ino
```

Install Python dependencies:

```powershell
pip install pyserial matplotlib
```

Run the logger:

```powershell
python electrical\experiment\hx711_calibration\hx711_calibration_logger.py
```

The workflow captures 100 valid HX711 samples per trial into:

```text
data/hx711_calibration/<loading-or-unloading>/m<mass>/trial<trial>/hx711_capture.csv
```

Current calibration range is `0-4 kg`. Use loading and unloading trials to
check scale, residual error, and hysteresis before treating force in newtons as
calibrated.

## IMU Cadence Validation

Cadence validation sketch:

```text
electrical/experiment/imu_cadence_validation/imu_cadence_validation.ino
```

The method:

1. Measure gyro bias while stationary.
2. Subtract bias from gyro readings.
3. Estimate the crank rotation axis in the IMU frame.
4. Project the gyro vector onto that axis.
5. Convert angular speed to RPM.
6. Compare IMU RPM against video-derived reference RPM.

The full-system firmware starts with this default crank unit vector unless the
axis calibration is refreshed:

```text
ux = -0.074
uy =  0.995
uz =  0.065
```

## Flutter App

App folder:

```text
software/smart_watt_meter/
```

Key dependencies:

- `flutter_blue_plus` for BLE.
- `geolocator` and `permission_handler` for phone GPS.
- `flutter_map` and `latlong2` for maps.
- `firebase_core`, `cloud_firestore`, and `firebase_auth` for cloud sync.
- `provider` for app state.

Run from the app folder:

```powershell
cd software\smart_watt_meter
flutter pub get
flutter run
```

Run Flutter tests:

```powershell
cd software\smart_watt_meter
flutter test
```

The app connects to `ESP32_PowerMeter_01`, subscribes to telemetry notify
packets, writes phone GPS/recording status back to the ESP32, records ride
samples, and displays ride history/details.

## Git Notes

Large generated folders are intentionally ignored:

```text
build/
**/.gradle_user_home/
.dart_tool/
**/.claude/
```

Do not commit local build caches, Gradle user-home downloads, Flutter build
outputs, or local tool settings. The repository should keep source code,
firmware sketches, documentation, calibration data, and intentional design
artifacts only.

## Safety Notes

- Keep the crank/IMU still during gyro bias calibration.
- Remove all load before HX711 tare.
- Do not use GPIO12 for a button in this ESP32 project because it is a boot
  strapping pin.
- Do not mark hardware tests as passed from compile success alone.
- Treat wattage as experimental until HX711 calibration has been validated with
  known loads.
