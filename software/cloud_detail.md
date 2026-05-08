We already decided the Firebase/cloud design. Please follow this exactly unless there is a strong reason to change it.

Project:
- Mobile app for an IoT bicycle watt-meter
- During a ride, data is stored locally in the app
- After the ride ends, the app uploads the complete ride to Firebase
- This is post-ride upload, not realtime cloud streaming
- Current full-system device telemetry comes from ESP32 + HX711 + MPU6050 over BLE.
- Phone GPS owns speed, route, and distance. ESP32 does not send real speed.

Firebase setup already decided:
- Cloud Firestore
- Firebase Anonymous Auth
- Firestore currently in test mode

Firestore schema version:
- Current target schema: `schemaVersion = 2`
- Keep backward compatibility with older v1 documents where practical.
- Legacy v1 fields `power`, `cadence`, and `speed` may still exist in older samples, but new full-system samples should use unit-explicit fields such as `powerW`, `cadenceRpm`, and `gpsSpeed`.

Firestore schema:

users/{uid}
  fields:
    createdAt
    lastActiveAt

users/{uid}/rides/{rideId}
  fields:
    title
    startTime
    endTime
    durationSec
    distanceKm
    avgPower
    maxPower
    avgCadence
    maxCadence
    avgSpeed
    maxSpeed
    avgForce
    maxForce
    avgTorque
    maxTorque
    avgOmega
    maxOmega
    calories
    sampleCount
    deviceSampleCount
    gpsSampleCount
    droppedPacketCount
    calibrationOk
    hxTared
    imuOk
    hxOk
    schemaVersion
    uploadMode
    syncStatus
    createdAt

users/{uid}/rides/{rideId}/samples/{sampleId}
  fields:
    timestamp
    elapsedSec
    deviceTimeMs
    seq
    powerW
    cadenceRpm
    forceN
    torqueNm
    omegaRadS
    hxRaw
    hxTared
    imuOk
    hxOk
    calOk
    gpsSpeed
    lat
    lng

Important rules:
- 1 ride = 1 document
- 1 timestep sample = 1 document
- samples are uploaded only after the ride ends
- ride document stores summary
- samples subcollection stores time-series
- Keep writes simple and post-ride only; do not stream to Firestore during a ride.
- Do not store ESP32 mock `speed` as real bicycle speed. Store phone GPS speed as `gpsSpeed`.
- Do not add `batteryV` unless hardware battery sensing is actually added later.

Full-system BLE packet fields that map into samples:

```json
{
  "v": 1,
  "type": "telemetry",
  "seq": 128,
  "tMs": 128420,
  "powerW": 245.3,
  "cadenceRpm": 82.1,
  "forceN": 34.2,
  "torqueNm": 5.9,
  "omegaRadS": 8.6,
  "hxRaw": 29120,
  "hxTared": true,
  "imuOk": true,
  "hxOk": true,
  "calOk": true
}
```

Example v2 ride sample:

```json
{
  "timestamp": "2026-05-06T14:22:10.000",
  "elapsedSec": 42,
  "deviceTimeMs": 128420,
  "seq": 128,
  "powerW": 245.3,
  "cadenceRpm": 82.1,
  "forceN": 34.2,
  "torqueNm": 5.9,
  "omegaRadS": 8.6,
  "hxRaw": 29120,
  "hxTared": true,
  "imuOk": true,
  "hxOk": true,
  "calOk": true,
  "gpsSpeed": 28.5,
  "lat": 13.65,
  "lng": 100.49
}
```

Example v2 ride summary:

```json
{
  "schemaVersion": 2,
  "title": "Ride 2026-05-06",
  "startTime": "2026-05-06T14:21:28.000",
  "endTime": "2026-05-06T14:45:02.000",
  "durationSec": 1414,
  "distanceKm": 8.42,
  "avgPower": 184.5,
  "maxPower": 421.0,
  "avgCadence": 78.2,
  "maxCadence": 103.4,
  "avgSpeed": 21.4,
  "maxSpeed": 35.8,
  "avgForce": 31.6,
  "maxForce": 79.2,
  "avgTorque": 5.4,
  "maxTorque": 13.5,
  "avgOmega": 8.2,
  "maxOmega": 10.8,
  "sampleCount": 1414,
  "deviceSampleCount": 1410,
  "gpsSampleCount": 1398,
  "droppedPacketCount": 4,
  "calibrationOk": true,
  "hxTared": true,
  "imuOk": true,
  "hxOk": true,
  "uploadMode": "auto",
  "syncStatus": "synced",
  "createdAt": "2026-05-06T14:45:05.000"
}
```

Please build the Flutter-side implementation around this schema.
Keep the design simple and suitable for a 2-week course project.
