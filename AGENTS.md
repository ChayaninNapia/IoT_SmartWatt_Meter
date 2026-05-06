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

Last updated: 2026-05-05.

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
- Detailed gyro axis/sign/scale verification belongs in the later angle integration/calibration tests.

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

### HX711 Calibration Preparation

- HX711 uses DOUT GPIO32 and SCK GPIO33.
- User reported on 2026-05-05 that the HX711 circuit has been connected and has no obvious wiring/circuit issue.
- The remaining HX711 work is calibration: map raw ADC counts to force in newtons using known loads.
- Updated calibration plan uses multi-point loading with 1 kg x2 and 2 kg x2 reference bags, covering 0-6 kg:

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
