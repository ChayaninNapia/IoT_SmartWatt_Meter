class RideSummary {
  final String rideId;
  final String title;
  final DateTime startTime;
  final DateTime endTime;
  final int durationSec;
  final double distanceKm;

  // Power / cadence / speed
  final double avgPower;
  final double maxPower;
  final double avgCadence;
  final double maxCadence;
  final double avgSpeed;
  final double maxSpeed;

  // Force / torque / angular velocity (null for v1 / mock rides)
  final double? avgForce;
  final double? maxForce;
  final double? avgTorque;
  final double? maxTorque;
  final double? avgOmega;
  final double? maxOmega;

  final double calories;
  final int sampleCount;
  final int? deviceSampleCount;
  final int? gpsSampleCount;
  final int? droppedPacketCount;

  // Final sensor/calibration status (null for v1 / mock rides)
  final bool? calibrationOk;
  final bool? hxTared;
  final bool? imuOk;
  final bool? hxOk;

  final int schemaVersion;
  final String syncStatus;

  const RideSummary({
    required this.rideId,
    required this.title,
    required this.startTime,
    required this.endTime,
    required this.durationSec,
    required this.distanceKm,
    required this.avgPower,
    required this.maxPower,
    required this.avgCadence,
    required this.maxCadence,
    required this.avgSpeed,
    required this.maxSpeed,
    this.avgForce,
    this.maxForce,
    this.avgTorque,
    this.maxTorque,
    this.avgOmega,
    this.maxOmega,
    required this.calories,
    required this.sampleCount,
    this.deviceSampleCount,
    this.gpsSampleCount,
    this.droppedPacketCount,
    this.calibrationOk,
    this.hxTared,
    this.imuOk,
    this.hxOk,
    this.schemaVersion = 2,
    required this.syncStatus,
  });

  RideSummary copyWith({String? syncStatus}) => RideSummary(
        rideId: rideId,
        title: title,
        startTime: startTime,
        endTime: endTime,
        durationSec: durationSec,
        distanceKm: distanceKm,
        avgPower: avgPower,
        maxPower: maxPower,
        avgCadence: avgCadence,
        maxCadence: maxCadence,
        avgSpeed: avgSpeed,
        maxSpeed: maxSpeed,
        avgForce: avgForce,
        maxForce: maxForce,
        avgTorque: avgTorque,
        maxTorque: maxTorque,
        avgOmega: avgOmega,
        maxOmega: maxOmega,
        calories: calories,
        sampleCount: sampleCount,
        deviceSampleCount: deviceSampleCount,
        gpsSampleCount: gpsSampleCount,
        droppedPacketCount: droppedPacketCount,
        calibrationOk: calibrationOk,
        hxTared: hxTared,
        imuOk: imuOk,
        hxOk: hxOk,
        schemaVersion: schemaVersion,
        syncStatus: syncStatus ?? this.syncStatus,
      );

  Map<String, dynamic> toFirestore() {
    final map = <String, dynamic>{
      'schemaVersion': schemaVersion,
      'title': title,
      'startTime': startTime.toIso8601String(),
      'endTime': endTime.toIso8601String(),
      'durationSec': durationSec,
      'distanceKm': distanceKm,
      'avgPower': avgPower,
      'maxPower': maxPower,
      'avgCadence': avgCadence,
      'maxCadence': maxCadence,
      'avgSpeed': avgSpeed,
      'maxSpeed': maxSpeed,
      'calories': calories,
      'sampleCount': sampleCount,
      'uploadMode': 'auto',
      'syncStatus': syncStatus,
      'createdAt': DateTime.now().toIso8601String(),
    };
    if (avgForce != null) map['avgForce'] = avgForce;
    if (maxForce != null) map['maxForce'] = maxForce;
    if (avgTorque != null) map['avgTorque'] = avgTorque;
    if (maxTorque != null) map['maxTorque'] = maxTorque;
    if (avgOmega != null) map['avgOmega'] = avgOmega;
    if (maxOmega != null) map['maxOmega'] = maxOmega;
    if (deviceSampleCount != null) map['deviceSampleCount'] = deviceSampleCount;
    if (gpsSampleCount != null) map['gpsSampleCount'] = gpsSampleCount;
    if (droppedPacketCount != null) map['droppedPacketCount'] = droppedPacketCount;
    if (calibrationOk != null) map['calibrationOk'] = calibrationOk;
    if (hxTared != null) map['hxTared'] = hxTared;
    if (imuOk != null) map['imuOk'] = imuOk;
    if (hxOk != null) map['hxOk'] = hxOk;
    return map;
  }

  factory RideSummary.fromFirestore(String id, Map<String, dynamic> data) {
    return RideSummary(
      rideId: id,
      title: data['title'] ?? '',
      startTime: DateTime.parse(data['startTime']),
      endTime: DateTime.parse(data['endTime']),
      durationSec: data['durationSec'] ?? 0,
      distanceKm: (data['distanceKm'] ?? 0).toDouble(),
      avgPower: (data['avgPower'] ?? 0).toDouble(),
      maxPower: (data['maxPower'] ?? 0).toDouble(),
      avgCadence: (data['avgCadence'] ?? 0).toDouble(),
      maxCadence: (data['maxCadence'] ?? 0).toDouble(),
      avgSpeed: (data['avgSpeed'] ?? 0).toDouble(),
      maxSpeed: (data['maxSpeed'] ?? 0).toDouble(),
      avgForce: data['avgForce'] != null ? (data['avgForce'] as num).toDouble() : null,
      maxForce: data['maxForce'] != null ? (data['maxForce'] as num).toDouble() : null,
      avgTorque: data['avgTorque'] != null ? (data['avgTorque'] as num).toDouble() : null,
      maxTorque: data['maxTorque'] != null ? (data['maxTorque'] as num).toDouble() : null,
      avgOmega: data['avgOmega'] != null ? (data['avgOmega'] as num).toDouble() : null,
      maxOmega: data['maxOmega'] != null ? (data['maxOmega'] as num).toDouble() : null,
      calories: (data['calories'] ?? 0).toDouble(),
      sampleCount: data['sampleCount'] ?? 0,
      deviceSampleCount: data['deviceSampleCount'] as int?,
      gpsSampleCount: data['gpsSampleCount'] as int?,
      droppedPacketCount: data['droppedPacketCount'] as int?,
      calibrationOk: data['calibrationOk'] as bool?,
      hxTared: data['hxTared'] as bool?,
      imuOk: data['imuOk'] as bool?,
      hxOk: data['hxOk'] as bool?,
      schemaVersion: (data['schemaVersion'] ?? 1) as int,
      syncStatus: 'synced',
    );
  }
}
