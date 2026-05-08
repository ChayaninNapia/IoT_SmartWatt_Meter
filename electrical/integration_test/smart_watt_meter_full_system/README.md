# Smart Watt Meter Full-System Firmware

Firmware folder:

```text
electrical/integration_test/smart_watt_meter_full_system/
```

Sketch:

```text
smart_watt_meter_full_system.ino
```

This firmware is for the real full-system bench/field test:

- ESP32 Dev Module / NodeMCU-32S
- SH1106 OLED 128x64
- MPU6050
- HX711
- 4 active-high buttons
- passive buzzer
- BLE connection to the Flutter app

## Wiring Summary

```text
OLED + MPU6050 I2C:
  SDA = GPIO21
  SCL = GPIO22

HX711:
  DOUT = GPIO32
  SCK  = GPIO33
  gain = 64

Buttons, active-high with INPUT_PULLDOWN:
  BTN1 = GPIO25
  BTN2 = GPIO26
  BTN3 = GPIO27
  BTN4 = GPIO14

Buzzer:
  GPIO13
```

Button 5 / GPIO16 is not used.

## OLED Screens

Use `BTN1` and `BTN2` to move between screens.

```text
BTN1 = previous screen
BTN2 = next screen
```

Screens:

```text
LIVE    main ride display
SENSOR  IMU + HX711 status
HX      HX711 raw / force / tare page
BLE     app / notify / packet status
SYS     uptime, heap, calibration status
```

Every screen has a top bar:

```text
TITLE        BT GPS REC
```

- Bluetooth icon blinking/visible = BLE advertising or connected
- `GPS` = app reports phone GPS fix OK
- `REC` = app reports ride recording active

The app must write status to BLE characteristic:

```text
12345678-1234-1234-1234-123456789014
```

Example:

```json
{"type":"appStatus","gpsOk":true,"recording":true}
```

## Button Controls

```text
BTN1 short press:
  Previous OLED screen.

BTN2 short press:
  Next OLED screen.

BTN3 short press:
  Reserved / no operation.

BTN3 long press on SENSOR or SYS:
  Gyro bias calibration.
  Keep the crank/IMU still.

BTN4 long press on HX or SENSOR:
  HX711 tare.
  Remove all load first.

BTN4 long press on LIVE or SYS:
  Crank axis calibration.
  Spin crank steadily for about 10 seconds.
```

Long press is about 1.5 seconds.

## First-Time Bench Test Flow

Use this order after uploading the firmware.

### 1. Boot

Power on the ESP32. You should hear the startup tone and see a boot/self-check screen.

Expected:

```text
SELF CHECK
IMU OK
BLE ADV ON
```

If IMU says `NO`, check I2C wiring, power, and GND.

### 2. Check HX711 Raw

Go to the `HX` screen:

```text
BTN2 until HX screen appears
```

Before tare, the screen should show raw mode:

```text
HX        BT --- ---
NO TARE  c/N 852
RAW 242588
TARE NEEDED
Hold B4 tare
```

If `RAW` changes when you press/load the strain gauge, HX711 is being read.

Important: before tare, force in newtons is not shown yet. Raw value is the useful check.

### 3. Tare HX711

Remove all load from the strain gauge/crank.

On `HX` or `SENSOR` screen:

```text
Hold BTN4
```

Expected:

```text
TARE OK
```

After tare, `HX` screen should show force and delta raw:

```text
TARE OK  c/N 852
F 0.00N
dR 0
sample ... B4 tare
```

Apply load and check that `F` changes.

### 4. Gyro Bias Calibration

Keep crank/IMU still.

Go to `SENSOR` or `SYS` screen:

```text
Hold BTN3
```

Expected:

```text
BIAS OK
```

This removes stationary gyro offset.

### 5. Crank Axis Calibration

This step is optional. The firmware already starts with the crank unit vector
measured from experiment 3.2:

```text
ux = -0.074
uy =  0.995
uz =  0.065
```

If the MPU6050/crank mounting has not changed, you can skip this step.

Run this only when you want to refresh the crank axis for the current mounting.

Go to `LIVE` or `SYS` screen.

Spin the crank steadily in one direction.

```text
Hold BTN4
```

Keep spinning for about 10 seconds.

Expected:

```text
AXIS OK
```

This lets firmware estimate the real crank rotation axis from the current
MPU6050 mounting and replace the default vector above.

### 6. Connect the Flutter App

Open the app and connect to:

```text
ESP32_PowerMeter_01
```

On OLED `BLE` screen:

```text
APP CONN
NOTIFY ON
SEQ ...
CMD ...014
```

If the app sends status correctly, the top bar should show:

```text
GPS
REC
```

`GPS` appears only when the app reports phone GPS fix OK.

`REC` appears only when the app is recording and writes app status back to ESP32.

### 7. Start / Stop Ride

Start and stop ride recording from the phone app:

```text
Phone app Start Session
```

The ESP32 buttons do not start or stop recording. BTN1/BTN2 are reserved for
OLED page navigation so accidental button presses do not affect ride sessions.

## LIVE Screen Meaning

Before HX tare / gyro bias:

```text
LIVE       BT GPS ---
0 W
0rpm R242588
TQ 0.0Nm
HX RAW RAW
```

Meaning:

- HX711 raw is visible as `Rxxxxx`
- force/power is not trusted yet
- `RAW` means gyro bias has not been completed yet

After HX tare + gyro bias, using the default experiment 3.2 crank axis:

```text
LIVE       BT GPS REC
245 W
82rpm 34N
TQ 5.9Nm
HX OK AX DEF
```

Meaning:

- power is computed using the default experiment 3.2 unit vector
- crank axis calibration has not been refreshed in this boot session

After optional crank axis calibration:

```text
LIVE       BT GPS REC
245 W
82rpm 34N
TQ 5.9Nm
HX OK CAL
```

Meaning:

- power is computed from torque and angular velocity
- force is shown in newtons
- `CAL` means firmware is using a freshly calibrated crank axis from this boot session

## BLE Packets

Telemetry notify characteristic:

```text
12345678-1234-1234-1234-123456789013
```

Packet example:

```json
{"v":1,"type":"telemetry","seq":128,"tMs":128420,"powerW":245.3,"cadenceRpm":82.1,"forceN":34.20,"torqueNm":5.81,"omegaRadS":8.600,"hxRaw":29120,"hxTared":true,"imuOk":true,"hxOk":true,"calOk":true}
```

App status / command write characteristic:

```text
12345678-1234-1234-1234-123456789014
```

App status example:

```json
{"type":"appStatus","gpsOk":true,"recording":true}
```

Command examples:

```json
{"type":"command","cmd":"tare"}
{"type":"command","cmd":"gyroBias"}
{"type":"command","cmd":"axisCal"}
```

Event examples sent by ESP32:

```json
{"v":1,"type":"event","seq":46,"tMs":55400,"event":"tare_ok"}
{"v":1,"type":"event","seq":47,"tMs":55500,"event":"gyro_bias_ok"}
{"v":1,"type":"event","seq":48,"tMs":55600,"event":"axis_cal_ok"}
```

## Compile

Use ESP32 Dev Module:

```powershell
& 'C:\Users\Acer\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile --fqbn esp32:esp32:esp32 .\electrical\integration_test\smart_watt_meter_full_system
```

## Troubleshooting

### HX711 raw sketch works, but full firmware looks like no value

Go to the `HX` screen first. Before tare, full firmware shows raw value, not force.

Expected before tare:

```text
NO TARE
RAW xxxxx
```

Expected after tare:

```text
TARE OK
F x.xxN
dR xxxxx
```

### OLED shows `HX WAIT`

Check:

- HX711 VCC/GND
- DOUT GPIO32
- SCK GPIO33
- gain set to 64
- common ground with ESP32

### OLED does not show `GPS` or `REC`

This requires the phone app to write app status to characteristic `...014`.

Check app BLE logs for:

```text
sendAppStatus
```

### LIVE shows `RAW` or `AX DEF`

Run the common quick setup first:

```text
1. HX tare:       hold BTN4 on HX/SENSOR
2. Gyro bias:     hold BTN3 on SENSOR/SYS while still
```

If LIVE shows `AX DEF`, it is using the default experiment 3.2 crank axis:

```text
ux = -0.074, uy = 0.995, uz = 0.065
```

This is acceptable if the IMU/crank mounting has not changed.

Optional precision step:

```text
3. Axis cal:      hold BTN4 on LIVE/SYS while spinning crank
```

After that, LIVE should show `CAL` instead of `AX DEF`.
