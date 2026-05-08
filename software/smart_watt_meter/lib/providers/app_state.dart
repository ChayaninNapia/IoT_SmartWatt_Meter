import 'dart:async';
import 'package:flutter/foundation.dart';
import '../services/ble_service.dart';
import '../services/gps_service.dart';
import '../services/firestore_service.dart';
import '../services/ride_session_service.dart';
import '../models/ride_sample.dart';
import '../models/ride_summary.dart';

class AppState extends ChangeNotifier {
  final bleService = BleService();
  final gpsService = GpsService();
  final firestoreService = FirestoreService();
  late final RideSessionService rideSession;

  String bleStatus = 'idle';
  List<RideSummary> rideHistory = [];

  // Samples from the most recently completed session (for RideDetailScreen)
  List<RideSample> lastSessionSamples = [];

  bool _gpsOk = false;
  bool get gpsOk => _gpsOk;

  StreamSubscription? _eventSubscription;
  StreamSubscription? _bleStatusSubscription;

  AppState() {
    rideSession = RideSessionService(
      bleService: bleService,
      gpsService: gpsService,
      firestoreService: firestoreService,
    );

    rideSession.initLiveUpdates();

    _bleStatusSubscription = bleService.statusStream.listen((s) {
      bleStatus = s;
      if (s == 'connected') {
        _onBleConnected();
      } else if (s == 'disconnected') {
        bleService.stopAppStatusHeartbeat();
      }
      notifyListeners();
    });

    rideSession.onUpdate.listen((_) {
      notifyListeners();
    });

    // Handle events sent by ESP32 (start_request, stop_request, tare_ok, …)
    _eventSubscription = bleService.eventStream.listen(_handleBleEvent);
  }

  // Called once after the BLE stack reports connected AND characteristics
  // are discovered (statusStream emits 'connected' after _discoverCharacteristics).
  void _onBleConnected() {
    // Send current status immediately so OLED updates right away.
    _sendAppStatusNow();

    // Start 1 s heartbeat so OLED GPS/REC indicators stay alive.
    bleService.startAppStatusHeartbeat(
      gpsOkGetter: () => _gpsOk,
      recordingGetter: () => rideSession.isRecording,
    );
  }

  void _sendAppStatusNow() {
    bleService.sendAppStatus(
      gpsOk: _gpsOk,
      recording: rideSession.isRecording,
    );
  }

  void _handleBleEvent(BleEvent event) {
    debugPrint('[AppState] BLE event: ${event.event}');
    switch (event.event) {
      case 'start_request':
        if (bleStatus == 'connected' && !rideSession.isRecording) {
          startRide();
        }
      case 'stop_request':
        if (rideSession.isRecording) {
          stopRide();
        }
      default:
        // tare_ok, gyro_bias_ok, axis_cal_ok, etc. — log only for now.
        break;
    }
  }

  Future<void> init() async {
    await firestoreService.signInAnonymously();
    await loadHistory();
    // Start GPS tracking so live GPS speed is available before a session begins.
    gpsService.startTracking();
    // Watch GPS fix status to keep OLED in sync.
    gpsService.positionStream.listen((pos) {
      final nowOk = pos.accuracy < 50; // reasonable fix threshold
      if (nowOk != _gpsOk) {
        _gpsOk = nowOk;
        _sendAppStatusNow();
        notifyListeners();
      }
    });
  }

  Future<void> scanAndConnect() async {
    await bleService.startScan();
  }

  Future<void> disconnect() async {
    await bleService.disconnect();
  }

  void startRide() {
    rideSession.startRecording();
    _sendAppStatusNow(); // tell ESP32 OLED REC = true
    notifyListeners();
  }

  Future<void> stopRide() async {
    lastSessionSamples = List.from(rideSession.samples);
    await rideSession.stopRecording();
    _sendAppStatusNow(); // tell ESP32 OLED REC = false
    await loadHistory();
    notifyListeners();
  }

  Future<void> loadHistory() async {
    try {
      rideHistory = await firestoreService.fetchRides();
      notifyListeners();
    } catch (_) {}
  }

  // Command helpers — callers (e.g. future UI buttons) can invoke these.
  Future<void> sendTare() => bleService.sendTare();
  Future<void> sendGyroBias() => bleService.sendGyroBias();
  Future<void> sendAxisCal() => bleService.sendAxisCal();

  @override
  void dispose() {
    bleService.stopAppStatusHeartbeat();
    _eventSubscription?.cancel();
    _bleStatusSubscription?.cancel();
    bleService.dispose();
    gpsService.dispose();
    super.dispose();
  }
}
