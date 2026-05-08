import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

const String _deviceName = 'ESP32_PowerMeter_01';
const String _serviceUuid = '12345678-1234-1234-1234-123456789012';
const String _telemetryCharUuid = '12345678-1234-1234-1234-123456789013';
const String _appStatusCharUuid = '12345678-1234-1234-1234-123456789014';
const Duration _scanTimeout = Duration(seconds: 10);
const Duration _connectFlowTimeout = Duration(seconds: 15);
const Duration _appStatusInterval = Duration(seconds: 1);

// ---------------------------------------------------------------------------
// BlePacket — parsed telemetry from ESP32
// ---------------------------------------------------------------------------

class BlePacket {
  final double powerW;
  final double cadenceRpm;

  // Device fields — null when coming from legacy mock firmware
  final int? seq;
  final int? deviceTimeMs;
  final double? forceN;
  final double? torqueNm;
  final double? omegaRadS;
  final int? hxRaw;
  final bool? hxTared;
  final bool? imuOk;
  final bool? hxOk;
  final bool? calOk;

  // true  → full-system v2 telemetry packet
  // false → legacy mock packet
  final bool isFullSystem;

  const BlePacket({
    required this.powerW,
    required this.cadenceRpm,
    this.seq,
    this.deviceTimeMs,
    this.forceN,
    this.torqueNm,
    this.omegaRadS,
    this.hxRaw,
    this.hxTared,
    this.imuOk,
    this.hxOk,
    this.calOk,
    required this.isFullSystem,
  });

  // Returns null if the JSON should not be treated as a telemetry packet.
  // Specifically returns null for event packets so they are never converted
  // into zero-power fake telemetry.
  static BlePacket? tryFromJson(Map<String, dynamic> json) {
    final type = json['type'] as String?;

    // Event packets must never become telemetry — return null so the caller
    // can route them to the event stream instead.
    if (type == 'event') return null;

    // Full-system telemetry: detected by presence of the unit-explicit powerW field.
    if (json.containsKey('powerW')) {
      return BlePacket(
        powerW: (json['powerW'] as num? ?? 0).toDouble(),
        cadenceRpm: (json['cadenceRpm'] as num? ?? 0).toDouble(),
        seq: json['seq'] as int?,
        deviceTimeMs: json['tMs'] as int?,
        forceN: (json['forceN'] as num?)?.toDouble(),
        torqueNm: (json['torqueNm'] as num?)?.toDouble(),
        omegaRadS: (json['omegaRadS'] as num?)?.toDouble(),
        hxRaw: json['hxRaw'] as int?,
        hxTared: json['hxTared'] as bool?,
        imuOk: json['imuOk'] as bool?,
        hxOk: json['hxOk'] as bool?,
        calOk: json['calOk'] as bool?,
        isFullSystem: true,
      );
    }

    // Legacy mock packet: {"power":245,"cadence":85,"speed":28.5,"elapsedSec":42}
    // mock "speed" is deprecated — not stored, not displayed.
    if (json.containsKey('power') || json.containsKey('cadence')) {
      return BlePacket(
        powerW: (json['power'] as num? ?? 0).toDouble(),
        cadenceRpm: (json['cadence'] as num? ?? 0).toDouble(),
        isFullSystem: false,
      );
    }

    // Unrecognised packet — ignore safely.
    return null;
  }
}

// ---------------------------------------------------------------------------
// BleEvent — event packet sent by ESP32 on the telemetry characteristic
// ---------------------------------------------------------------------------

class BleEvent {
  final String event;
  final int? seq;
  final int? deviceTimeMs;

  const BleEvent({required this.event, this.seq, this.deviceTimeMs});

  static BleEvent? tryFromJson(Map<String, dynamic> json) {
    if (json['type'] != 'event') return null;
    final event = json['event'] as String?;
    if (event == null) return null;
    return BleEvent(
      event: event,
      seq: json['seq'] as int?,
      deviceTimeMs: json['tMs'] as int?,
    );
  }
}

// ---------------------------------------------------------------------------
// BleService
// ---------------------------------------------------------------------------

class BleService {
  BluetoothDevice? _device;
  BluetoothCharacteristic? _telemetryCharacteristic;
  BluetoothCharacteristic? _appStatusCharacteristic;

  StreamSubscription? _notifySubscription;
  StreamSubscription? _scanSubscription;
  StreamSubscription<BluetoothConnectionState>? _connectionSubscription;
  Timer? _appStatusTimer;

  bool _isConnecting = false;

  final _packetController = StreamController<BlePacket>.broadcast();
  final _statusController = StreamController<String>.broadcast();
  final _eventController = StreamController<BleEvent>.broadcast();

  Stream<BlePacket> get packetStream => _packetController.stream;
  Stream<String> get statusStream => _statusController.stream;

  // Event stream: start_request, stop_request, tare_ok, gyro_bias_ok, etc.
  Stream<BleEvent> get eventStream => _eventController.stream;

  bool get isConnected => _device != null;
  bool get canWrite => _appStatusCharacteristic != null;
  String? get deviceId => _device?.remoteId.str;

  void _log(String message) => debugPrint('[BLE] $message');

  // -------------------------------------------------------------------------
  // Scanning
  // -------------------------------------------------------------------------

  Future<void> startScan() async {
    _log('startScan requested');
    final hasPermission = await _ensurePermissions();
    if (!hasPermission) {
      _log('permissions denied');
      _statusController.add('error: bluetooth permission denied');
      return;
    }

    _statusController.add('scanning');
    await FlutterBluePlus.stopScan();
    bool foundTarget = false;

    _scanSubscription?.cancel();
    _scanSubscription = FlutterBluePlus.scanResults.listen((results) {
      for (final r in results) {
        _log('scan result name="${r.device.platformName}" id=${r.device.remoteId.str}');
        if (r.device.platformName == _deviceName) {
          foundTarget = true;
          _log('target device found, stopping scan and connecting');
          FlutterBluePlus.stopScan();
          _connect(r.device);
          break;
        }
      }
    });

    await FlutterBluePlus.startScan(timeout: _scanTimeout);

    if (!foundTarget && !_isConnecting && _device == null) {
      _log('scan timeout: target device not found');
      _statusController.add('error: device not found');
    }
  }

  Future<bool> _ensurePermissions() async {
    if (!Platform.isAndroid) return true;
    final statuses = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.locationWhenInUse,
    ].request();
    _log('permission statuses: ${statuses.entries.map((e) => '${e.key}: ${e.value}').join(', ')}');
    return statuses.values.every((s) => s.isGranted);
  }

  Future<void> stopScan() async {
    _log('stopScan requested');
    await FlutterBluePlus.stopScan();
    _scanSubscription?.cancel();
    _statusController.add('idle');
  }

  // -------------------------------------------------------------------------
  // Connection
  // -------------------------------------------------------------------------

  Future<void> _connect(BluetoothDevice device) async {
    if (_isConnecting) {
      _log('connect ignored: already connecting');
      return;
    }
    _isConnecting = true;
    try {
      _log('connecting to ${device.platformName} (${device.remoteId.str})');
      _statusController.add('connecting');
      await Future<void>(() async {
        await device.connect(timeout: const Duration(seconds: 10));
        _device = device;
        _listenToConnectionState(device);
        await _discoverCharacteristics(device);
      }).timeout(_connectFlowTimeout);
    } catch (e) {
      if (e is TimeoutException) {
        _log('connect timeout, forcing disconnect cleanup');
        try {
          await device.disconnect(queue: false);
        } catch (_) {}
        if (_device?.remoteId == device.remoteId) _device = null;
        _telemetryCharacteristic = null;
        _appStatusCharacteristic = null;
        await _notifySubscription?.cancel();
        _notifySubscription = null;
        _statusController.add('error: connection timeout');
      }
      _log('connect failed: $e');
      if (e is! TimeoutException) _statusController.add('error: $e');
    } finally {
      _isConnecting = false;
    }
  }

  Future<void> disconnect() async {
    _log('disconnect requested');
    _appStatusTimer?.cancel();
    _appStatusTimer = null;

    await FlutterBluePlus.stopScan();
    await _scanSubscription?.cancel();

    final device = _device;
    if (device == null) {
      _log('disconnect skipped: no active device');
      _statusController.add('disconnected');
      return;
    }

    _statusController.add('disconnecting');
    try {
      if (_telemetryCharacteristic?.isNotifying ?? false) {
        _log('disabling notifications before disconnect');
        await _telemetryCharacteristic!.setNotifyValue(false);
      }
    } catch (_) {
      _log('failed to disable notifications, continuing disconnect');
    }
    await _notifySubscription?.cancel();
    _notifySubscription = null;
    _log('calling platform disconnect for ${device.remoteId.str}');
    await device.disconnect(queue: false);
  }

  void _listenToConnectionState(BluetoothDevice device) {
    _connectionSubscription?.cancel();
    _connectionSubscription = device.connectionState.listen((state) {
      _log('connection state changed: ${state.name}');
      switch (state) {
        case BluetoothConnectionState.connected:
          _isConnecting = false;
          _statusController.add('connected');
        case BluetoothConnectionState.disconnected:
          _isConnecting = false;
          _appStatusTimer?.cancel();
          _appStatusTimer = null;
          _notifySubscription?.cancel();
          _notifySubscription = null;
          _telemetryCharacteristic = null;
          _appStatusCharacteristic = null;
          if (_device?.remoteId == device.remoteId) _device = null;
          _statusController.add('disconnected');
        default:
          _statusController.add(state.name);
      }
    });
  }

  // -------------------------------------------------------------------------
  // Service / characteristic discovery
  // -------------------------------------------------------------------------

  Future<void> _discoverCharacteristics(BluetoothDevice device) async {
    _log('discovering services');
    final services = await device.discoverServices();
    for (final svc in services) {
      _log('service discovered: ${svc.uuid}');
      if (svc.uuid.toString().toLowerCase() != _serviceUuid.toLowerCase()) continue;

      for (final char in svc.characteristics) {
        final uuid = char.uuid.toString().toLowerCase();
        _log('characteristic discovered: $uuid');

        if (uuid == _telemetryCharUuid.toLowerCase()) {
          _telemetryCharacteristic = char;
          _log('enabling notifications on telemetry characteristic');
          await char.setNotifyValue(true);
          _notifySubscription = char.lastValueStream.listen(_onTelemetryData);
        }

        if (uuid == _appStatusCharUuid.toLowerCase()) {
          _appStatusCharacteristic = char;
          _log('app status / command write characteristic found');
        }
      }
    }

    if (_appStatusCharacteristic == null) {
      _log('WARNING: app status characteristic ...014 not found — OLED sync disabled');
    }
  }

  // -------------------------------------------------------------------------
  // Telemetry + event parsing
  // -------------------------------------------------------------------------

  void _onTelemetryData(List<int> value) {
    if (value.isEmpty) return;
    final raw = utf8.decode(value);
    try {
      final json = jsonDecode(raw) as Map<String, dynamic>;
      _log('packet received: $raw');

      // Try event first
      final event = BleEvent.tryFromJson(json);
      if (event != null) {
        _log('event received: ${event.event}');
        _eventController.add(event);
        return; // Do not feed event into packet stream
      }

      // Try telemetry
      final packet = BlePacket.tryFromJson(json);
      if (packet != null) {
        _packetController.add(packet);
      } else {
        _log('unrecognised packet ignored: $raw');
      }
    } catch (e) {
      _log('packet parse failed: $e  raw=$raw');
    }
  }

  // -------------------------------------------------------------------------
  // App status write (OLED top bar sync)
  // -------------------------------------------------------------------------

  Future<void> sendAppStatus({required bool gpsOk, required bool recording}) async {
    final char = _appStatusCharacteristic;
    if (char == null) return;
    final payload = '{"type":"appStatus","gpsOk":${gpsOk ? 'true' : 'false'},'
        '"recording":${recording ? 'true' : 'false'}}';
    _log('sendAppStatus: $payload');
    try {
      await char.write(utf8.encode(payload), withoutResponse: true);
    } catch (e) {
      _log('sendAppStatus failed: $e');
    }
  }

  // Starts a periodic 1 s timer that keeps the ESP32 OLED GPS/REC indicators
  // alive. The caller supplies a callback that returns current state.
  void startAppStatusHeartbeat({
    required bool Function() gpsOkGetter,
    required bool Function() recordingGetter,
  }) {
    _appStatusTimer?.cancel();
    _appStatusTimer = Timer.periodic(_appStatusInterval, (_) {
      sendAppStatus(gpsOk: gpsOkGetter(), recording: recordingGetter());
    });
  }

  void stopAppStatusHeartbeat() {
    _appStatusTimer?.cancel();
    _appStatusTimer = null;
  }

  // -------------------------------------------------------------------------
  // Command helpers
  // -------------------------------------------------------------------------

  Future<void> sendTare() => _writeCommand('tare');
  Future<void> sendGyroBias() => _writeCommand('gyroBias');
  Future<void> sendAxisCal() => _writeCommand('axisCal');

  Future<void> _writeCommand(String cmd) async {
    final char = _appStatusCharacteristic;
    if (char == null) {
      _log('cannot send command "$cmd": no app status characteristic');
      return;
    }
    final payload = '{"type":"command","cmd":"$cmd"}';
    _log('sendCommand: $payload');
    try {
      await char.write(utf8.encode(payload), withoutResponse: true);
    } catch (e) {
      _log('sendCommand failed: $e');
    }
  }

  // -------------------------------------------------------------------------
  // Dispose
  // -------------------------------------------------------------------------

  void dispose() {
    _appStatusTimer?.cancel();
    _notifySubscription?.cancel();
    _scanSubscription?.cancel();
    _connectionSubscription?.cancel();
    _packetController.close();
    _statusController.close();
    _eventController.close();
  }
}
