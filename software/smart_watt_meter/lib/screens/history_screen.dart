import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:provider/provider.dart';
import '../models/ride_sample.dart';
import '../models/ride_summary.dart';
import '../providers/app_state.dart';
import 'ride_detail_screen.dart';

enum _SortField { date, duration, distance }
enum _SortOrder { asc, desc }

class HistoryScreen extends StatefulWidget {
  const HistoryScreen({super.key});

  @override
  State<HistoryScreen> createState() => _HistoryScreenState();
}

class _HistoryScreenState extends State<HistoryScreen> {
  _SortField _sortField = _SortField.date;
  _SortOrder _sortOrder = _SortOrder.desc;

  String _formatDuration(int sec) {
    final h = sec ~/ 3600;
    final m = (sec % 3600) ~/ 60;
    final s = sec % 60;
    if (h > 0) return '${h}h ${m}m';
    return '${m}m ${s}s';
  }

  List<RideSummary> _sorted(List<RideSummary> rides) {
    final list = List<RideSummary>.from(rides);
    list.sort((a, b) {
      int cmp;
      switch (_sortField) {
        case _SortField.date:
          cmp = a.startTime.compareTo(b.startTime);
        case _SortField.duration:
          cmp = a.durationSec.compareTo(b.durationSec);
        case _SortField.distance:
          cmp = a.distanceKm.compareTo(b.distanceKm);
      }
      return _sortOrder == _SortOrder.desc ? -cmp : cmp;
    });
    return list;
  }

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final rides = _sorted(app.rideHistory);

    return Scaffold(
      backgroundColor: const Color(0xFF0D1117),
      appBar: AppBar(
        backgroundColor: const Color(0xFF161B22),
        title: const Text('History', style: TextStyle(color: Colors.white)),
        elevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh, color: Colors.white70),
            onPressed: app.loadHistory,
            tooltip: 'Refresh',
          ),
        ],
      ),
      body: Column(
        children: [
          // Filter bar
          Container(
            color: const Color(0xFF161B22),
            padding:
                const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
            child: Row(
              children: [
                const Text('Sort:',
                    style: TextStyle(color: Colors.grey, fontSize: 12)),
                const SizedBox(width: 8),
                _SortChip(
                  label: 'Date',
                  selected: _sortField == _SortField.date,
                  onTap: () => setState(() {
                    if (_sortField == _SortField.date) {
                      _sortOrder = _sortOrder == _SortOrder.desc
                          ? _SortOrder.asc
                          : _SortOrder.desc;
                    } else {
                      _sortField = _SortField.date;
                      _sortOrder = _SortOrder.desc;
                    }
                  }),
                  order: _sortField == _SortField.date ? _sortOrder : null,
                ),
                const SizedBox(width: 6),
                _SortChip(
                  label: 'Duration',
                  selected: _sortField == _SortField.duration,
                  onTap: () => setState(() {
                    if (_sortField == _SortField.duration) {
                      _sortOrder = _sortOrder == _SortOrder.desc
                          ? _SortOrder.asc
                          : _SortOrder.desc;
                    } else {
                      _sortField = _SortField.duration;
                      _sortOrder = _SortOrder.desc;
                    }
                  }),
                  order:
                      _sortField == _SortField.duration ? _sortOrder : null,
                ),
                const SizedBox(width: 6),
                _SortChip(
                  label: 'Distance',
                  selected: _sortField == _SortField.distance,
                  onTap: () => setState(() {
                    if (_sortField == _SortField.distance) {
                      _sortOrder = _sortOrder == _SortOrder.desc
                          ? _SortOrder.asc
                          : _SortOrder.desc;
                    } else {
                      _sortField = _SortField.distance;
                      _sortOrder = _SortOrder.desc;
                    }
                  }),
                  order:
                      _sortField == _SortField.distance ? _sortOrder : null,
                ),
                const Spacer(),
                Text('${rides.length} rides',
                    style: const TextStyle(
                        color: Colors.grey, fontSize: 11)),
              ],
            ),
          ),

          // List
          Expanded(
            child: rides.isEmpty
                ? const Center(
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(Icons.history, size: 64, color: Colors.grey),
                        SizedBox(height: 12),
                        Text('No rides yet',
                            style: TextStyle(color: Colors.grey)),
                      ],
                    ),
                  )
                : ListView.separated(
                    padding: const EdgeInsets.all(14),
                    itemCount: rides.length,
                    separatorBuilder: (_, _) => const SizedBox(height: 8),
                    itemBuilder: (ctx, i) {
                      final ride = rides[i];
                      final isLast = app.lastSessionSamples.isNotEmpty &&
                          app.rideHistory.isNotEmpty &&
                          app.rideHistory.first.rideId == ride.rideId;
                      return _RideCard(
                        ride: ride,
                        formatDuration: _formatDuration,
                        samples: isLast ? app.lastSessionSamples : [],
                        isLastSession: isLast,
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }
}

class _SortChip extends StatelessWidget {
  final String label;
  final bool selected;
  final VoidCallback onTap;
  final _SortOrder? order;

  const _SortChip({
    required this.label,
    required this.selected,
    required this.onTap,
    this.order,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
        decoration: BoxDecoration(
          color: selected
              ? const Color(0xFF238636).withValues(alpha: 0.25)
              : const Color(0xFF30363D),
          borderRadius: BorderRadius.circular(4),
          border: Border.all(
            color: selected
                ? const Color(0xFF238636)
                : Colors.transparent,
          ),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              label,
              style: TextStyle(
                color: selected ? Colors.white : Colors.grey,
                fontSize: 11,
                fontWeight:
                    selected ? FontWeight.w600 : FontWeight.normal,
              ),
            ),
            if (selected && order != null) ...[
              const SizedBox(width: 3),
              Icon(
                order == _SortOrder.desc
                    ? Icons.arrow_downward
                    : Icons.arrow_upward,
                size: 11,
                color: Colors.white,
              ),
            ],
          ],
        ),
      ),
    );
  }
}

class _RideCard extends StatelessWidget {
  final RideSummary ride;
  final String Function(int) formatDuration;
  final List<RideSample> samples;
  final bool isLastSession;

  const _RideCard({
    required this.ride,
    required this.formatDuration,
    required this.samples,
    required this.isLastSession,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () => _showDetail(context),
      child: Container(
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          color: const Color(0xFF161B22),
          borderRadius: BorderRadius.circular(4),
          border: Border.all(color: Colors.white10),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  ride.title,
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 15,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                _SyncBadge(status: ride.syncStatus),
              ],
            ),
            const SizedBox(height: 4),
            Text(
              DateFormat('EEE, d MMM yyyy  HH:mm').format(ride.startTime),
              style: const TextStyle(color: Colors.grey, fontSize: 12),
            ),
            const SizedBox(height: 10),
            Row(
              children: [
                _MiniStat(
                    icon: Icons.route,
                    value: '${ride.distanceKm.toStringAsFixed(2)} km'),
                _MiniStat(
                    icon: Icons.timer_outlined,
                    value: formatDuration(ride.durationSec)),
                _MiniStat(
                    icon: Icons.bolt,
                    value: '${ride.avgPower.toStringAsFixed(0)} W avg'),
                _MiniStat(
                    icon: Icons.local_fire_department,
                    value: '${ride.calories.toStringAsFixed(0)} kcal'),
              ],
            ),
          ],
        ),
      ),
    );
  }

  void _showDetail(BuildContext context) {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => RideDetailScreen(
          ride: ride,
          samples: isLastSession ? samples : [],
        ),
      ),
    );
  }
}

class _MiniStat extends StatelessWidget {
  final IconData icon;
  final String value;
  const _MiniStat({required this.icon, required this.value});

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 13, color: Colors.grey),
          const SizedBox(width: 3),
          Flexible(
            child: Text(
              value,
              style: const TextStyle(color: Colors.white70, fontSize: 11),
              overflow: TextOverflow.ellipsis,
            ),
          ),
        ],
      ),
    );
  }
}

class _SyncBadge extends StatelessWidget {
  final String status;
  const _SyncBadge({required this.status});

  @override
  Widget build(BuildContext context) {
    final (color, label) = switch (status) {
      'synced' => (Colors.green, 'Synced'),
      'pending' => (Colors.orange, 'Pending'),
      _ => (Colors.grey, status),
    };
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.15),
        borderRadius: BorderRadius.circular(4),
        border: Border.all(color: color.withValues(alpha: 0.4)),
      ),
      child: Text(label, style: TextStyle(color: color, fontSize: 11)),
    );
  }
}
