import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:intl/intl.dart';
import 'package:latlong2/latlong.dart';
import '../models/ride_sample.dart';
import '../models/ride_summary.dart';

class RideDetailScreen extends StatefulWidget {
  final RideSummary ride;
  final List<RideSample> samples;

  const RideDetailScreen({
    super.key,
    required this.ride,
    required this.samples,
  });

  @override
  State<RideDetailScreen> createState() => _RideDetailScreenState();
}

class _RideDetailScreenState extends State<RideDetailScreen> {
  bool _heatmapMode = true;

  List<RideSample> get _routeSamples =>
      widget.samples.where((s) => s.lat != null && s.lng != null).toList();

  Color _powerColor(double power) {
    if (power < 100) return const Color(0xFF4CAF50);
    if (power < 160) return const Color(0xFF2196F3);
    if (power < 220) return const Color(0xFFFFEB3B);
    if (power < 280) return const Color(0xFFFF9800);
    return const Color(0xFFF44336);
  }

  List<Polyline> _buildPolylines() {
    final route = _routeSamples;
    if (route.isEmpty) return [];

    if (!_heatmapMode) {
      return [
        Polyline(
          points: route.map((s) => LatLng(s.lat!, s.lng!)).toList(),
          strokeWidth: 4,
          color: const Color(0xFF2979FF),
        ),
      ];
    }

    final polylines = <Polyline>[];
    for (int i = 0; i < route.length - 1; i++) {
      final a = route[i];
      final b = route[i + 1];
      polylines.add(Polyline(
        points: [LatLng(a.lat!, a.lng!), LatLng(b.lat!, b.lng!)],
        strokeWidth: 5,
        color: _powerColor(a.powerW),
      ));
    }
    return polylines;
  }

  LatLng? get _mapCenter {
    final route = _routeSamples;
    if (route.isEmpty) return null;
    final mid = route[route.length ~/ 2];
    return LatLng(mid.lat!, mid.lng!);
  }

  String _formatDuration(int sec) {
    final h = sec ~/ 3600;
    final m = (sec % 3600) ~/ 60;
    final s = sec % 60;
    if (h > 0) return '${h}h ${m}m';
    return '${m}m ${s}s';
  }

  @override
  Widget build(BuildContext context) {
    final ride = widget.ride;
    final center = _mapCenter ?? const LatLng(13.65, 100.49);
    final hasGps = _routeSamples.isNotEmpty;

    return Scaffold(
      backgroundColor: const Color(0xFF0D1117),
      appBar: AppBar(
        backgroundColor: const Color(0xFF161B22),
        title: Text(ride.title, style: const TextStyle(color: Colors.white)),
        iconTheme: const IconThemeData(color: Colors.white),
        elevation: 0,
        actions: [
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
              onSelected: hasGps ? (v) => setState(() => _heatmapMode = v) : null,
              selectedColor: const Color(0xFFFFEB3B),
              backgroundColor: const Color(0xFF30363D),
              checkmarkColor: Colors.black,
            ),
          ),
        ],
      ),
      body: Column(
        children: [
          // Map
          SizedBox(
            height: 280,
            child: hasGps
                ? Stack(
                    children: [
                      FlutterMap(
                        options: MapOptions(
                          initialCenter: center,
                          initialZoom: 15,
                        ),
                        children: [
                          TileLayer(
                            urlTemplate:
                                'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
                            userAgentPackageName:
                                'com.example.smart_watt_meter',
                          ),
                          PolylineLayer(polylines: _buildPolylines()),
                          MarkerLayer(
                            markers: [
                              // Start marker
                              Marker(
                                point: LatLng(
                                    _routeSamples.first.lat!,
                                    _routeSamples.first.lng!),
                                width: 20,
                                height: 20,
                                child: Container(
                                  decoration: BoxDecoration(
                                    color: Colors.green,
                                    shape: BoxShape.circle,
                                    border: Border.all(
                                        color: Colors.white, width: 2),
                                  ),
                                ),
                              ),
                              // End marker
                              Marker(
                                point: LatLng(
                                    _routeSamples.last.lat!,
                                    _routeSamples.last.lng!),
                                width: 20,
                                height: 20,
                                child: Container(
                                  decoration: BoxDecoration(
                                    color: Colors.red,
                                    shape: BoxShape.circle,
                                    border: Border.all(
                                        color: Colors.white, width: 2),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                      if (_heatmapMode)
                        Positioned(
                          top: 8,
                          left: 8,
                          child: Container(
                            padding: const EdgeInsets.symmetric(
                                horizontal: 8, vertical: 6),
                            decoration: BoxDecoration(
                              color: const Color(0xEE161B22),
                              borderRadius: BorderRadius.circular(8),
                            ),
                            child: const Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                _ZoneDot(color: Color(0xFF4CAF50), label: '<100W'),
                                _ZoneDot(color: Color(0xFF2196F3), label: '<160W'),
                                _ZoneDot(color: Color(0xFFFFEB3B), label: '<220W'),
                                _ZoneDot(color: Color(0xFFFF9800), label: '<280W'),
                                _ZoneDot(color: Color(0xFFF44336), label: '≥280W'),
                              ],
                            ),
                          ),
                        ),
                    ],
                  )
                : Container(
                    color: const Color(0xFF1C2128),
                    child: const Center(
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Icon(Icons.map_outlined, size: 48, color: Colors.grey),
                          SizedBox(height: 8),
                          Text('No GPS data for this ride',
                              style: TextStyle(color: Colors.grey, fontSize: 13)),
                        ],
                      ),
                    ),
                  ),
          ),

          // Stats
          Expanded(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(20),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    DateFormat('EEEE, d MMMM yyyy  HH:mm').format(ride.startTime),
                    style: const TextStyle(color: Colors.grey, fontSize: 13),
                  ),
                  const SizedBox(height: 20),
                  _statGrid(ride),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _statGrid(RideSummary ride) {
    final stats = [
      ('Distance', '${ride.distanceKm.toStringAsFixed(2)} km'),
      ('Duration', _formatDuration(ride.durationSec)),
      ('Avg Power', '${ride.avgPower.toStringAsFixed(0)} W'),
      ('Max Power', '${ride.maxPower.toStringAsFixed(0)} W'),
      ('Avg Cadence', '${ride.avgCadence.toStringAsFixed(0)} rpm'),
      ('Max Cadence', '${ride.maxCadence.toStringAsFixed(0)} rpm'),
      ('Avg GPS Speed', '${ride.avgSpeed.toStringAsFixed(1)} km/h'),
      ('Max GPS Speed', '${ride.maxSpeed.toStringAsFixed(1)} km/h'),
      if (ride.avgForce != null)
        ('Avg Force', '${ride.avgForce!.toStringAsFixed(1)} N'),
      if (ride.maxForce != null)
        ('Max Force', '${ride.maxForce!.toStringAsFixed(1)} N'),
      if (ride.avgTorque != null)
        ('Avg Torque', '${ride.avgTorque!.toStringAsFixed(2)} Nm'),
      if (ride.maxTorque != null)
        ('Max Torque', '${ride.maxTorque!.toStringAsFixed(2)} Nm'),
      ('Calories', '${ride.calories.toStringAsFixed(0)} kcal'),
      ('Samples', '${ride.sampleCount}'),
      if (ride.droppedPacketCount != null && ride.droppedPacketCount! > 0)
        ('Dropped BLE', '${ride.droppedPacketCount}'),
    ];

    return GridView.count(
      crossAxisCount: 2,
      shrinkWrap: true,
      physics: const NeverScrollableScrollPhysics(),
      crossAxisSpacing: 12,
      mainAxisSpacing: 12,
      childAspectRatio: 2.4,
      children: stats
          .map((e) => Container(
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                decoration: BoxDecoration(
                  color: const Color(0xFF161B22),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(color: Colors.white10),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(e.$1,
                        style: const TextStyle(color: Colors.grey, fontSize: 11)),
                    const SizedBox(height: 2),
                    Text(e.$2,
                        style: const TextStyle(
                            color: Colors.white,
                            fontSize: 15,
                            fontWeight: FontWeight.bold)),
                  ],
                ),
              ))
          .toList(),
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
            width: 8,
            height: 8,
            decoration: BoxDecoration(color: color, shape: BoxShape.circle),
          ),
          const SizedBox(width: 5),
          Text(label,
              style: const TextStyle(color: Colors.white70, fontSize: 10)),
        ],
      ),
    );
  }
}
