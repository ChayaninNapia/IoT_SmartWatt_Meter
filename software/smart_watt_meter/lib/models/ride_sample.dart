class RideSample {
  final DateTime timestamp;
  final int elapsedSec;

  // Unit-explicit fields (schema v2)
  final double powerW;
  final double cadenceRpm;
  final double gpsSpeed; // km/h from phone GPS — not ESP32

  // Device fields from full-system BLE packet (null when using mock firmware)
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

  // GPS
  final double? lat;
  final double? lng;

  const RideSample({
    required this.timestamp,
    required this.elapsedSec,
    required this.powerW,
    required this.cadenceRpm,
    required this.gpsSpeed,
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
    this.lat,
    this.lng,
  });

  Map<String, dynamic> toFirestore() {
    final map = <String, dynamic>{
      'timestamp': timestamp.toIso8601String(),
      'elapsedSec': elapsedSec,
      'powerW': powerW,
      'cadenceRpm': cadenceRpm,
      'gpsSpeed': gpsSpeed,
      'lat': lat,
      'lng': lng,
    };
    if (seq != null) map['seq'] = seq;
    if (deviceTimeMs != null) map['deviceTimeMs'] = deviceTimeMs;
    if (forceN != null) map['forceN'] = forceN;
    if (torqueNm != null) map['torqueNm'] = torqueNm;
    if (omegaRadS != null) map['omegaRadS'] = omegaRadS;
    if (hxRaw != null) map['hxRaw'] = hxRaw;
    if (hxTared != null) map['hxTared'] = hxTared;
    if (imuOk != null) map['imuOk'] = imuOk;
    if (hxOk != null) map['hxOk'] = hxOk;
    if (calOk != null) map['calOk'] = calOk;
    return map;
  }
}
