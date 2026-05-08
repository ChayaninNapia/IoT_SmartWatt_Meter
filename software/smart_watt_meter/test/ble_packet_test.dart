import 'package:flutter_test/flutter_test.dart';
import 'package:smart_watt_meter/services/ble_service.dart';

void main() {
  group('BlePacket.tryFromJson', () {
    test('parses full-system v2 telemetry packet', () {
      final json = {
        'v': 1,
        'type': 'telemetry',
        'seq': 128,
        'tMs': 128420,
        'powerW': 245.3,
        'cadenceRpm': 82.1,
        'forceN': 34.2,
        'torqueNm': 5.9,
        'omegaRadS': 8.6,
        'hxRaw': 29120,
        'hxTared': true,
        'imuOk': true,
        'hxOk': true,
        'calOk': true,
      };

      final packet = BlePacket.tryFromJson(json);

      expect(packet, isNotNull);
      expect(packet!.isFullSystem, isTrue);
      expect(packet.powerW, closeTo(245.3, 0.01));
      expect(packet.cadenceRpm, closeTo(82.1, 0.01));
      expect(packet.seq, 128);
      expect(packet.deviceTimeMs, 128420);
      expect(packet.forceN, closeTo(34.2, 0.01));
      expect(packet.torqueNm, closeTo(5.9, 0.01));
      expect(packet.omegaRadS, closeTo(8.6, 0.01));
      expect(packet.hxRaw, 29120);
      expect(packet.hxTared, isTrue);
      expect(packet.imuOk, isTrue);
      expect(packet.hxOk, isTrue);
      expect(packet.calOk, isTrue);
    });

    test('parses legacy mock packet and maps power/cadence', () {
      final json = {
        'power': 200,
        'cadence': 75,
        'speed': 28.5,
        'elapsedSec': 42,
      };

      final packet = BlePacket.tryFromJson(json);

      expect(packet, isNotNull);
      expect(packet!.isFullSystem, isFalse);
      expect(packet.powerW, closeTo(200.0, 0.01));
      expect(packet.cadenceRpm, closeTo(75.0, 0.01));
      // Device-specific fields must all be null for mock packets
      expect(packet.forceN, isNull);
      expect(packet.torqueNm, isNull);
      expect(packet.omegaRadS, isNull);
      expect(packet.hxRaw, isNull);
      expect(packet.calOk, isNull);
      expect(packet.seq, isNull);
    });

    test('event packet returns null — not converted to fake telemetry', () {
      final json = {
        'v': 1,
        'type': 'event',
        'seq': 44,
        'tMs': 55200,
        'event': 'start_request',
      };

      final packet = BlePacket.tryFromJson(json);

      expect(packet, isNull,
          reason: 'event packets must not become BlePacket telemetry');
    });

    test('stop_request event packet also returns null', () {
      final json = {
        'v': 1,
        'type': 'event',
        'seq': 45,
        'tMs': 55300,
        'event': 'stop_request',
      };

      expect(BlePacket.tryFromJson(json), isNull);
    });

    test('tare_ok event packet returns null', () {
      final json = {
        'v': 1,
        'type': 'event',
        'seq': 46,
        'tMs': 55400,
        'event': 'tare_ok',
      };

      expect(BlePacket.tryFromJson(json), isNull);
    });

    test('unrecognised packet returns null safely', () {
      final json = {'foo': 'bar'};
      expect(BlePacket.tryFromJson(json), isNull);
    });

    test('full-system packet with calOk false is still parsed', () {
      final json = {
        'v': 1,
        'type': 'telemetry',
        'seq': 1,
        'tMs': 1000,
        'powerW': 0.0,
        'cadenceRpm': 0.0,
        'hxTared': false,
        'imuOk': true,
        'hxOk': false,
        'calOk': false,
      };

      final packet = BlePacket.tryFromJson(json);

      expect(packet, isNotNull);
      expect(packet!.isFullSystem, isTrue);
      expect(packet.calOk, isFalse,
          reason: 'calOk=false must be parsed — telemetry still shown for debug');
    });
  });

  group('BleEvent.tryFromJson', () {
    test('parses start_request event', () {
      final json = {
        'v': 1,
        'type': 'event',
        'seq': 44,
        'tMs': 55200,
        'event': 'start_request',
      };

      final event = BleEvent.tryFromJson(json);

      expect(event, isNotNull);
      expect(event!.event, 'start_request');
      expect(event.seq, 44);
      expect(event.deviceTimeMs, 55200);
    });

    test('parses stop_request event', () {
      final json = {
        'v': 1,
        'type': 'event',
        'seq': 45,
        'tMs': 55300,
        'event': 'stop_request',
      };

      final event = BleEvent.tryFromJson(json);

      expect(event, isNotNull);
      expect(event!.event, 'stop_request');
    });

    test('returns null for telemetry packet', () {
      final json = {'type': 'telemetry', 'powerW': 100.0, 'cadenceRpm': 80.0};
      expect(BleEvent.tryFromJson(json), isNull);
    });

    test('returns null when event field missing', () {
      final json = {'type': 'event', 'seq': 1};
      expect(BleEvent.tryFromJson(json), isNull);
    });
  });
}
