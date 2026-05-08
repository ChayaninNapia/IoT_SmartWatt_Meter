# Smart Watt-Meter — Run Instructions

## Prerequisites
- Flutter installed (stable channel)
- Android Studio + Pixel 4a emulator
- Android SDK Platform 34 installed

---

## 1. Start the Emulator

Open **Android Studio** → **Device Manager** → Start **Pixel 4a**

Or via terminal:
```bash
flutter emulators --launch Pixel_4a
```

---

## 2. Run the App

```bash
cd "C:\Users\Acer\Desktop\IoT\iot_bicycle_wattMeter_project\software\smart_watt_meter"
flutter run -d emulator-5554
```

Wait ~1-2 minutes for the first build (Gradle downloads dependencies).  
Subsequent runs are much faster.

---

## 3. Hot Reload / Hot Restart

While the app is running in terminal:

| Key | Action |
|-----|--------|
| `r` | Hot reload (apply code changes instantly) |
| `R` | Hot restart (restart app, resets state) |
| `q` | Quit |

---

## 4. What Works Right Now

| Feature | Status |
|---------|--------|
| Device screen (BLE scan UI) | ✅ Works |
| Power Dashboard screen | ✅ Works |
| Map screen | ✅ Works (placeholder — no Google Maps key yet) |
| History screen | ✅ Works (empty until Firebase is set up) |
| BLE connect to ESP32 | ✅ Ready (needs real ESP32 device) |
| Firebase upload | ❌ Needs `google-services.json` |
| Google Maps route | ❌ Needs Maps API key |

---

## 5. Set Up Firebase (for History / Cloud Sync)

### Step 1 — Create Firebase project
1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Create project → Enable **Firestore** (test mode) + **Anonymous Auth**

### Step 2 — Connect to app
```bash
# Install FlutterFire CLI (once)
dart pub global activate flutterfire_cli

# In the project folder
cd "C:\Users\Acer\Desktop\IoT\iot_bicycle_wattMeter_project\software\smart_watt_meter"
flutterfire configure
```
This generates `lib/firebase_options.dart` and downloads `google-services.json` automatically.

### Step 3 — Update main.dart
Replace:
```dart
await Firebase.initializeApp();
```
With:
```dart
await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform);
```
And add import:
```dart
import 'firebase_options.dart';
```

---

## 6. Set Up Google Maps (for Map screen)

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Enable **Maps SDK for Android**
3. Create an API key
4. Open `android/app/src/main/AndroidManifest.xml`
5. Replace `YOUR_GOOGLE_MAPS_API_KEY` with your key
6. Uncomment the import in `lib/screens/map_screen.dart` and restore the `GoogleMap` widget

---

## Project Structure

```
lib/
├── main.dart                  ← App entry point + navigation shell
├── models/
│   ├── ride_sample.dart       ← Single 1Hz telemetry data point
│   └── ride_summary.dart      ← Ride metadata (avg power, distance, etc.)
├── services/
│   ├── ble_service.dart       ← BLE scan, connect, GATT notify
│   ├── gps_service.dart       ← Phone GPS tracking + distance calc
│   ├── firestore_service.dart ← Firebase upload + ride history fetch
│   └── ride_session_service.dart ← Merges BLE + GPS, manages recording
├── providers/
│   └── app_state.dart         ← Global state (Provider)
├── screens/
│   ├── device_screen.dart     ← Tab 1: BLE scan & connect
│   ├── power_dashboard_screen.dart ← Tab 2: Live power + start/stop
│   ├── map_screen.dart        ← Tab 3: GPS route map
│   └── history_screen.dart    ← Tab 4: Past rides list
└── widgets/
    └── power_chart.dart       ← 60-second bar chart widget
```
