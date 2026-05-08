# Agent Notes

## Project Planning Source Of Truth

- Use the Google Sheet `IoT Watt Meter Implementation Plan` as the source of truth for project planning, GPIO mapping, unit tests, integration tests, BOM, calibration logs, and experiment logs.
- Google Sheet URL: `https://docs.google.com/spreadsheets/d/1TLvIBa0SyqpkKix25UrbDlp2DpvdeKhnuRGYiF-QxSA/edit`.
- Before changing firmware, wiring assumptions, or test status, check the relevant tabs in this sheet, especially `Implementation Plan`, `GPIO Mapping`, and `Unit Test Checklist`.
- When a unit test, integration test, or implementation task changes status, update the corresponding row in the Google Sheet with concise evidence.
- Do not mark a hardware unit test as `Pass` from compile success alone. Use `In Progress` when firmware compiles but physical hardware evidence, such as audible buzzer output, serial log, photo, or video, is still missing.
- Preserve the sheet's existing status values and validation choices: `Todo`, `In Progress`, `Pass`, `Fail`, `Blocked`, `Done`.
- In `Unit Test Checklist`, keep the two-group structure:
  - `Single Component Unit Tests` uses IDs `UT-S-001`, `UT-S-002`, etc.
  - `Multi-component / Pre-integration Checks` uses IDs `UT-M-001`, `UT-M-002`, etc.
- Do not mix multi-component checks into the single-component group. I2C bus scan, OLED + MPU6050, strain gauge + HX711, power stability, and battery-under-load checks belong in the `UT-M` group.
- Keep the `ID` column compact in the sheet; it should be narrow enough for `UT-S-xxx` / `UT-M-xxx`, not auto-expanded from long text.

## Arduino / ESP32 Tooling

- Arduino IDE is installed at `C:\Users\Acer\AppData\Local\Programs\Arduino IDE`.
- The IDE bundles `arduino-cli.exe`, but it is not available from the system `PATH`.
- Use this full path when compiling sketches from the terminal:

```powershell
& 'C:\Users\Acer\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' version
```

- Verified bundled CLI version:

```text
arduino-cli Version: 1.3.1
```

- For this ESP32 NodeMCU-32S / ESP-32S Kit, Arduino IDE should use `ESP32 Dev Module`.
- Do not use an ESP32-S2 board target for this hardware. The IDE reports this board as `ESP32 Dev Module`.
- For ESP32 unit tests, call the bundled CLI by full path instead of assuming `arduino-cli` is globally installed.

## Electrical / Firmware Know-How

Last updated: 2026-05-06.

- Development board: ESP32 NodeMCU-32S / ESP-32S Kit.
- Arduino IDE board target: `ESP32 Dev Module`.
- CLI FQBN for compile checks: `esp32:esp32:esp32`.
- Compile ESP32 unit tests with:

```powershell
& 'C:\Users\Acer\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile --fqbn esp32:esp32:esp32 .\electrical\unit_test\<sketch_folder>
```

### Current GPIO Mapping Notes

| Function | GPIO | Notes |
| --- | ---: | --- |
| HX711 DOUT | 32 | Planned digital input |
| HX711 SCK | 33 | Planned digital output |
| I2C SDA | 21 | Shared by OLED and MPU6050 |
| I2C SCL | 22 | Shared by OLED and MPU6050 |
| Button 1 | 25 | Tested active-high input |
| Button 2 | 26 | Tested active-high input |
| Button 3 | 27 | Tested active-high input |
| Button 4 | 14 | Tested active-high input |
| Passive buzzer | 13 | Tested PWM/tone output |

Current button decision:

- The current system uses 4 buttons only: GPIO25, GPIO26, GPIO27, and GPIO14.
- Button 5 / ZERO-CAL has been removed from the current system.
- GPIO16 is no longer assigned to a button in the current mapping.

### Button Wiring Finding

- The installed enclosure buttons are active-high, not active-low.
- Use `INPUT_PULLDOWN`.
- Released state reads `LOW`.
- Pressed state reads `HIGH`.
- The historical five-button unit test sketch is `electrical/unit_test/all_buttons_test/all_buttons_test.ino`.
- Current integration work should use only Button 1 through Button 4.
- Relevant constants:

```cpp
const uint8_t BUTTON_PIN_MODE = INPUT_PULLDOWN;
const uint8_t BUTTON_PRESSED_LEVEL = HIGH;
```

- Test result on 2026-04-25:
  - BTN1 GPIO25: press/release detected; long press detected.
  - BTN2 GPIO26: press/release detected.
  - BTN3 GPIO27: press/release detected.
  - BTN4 GPIO14: press/release detected.
  - Idle status line shows all buttons as `released(LOW)`.

### Button 5 GPIO Decision

- Button 5 / ZERO-CAL was previously tested on GPIO16, but it has been removed from the current system.
- GPIO12 was considered for Button 5, but it should not be used for a button in this project.
- GPIO12 / MTDI is an ESP32 strapping pin.
- If GPIO12 is pulled HIGH during boot, some ESP32 boards may select 1.8 V flash voltage and fail to boot.
- GPIO16 avoids the GPIO12 boot-time risk, but GPIO16 is currently unassigned because the system now uses only 4 buttons.

### OLED Finding

- The 1.3 inch I2C OLED was confirmed as SH1106-compatible.
- Use `Adafruit_SH110X` and `Adafruit_SH1106G`.
- I2C address: `0x3C`.
- Wiring: SDA GPIO21, SCL GPIO22.
- SSD1306 driver produced noisy/incorrect display output.
- Test sketch: `electrical/unit_test/oled_sh1106_test/oled_sh1106_test.ino`.

### MPU6050 Test Preparation

- MPU6050 uses shared I2C with OLED: SDA GPIO21, SCL GPIO22.
- First bring-up sketch uses direct MPU6050 registers over `Wire`, so no external MPU6050 library is required.
- The sketch scans I2C, checks `0x68` / `0x69`, reads `WHO_AM_I`, wakes the sensor, and prints Serial Plotter values in SI-friendly units: accel `m/s^2`, gyro `rad/s`, temp `degC`.
- Test sketch: `electrical/unit_test/mpu6050_test/mpu6050_test.ino`.
- UT-S-003 is `Pass`; Serial Monitor/Plotter confirms readable accel/gyro SI values on 2026-04-25.
- Detailed gyro axis/sign/scale verification now belongs in the cadence validation experiment, not an angle-integration experiment.

### Passive Buzzer Finding

- Passive buzzer test is on GPIO13.
- Use `tone()` / `noTone()` or LEDC/PWM-style tone output.
- Hardware audible test was confirmed by the user.
- Test sketch: `electrical/unit_test/passive_buzzer_test/passive_buzzer_test.ino`.

### Current Integration Test Notes

- IMU + OLED integration displays MPU6050 gyro values in `rad/s` on the SH1106 OLED.
- IMU + OLED + buzzer integration plays an audible startup sound on GPIO13.
- IMU + OLED + buzzer + 4-button integration uses Button 1 through Button 4 only.
- The 4-button integration sketch is `electrical/integration_test/imu_oled_buzzer_4button_test/imu_oled_buzzer_4button_test.ino`.
- User confirmed on 2026-05-05 that the integrated IMU + OLED + buzzer + 4-button test works on hardware: OLED displays gyro values, buzzer produces startup/respond sounds, and the four buttons can be used with distinct buzzer feedback.
- IT-005 is `Pass`: user confirmed on 2026-05-05 that the 4-button UI/OLED feedback prototype works on hardware.
- Each button press plays a distinct buzzer response tone:
  - BTN1 GPIO25: 659 Hz
  - BTN2 GPIO26: 784 Hz
  - BTN3 GPIO27: 988 Hz
  - BTN4 GPIO14: 1319 Hz

### IMU Cadence Validation Plan

- Current experiment 3.2 in `FRA503_Project_Proposal_Report_65009_draft.md` is `การทดสอบความถูกต้องของการประมาณ Cadence จาก Gyroscope`.
- The goal is to estimate crank cadence/RPM from MPU6050 gyroscope data and compare it with video-derived reference RPM.
- Do not describe this experiment as crank angle integration. The current method is:
  - measure gyro bias while the IMU is stationary,
  - subtract `bias_x`, `bias_y`, and `bias_z` from the gyro readings,
  - estimate the real crank rotation axis in the IMU frame from the mean gyro vector,
  - normalize that vector to obtain `u_crank`,
  - project `[g_x, g_y, g_z]` onto `u_crank` using a dot product,
  - compute `RPM_IMU = |omega_crank| * 60 / 360`,
  - compare against `RPM_ref = (N_rev / delta_t) * 60` from video.
- Planned trials:
  - Trial 1: slow rotation, 10 revolutions.
  - Trial 2: normal-speed rotation, 10 revolutions.
  - Trial 3: faster rotation, 10 revolutions.
- Record `N_rev`, `delta_t`, `RPM_ref`, `RPM_IMU_avg`, and `Error(%)`.
- Evaluation target: average `Error(%)` no more than 10% versus `RPM_ref`, with Serial log, CSV, or video evidence.
- Firmware sketch for this workflow is `electrical/experiment/imu_cadence_validation/imu_cadence_validation.ino`.
- The sketch compiles for `esp32:esp32:esp32` and implements OLED-guided steps:
  - Step 1: gyro bias calibration for 5 s.
  - Step 2: crank axis calibration for 10 s at 50 Hz.
  - Step 3: repeatable 10 s trial measurement with average RPM shown on OLED.
- Axis result UI currently shows `ux`, `uy`, and `uz` to 3 decimal places plus valid sample count `n`.
- User reported on 2026-05-06 that experiment 3.2 data collection has been completed. The next work is to use the collected values for documentation / sheet entry and continue toward real-use integration testing.

### Current Field Test Strategy

- The user plans to prepare experiment 3.4 Full Integration Test before carrying the bicycle outside for real-use testing.
- Rationale: if the integrated system works first, the user can test experiment 3.4 and experiment 3.3 mechanical durability in the same outdoor session.
- Next firmware priority should be a 3.4 integration sketch or update that confirms the combined system works before field testing:
  - ESP32 power stability,
  - OLED status display,
  - MPU6050 / cadence path,
  - HX711 raw or calibrated force path,
  - 4-button input,
  - buzzer feedback,
  - BLE / app / cloud path if included in the current 3.4 scope.
- Before outdoor testing, prefer a room/bench check that the firmware does not freeze or reboot and that OLED values update while sensors are exercised.

### Full-System BLE + OLED Firmware

- Full-system integration firmware is at `electrical/integration_test/smart_watt_meter_full_system/smart_watt_meter_full_system.ino`.
- It compiles for `esp32:esp32:esp32` as of 2026-05-06. Compile success is not hardware pass evidence.
- The sketch combines:
  - SH1106 OLED UI with a global top status bar (`LIVE/SENSOR/HX/BLE/SYS` title + Bluetooth icon + `GPS` + `REC`).
  - MPU6050 gyro cadence estimation with gyro bias calibration and crank-axis calibration.
  - HX711 force readout/tare using GPIO32/GPIO33 and default `HX711_GAIN = 64`.
  - Force/torque/power calculation with `CRANK_ARM_LENGTH_M = 0.170`.
  - 4 active-high buttons on GPIO25/GPIO26/GPIO27/GPIO14.
  - Passive buzzer feedback on GPIO13.
  - BLE telemetry notify packet v2 for the Flutter app.
- BLE UUIDs:
  - Device name: `ESP32_PowerMeter_01`.
  - Service UUID: `12345678-1234-1234-1234-123456789012`.
  - Telemetry notify characteristic UUID: `12345678-1234-1234-1234-123456789013`.
  - App status / command write characteristic UUID: `12345678-1234-1234-1234-123456789014`.
- ESP32 telemetry packet:

```json
{"v":1,"type":"telemetry","seq":128,"tMs":128420,"powerW":245.3,"cadenceRpm":82.1,"forceN":34.20,"torqueNm":5.81,"omegaRadS":8.600,"hxRaw":29120,"hxTared":true,"imuOk":true,"hxOk":true,"calOk":true}
```

- App-to-device status write example:

```json
{"type":"appStatus","gpsOk":true,"recording":true}
```

- App-to-device command examples:

```json
{"type":"command","cmd":"tare"}
{"type":"command","cmd":"gyroBias"}
{"type":"command","cmd":"axisCal"}
```

- Button behavior in the full-system sketch:
  - BTN1 short: previous OLED screen.
  - BTN2 short: next OLED screen.
  - BTN3 short: reserved / no operation.
  - BTN3 long on SENSOR/SYS: gyro bias calibration; keep crank still.
  - BTN4 long on HX/SENSOR/SYS: HX711 tare; remove load.
  - BTN4 long on LIVE/SYS: crank axis calibration; spin crank steadily.
- Ride recording should be started/stopped from the phone app only. ESP32 buttons do not send start/stop ride requests.
- Default crank unit vector from experiment 3.2 is used immediately unless refreshed by optional axis calibration:
  - `ux = -0.074`
  - `uy = 0.995`
  - `uz = 0.065`
- Gyro bias calibration is still the normal quick calibration step (`BTN3` long). Crank axis calibration is optional if the IMU/crank mounting has not changed.

### HX711 Calibration Preparation

- HX711 uses DOUT GPIO32 and SCK GPIO33.
- User reported on 2026-05-05 that the HX711 circuit has been connected and has no obvious wiring/circuit issue.
- The remaining HX711 work is calibration: map raw ADC counts to force in newtons using known loads.
- Updated calibration plan uses multi-point loading with 1 kg x2 and 2 kg x2 reference bags, but the current documented experiment limits collected loads to 0-4 kg for safety:

```text
known_force_N = mass_kg * 9.80665
delta_raw = raw_average - zero_raw_average
delta_raw = m * known_force_N + b
counts_per_newton = m
force_N = (raw_average - zero_raw_average - b) / counts_per_newton
```

- Calibration should use HX711 tare/offset and averaged readings; plot calibration curve, time series, residual/error, and loading-vs-unloading hysteresis. Do not treat a raw reading or compile success as calibrated force evidence.
- Draft report/calibration documentation is at `FRA503_Project_Proposal_Report_65009_draft.md`.

### Unit Test Status Notes

Passed with hardware evidence:

- ESP32 board upload sanity check.
- ESP32 GPIO output sanity check.
- OLED SH1106 display test.
- Passive buzzer audible test.
- Power switch on/off test.
- Enclosure mechanical button fit test.
- Battery pack open-circuit voltage check.
- Battery wiring / connector safety check.
- Button 1 through Button 4 input tests.
- MPU6050 accelerometer/gyroscope raw value test.
- IMU + OLED integration display of live gyro `rad/s` values.
- IMU + OLED + buzzer startup sound integration.
- IMU + OLED + buzzer + 4-button integration test.

Still requiring hardware evidence:

- HX711 raw reading test.
- HX711 calibration from ADC raw count to force in N.
- Strain gauge bridge continuity/basic electrical check.
- Voltmeter module comparison against multimeter.

Current scope changes:

- Button 5 / GPIO16 is no longer part of the planned system, so future integration tests should not require Button 5.
