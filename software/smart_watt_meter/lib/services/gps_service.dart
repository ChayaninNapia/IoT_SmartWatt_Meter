import 'dart:async';
import 'package:geolocator/geolocator.dart';

class GpsService {
  StreamSubscription<Position>? _subscription;
  final _positionController = StreamController<Position>.broadcast();

  Stream<Position> get positionStream => _positionController.stream;
  Position? _lastPosition;
  Position? get lastPosition => _lastPosition;

  double _totalDistanceKm = 0;
  double get totalDistanceKm => _totalDistanceKm;

  Future<bool> requestPermission() async {
    bool serviceEnabled = await Geolocator.isLocationServiceEnabled();
    if (!serviceEnabled) return false;

    LocationPermission permission = await Geolocator.checkPermission();
    if (permission == LocationPermission.denied) {
      permission = await Geolocator.requestPermission();
    }
    return permission == LocationPermission.whileInUse ||
        permission == LocationPermission.always;
  }

  void startTracking() {
    _totalDistanceKm = 0;
    _lastPosition = null;

    _subscription = Geolocator.getPositionStream(
      locationSettings: const LocationSettings(
        accuracy: LocationAccuracy.high,
        distanceFilter: 2,
      ),
    ).listen((pos) {
      if (_lastPosition != null) {
        final dist = Geolocator.distanceBetween(
          _lastPosition!.latitude,
          _lastPosition!.longitude,
          pos.latitude,
          pos.longitude,
        );
        _totalDistanceKm += dist / 1000;
      }
      _lastPosition = pos;
      _positionController.add(pos);
    });
  }

  void stopTracking() {
    _subscription?.cancel();
    _subscription = null;
  }

  void dispose() {
    stopTracking();
    _positionController.close();
  }
}
