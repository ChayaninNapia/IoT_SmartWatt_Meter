import 'dart:async';
import 'package:geolocator/geolocator.dart';
import 'package:uuid/uuid.dart';
import '../models/ride_sample.dart';
import '../models/ride_summary.dart';
import 'ble_service.dart';
import 'gps_service.dart';
import 'firestore_service.dart';

class RideSessionService {
  final BleService bleService;
  final GpsService gpsService;
  final FirestoreService firestoreService;

  RideSessionService({
    required this.bleService,
    required this.gpsService,
    required this.firestoreService,
  });

  bool _recording = false;
  DateTime? _startTime;
  final List<RideSample> _samples = [];
  StreamSubscription? _liveSubscription;
  StreamSubscription? _gpsSubscription;
  Position? _latestPosition;

  // Dropped packet tracking (seq-based)
  int? _lastSeq;
  int _droppedPacketCount = 0;
  int _deviceSampleCount = 0;
  int _gpsSampleCount = 0;

  // Sensor status from last received packet
  bool? _lastCalOk;
  bool? _lastHxTared;
  bool? _lastImuOk;
  bool? _lastHxOk;

  bool get isRecording => _recording;
  bool? get lastCalOk => _lastCalOk;
  List<RideSample> get samples => List.unmodifiable(_samples);

  List<RideSample> get routeSamples =>
      _samples.where((s) => s.lat != null && s.lng != null).toList();

  // Latest live values — updated whenever connected, regardless of recording
  double currentPowerW = 0;
  double currentCadenceRpm = 0;
  double currentGpsSpeed = 0; // always from phone GPS
  double? currentForceN;
  double? currentTorqueNm;

  double maxPowerW = 0;
  double maxCadenceRpm = 0;
  double maxGpsSpeed = 0;
  double maxForceN = 0;

  final _updateController = StreamController<void>.broadcast();
  Stream<void> get onUpdate => _updateController.stream;

  void initLiveUpdates() {
    // Subscribe to GPS position stream for live GPS speed display.
    // GPS tracking itself is started by AppState.init() so speed is always live.
    _gpsSubscription = gpsService.positionStream.listen((pos) {
      _latestPosition = pos;
      currentGpsSpeed = (pos.speed * 3.6).clamp(0, double.infinity);
      _updateController.add(null);
    });

    _liveSubscription = bleService.packetStream.listen((packet) {
      currentPowerW = packet.powerW;
      currentCadenceRpm = packet.cadenceRpm;
      currentForceN = packet.forceN;
      currentTorqueNm = packet.torqueNm;

      if (_recording) {
        // Detect dropped packets using seq number (full-system only)
        if (packet.seq != null && _lastSeq != null) {
          final expected = (_lastSeq! + 1) & 0x7FFFFFFF;
          if (packet.seq != expected) {
            _droppedPacketCount += (packet.seq! - expected).abs();
          }
        }
        if (packet.seq != null) _lastSeq = packet.seq;

        if (packet.isFullSystem) _deviceSampleCount++;

        final lat = _latestPosition?.latitude;
        final lng = _latestPosition?.longitude;
        if (lat != null && lng != null) _gpsSampleCount++;

        if (packet.powerW > maxPowerW) maxPowerW = packet.powerW;
        if (packet.cadenceRpm > maxCadenceRpm) maxCadenceRpm = packet.cadenceRpm;
        if (currentGpsSpeed > maxGpsSpeed) maxGpsSpeed = currentGpsSpeed;
        if ((packet.forceN ?? 0) > maxForceN) maxForceN = packet.forceN ?? 0;

        _lastCalOk = packet.calOk;
        _lastHxTared = packet.hxTared;
        _lastImuOk = packet.imuOk;
        _lastHxOk = packet.hxOk;

        _samples.add(RideSample(
          timestamp: DateTime.now(),
          elapsedSec: _startTime == null
              ? 0
              : DateTime.now().difference(_startTime!).inSeconds,
          powerW: packet.powerW,
          cadenceRpm: packet.cadenceRpm,
          gpsSpeed: currentGpsSpeed,
          seq: packet.seq,
          deviceTimeMs: packet.deviceTimeMs,
          forceN: packet.forceN,
          torqueNm: packet.torqueNm,
          omegaRadS: packet.omegaRadS,
          hxRaw: packet.hxRaw,
          hxTared: packet.hxTared,
          imuOk: packet.imuOk,
          hxOk: packet.hxOk,
          calOk: packet.calOk,
          lat: lat,
          lng: lng,
        ));
      }

      _updateController.add(null);
    });
  }

  void startRecording() {
    _recording = true;
    _startTime = DateTime.now();
    _samples.clear();
    _lastSeq = null;
    _droppedPacketCount = 0;
    _deviceSampleCount = 0;
    _gpsSampleCount = 0;
    _lastCalOk = null;
    _lastHxTared = null;
    _lastImuOk = null;
    _lastHxOk = null;
    maxPowerW = 0;
    maxCadenceRpm = 0;
    maxGpsSpeed = 0;
    maxForceN = 0;

    // Reset distance accumulator for this session; GPS stream stays active.
    gpsService.startTracking();
  }

  Future<RideSummary?> stopRecording() async {
    if (!_recording || _startTime == null) return null;

    _recording = false;
    // Do not stop GPS tracking — keep it running for live speed display.

    final endTime = DateTime.now();
    final durationSec = endTime.difference(_startTime!).inSeconds;

    if (_samples.isEmpty) return null;

    final avgPower = _samples.map((s) => s.powerW).reduce((a, b) => a + b) / _samples.length;
    final avgCadence = _samples.map((s) => s.cadenceRpm).reduce((a, b) => a + b) / _samples.length;
    final avgSpeed = _samples.map((s) => s.gpsSpeed).reduce((a, b) => a + b) / _samples.length;
    final calories = avgPower * durationSec / 1000 * 0.24;

    // Optional device fields — only computed when full-system samples present
    double? avgForce, maxForce, avgTorque, maxTorque, avgOmega, maxOmega;
    final deviceSamples = _samples.where((s) => s.forceN != null).toList();
    if (deviceSamples.isNotEmpty) {
      avgForce = deviceSamples.map((s) => s.forceN!).reduce((a, b) => a + b) / deviceSamples.length;
      maxForce = deviceSamples.map((s) => s.forceN!).reduce((a, b) => a > b ? a : b);
    }
    final torqueSamples = _samples.where((s) => s.torqueNm != null).toList();
    if (torqueSamples.isNotEmpty) {
      avgTorque = torqueSamples.map((s) => s.torqueNm!).reduce((a, b) => a + b) / torqueSamples.length;
      maxTorque = torqueSamples.map((s) => s.torqueNm!).reduce((a, b) => a > b ? a : b);
    }
    final omegaSamples = _samples.where((s) => s.omegaRadS != null).toList();
    if (omegaSamples.isNotEmpty) {
      avgOmega = omegaSamples.map((s) => s.omegaRadS!).reduce((a, b) => a + b) / omegaSamples.length;
      maxOmega = omegaSamples.map((s) => s.omegaRadS!).reduce((a, b) => a > b ? a : b);
    }

    final summary = RideSummary(
      rideId: const Uuid().v4(),
      title: 'Ride ${DateTime.now().toString().substring(0, 10)}',
      startTime: _startTime!,
      endTime: endTime,
      durationSec: durationSec,
      distanceKm: gpsService.totalDistanceKm,
      avgPower: avgPower,
      maxPower: maxPowerW,
      avgCadence: avgCadence,
      maxCadence: maxCadenceRpm,
      avgSpeed: avgSpeed,
      maxSpeed: maxGpsSpeed,
      avgForce: avgForce,
      maxForce: maxForce,
      avgTorque: avgTorque,
      maxTorque: maxTorque,
      avgOmega: avgOmega,
      maxOmega: maxOmega,
      calories: calories,
      sampleCount: _samples.length,
      deviceSampleCount: _deviceSampleCount > 0 ? _deviceSampleCount : null,
      gpsSampleCount: _gpsSampleCount > 0 ? _gpsSampleCount : null,
      droppedPacketCount: _droppedPacketCount > 0 ? _droppedPacketCount : null,
      calibrationOk: _lastCalOk,
      hxTared: _lastHxTared,
      imuOk: _lastImuOk,
      hxOk: _lastHxOk,
      schemaVersion: 2,
      syncStatus: 'pending',
    );

    try {
      await firestoreService.uploadRide(summary, List.from(_samples));
      _samples.clear();
      _updateController.add(null);
      return summary.copyWith(syncStatus: 'synced');
    } catch (_) {
      _samples.clear();
      _updateController.add(null);
      return summary;
    }
  }

  void dispose() {
    _liveSubscription?.cancel();
    _gpsSubscription?.cancel();
    _updateController.close();
  }
}
