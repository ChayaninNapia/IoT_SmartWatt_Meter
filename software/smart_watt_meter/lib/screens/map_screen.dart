import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:provider/provider.dart';
import '../providers/app_state.dart';
import '../models/ride_sample.dart';

class MapScreen extends StatefulWidget {
  const MapScreen({super.key});

  @override
  State<MapScreen> createState() => _MapScreenState();
}

class _MapScreenState extends State<MapScreen> {
  final _mapController = MapController();
  bool _heatmapMode = false;
  bool _followPosition = true;

  @override
  void dispose() {
    _mapController.dispose();
    super.dispose();
  }

  Color _powerColor(double power) {
    if (power < 100) return const Color(0xFF4CAF50); // Recovery — green
    if (power < 160) return const Color(0xFF2196F3); // Endurance — blue
    if (power < 220) return const Color(0xFFFFEB3B); // Tempo — yellow
    if (power < 280) return const Color(0xFFFF9800); // Threshold — orange
    return const Color(0xFFF44336);                  // VO₂Max — red
  }

  List<Polyline> _buildPolylines(List<RideSample> routeSamples) {
    if (routeSamples.isEmpty) return [];

    if (!_heatmapMode) {
      return [
        Polyline(
          points: routeSamples
              .map((s) => LatLng(s.lat!, s.lng!))
              .toList(),
          strokeWidth: 4,
          color: const Color(0xFF2979FF),
        ),
      ];
    }

    // Heatmap: one segment per consecutive pair
    final polylines = <Polyline>[];
    for (int i = 0; i < routeSamples.length - 1; i++) {
      final a = routeSamples[i];
      final b = routeSamples[i + 1];
      polylines.add(Polyline(
        points: [LatLng(a.lat!, a.lng!), LatLng(b.lat!, b.lng!)],
        strokeWidth: 5,
        color: _powerColor(a.powerW),
      ));
    }
    return polylines;
  }

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final session = app.rideSession;
    final routeSamples = session.routeSamples;
    final lastPos = app.gpsService.lastPosition;

    final center = lastPos != null
        ? LatLng(lastPos.latitude, lastPos.longitude)
        : const LatLng(13.65, 100.49); // KMUTT default

    if (_followPosition && lastPos != null) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        _mapController.move(LatLng(lastPos.latitude, lastPos.longitude), 17);
      });
    }

    return Scaffold(
      backgroundColor: const Color(0xFF0D1117),
      appBar: AppBar(
        backgroundColor: const Color(0xFF161B22),
        title: const Text('Map', style: TextStyle(color: Colors.white)),
        elevation: 0,
        actions: [
          if (session.isRecording)
            Padding(
              padding: const EdgeInsets.only(right: 8),
              child: Row(
                children: [
                  Container(
                      width: 8,
                      height: 8,
                      decoration: const BoxDecoration(
                          color: Colors.red, shape: BoxShape.circle)),
                  const SizedBox(width: 6),
                  const Text('REC',
                      style: TextStyle(color: Colors.red, fontSize: 12)),
                  const SizedBox(width: 8),
                ],
              ),
            ),
          // Heatmap toggle
          Padding(
            padding: const EdgeInsets.only(right: 8),
            child: FilterChip(
              label: Text(
                _heatmapMode ? 'Heatmap' : 'Route',
                style: TextStyle(
                  color: _heatmapMode ? Colors.black : Colors.white,
                  fontSize: 12,
                ),
              ),
              selected: _heatmapMode,
              onSelected: (v) => setState(() => _heatmapMode = v),
              selectedColor: const Color(0xFFFFEB3B),
              backgroundColor: const Color(0xFF30363D),
              checkmarkColor: Colors.black,
            ),
          ),
          // Follow toggle
          IconButton(
            icon: Icon(
              _followPosition ? Icons.gps_fixed : Icons.gps_not_fixed,
              color: _followPosition ? const Color(0xFF2979FF) : Colors.grey,
            ),
            onPressed: () => setState(() => _followPosition = !_followPosition),
            tooltip: 'Follow position',
          ),
        ],
      ),
      body: Stack(
        children: [
          FlutterMap(
            mapController: _mapController,
            options: MapOptions(
              initialCenter: center,
              initialZoom: 17,
              onPositionChanged: (_, hasGesture) {
                if (hasGesture && _followPosition) {
                  setState(() => _followPosition = false);
                }
              },
            ),
            children: [
              TileLayer(
                urlTemplate: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
                userAgentPackageName: 'com.example.smart_watt_meter',
              ),
              PolylineLayer(polylines: _buildPolylines(routeSamples)),
              if (lastPos != null)
                MarkerLayer(
                  markers: [
                    Marker(
                      point: LatLng(lastPos.latitude, lastPos.longitude),
                      width: 20,
                      height: 20,
                      child: Container(
                        decoration: BoxDecoration(
                          color: const Color(0xFF2979FF),
                          shape: BoxShape.circle,
                          border: Border.all(color: Colors.white, width: 2),
                        ),
                      ),
                    ),
                  ],
                ),
            ],
          ),

          // Power zone legend (heatmap mode only)
          if (_heatmapMode)
            Positioned(
              top: 12,
              left: 12,
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
                decoration: BoxDecoration(
                  color: const Color(0xEE161B22),
                  borderRadius: BorderRadius.circular(4),
                ),
                child: const Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    _ZoneDot(color: Color(0xFF4CAF50), label: 'Recovery <100W'),
                    _ZoneDot(color: Color(0xFF2196F3), label: 'Endurance <160W'),
                    _ZoneDot(color: Color(0xFFFFEB3B), label: 'Tempo <220W'),
                    _ZoneDot(color: Color(0xFFFF9800), label: 'Threshold <280W'),
                    _ZoneDot(color: Color(0xFFF44336), label: 'VO₂Max ≥280W'),
                  ],
                ),
              ),
            ),

          // Stats overlay
          Positioned(
            bottom: 20,
            left: 16,
            right: 16,
            child: Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: const Color(0xEE161B22),
                borderRadius: BorderRadius.circular(4),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  _OverlayStat(
                    label: 'Distance',
                    value: app.gpsService.totalDistanceKm.toStringAsFixed(2),
                    unit: 'km',
                  ),
                  _OverlayStat(
                    label: 'Power',
                    value: session.currentPowerW.toStringAsFixed(0),
                    unit: 'W',
                  ),
                  _OverlayStat(
                    label: 'GPS Speed',
                    value: session.currentGpsSpeed.toStringAsFixed(1),
                    unit: 'km/h',
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _ZoneDot extends StatelessWidget {
  final Color color;
  final String label;
  const _ZoneDot({required this.color, required this.label});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 10,
            height: 10,
            decoration: BoxDecoration(color: color, shape: BoxShape.circle),
          ),
          const SizedBox(width: 6),
          Text(label, style: const TextStyle(color: Colors.white70, fontSize: 11)),
        ],
      ),
    );
  }
}

class _OverlayStat extends StatelessWidget {
  final String label;
  final String value;
  final String unit;
  const _OverlayStat({required this.label, required this.value, required this.unit});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Text(label, style: const TextStyle(color: Colors.grey, fontSize: 11)),
        const SizedBox(height: 2),
        Text(
          '$value $unit',
          style: const TextStyle(
              color: Colors.white, fontSize: 16, fontWeight: FontWeight.bold),
        ),
      ],
    );
  }
}
