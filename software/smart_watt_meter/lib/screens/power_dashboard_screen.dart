import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/app_state.dart';
import '../widgets/power_chart.dart';

class PowerDashboardScreen extends StatefulWidget {
  const PowerDashboardScreen({super.key});

  @override
  State<PowerDashboardScreen> createState() => _PowerDashboardScreenState();
}

class _PowerDashboardScreenState extends State<PowerDashboardScreen> {
  bool _isStopping = false;

  Color _powerZoneColor(double power) {
    if (power < 100) return const Color(0xFF58A6FF);
    if (power < 160) return const Color(0xFF3FB950);
    if (power < 220) return const Color(0xFFD29922);
    if (power < 280) return const Color(0xFFF78166);
    return const Color(0xFFFF4E4E);
  }

  String _powerZoneName(double power) {
    if (power < 100) return 'Recovery';
    if (power < 160) return 'Endurance';
    if (power < 220) return 'Tempo';
    if (power < 280) return 'Threshold';
    return 'VO₂Max';
  }

  Future<void> _handleStop(AppState app) async {
    setState(() => _isStopping = true);
    await app.stopRide();
    if (mounted) setState(() => _isStopping = false);
  }

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final session = app.rideSession;
    final isRecording = session.isRecording;
    final isConnected = app.bleStatus == 'connected';
    final zoneColor = _powerZoneColor(session.currentPowerW);

    final avgPower = session.samples.isEmpty
        ? 0.0
        : session.samples.map((s) => s.powerW).reduce((a, b) => a + b) /
            session.samples.length;

    return Scaffold(
      backgroundColor: const Color(0xFF0D1117),
      appBar: AppBar(
        backgroundColor: const Color(0xFF161B22),
        title: const Text('Power', style: TextStyle(color: Colors.white)),
        elevation: 0,
        actions: [
          if (isRecording)
            Padding(
              padding: const EdgeInsets.only(right: 16),
              child: Row(
                children: [
                  Container(
                    width: 8,
                    height: 8,
                    decoration: const BoxDecoration(
                      color: Colors.red,
                      shape: BoxShape.circle,
                    ),
                  ),
                  const SizedBox(width: 6),
                  const Text('REC',
                      style: TextStyle(color: Colors.red, fontSize: 12)),
                ],
              ),
            ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            // Big power display
            Container(
              width: double.infinity,
              padding: const EdgeInsets.symmetric(vertical: 28),
              decoration: BoxDecoration(
                color: const Color(0xFF161B22),
                borderRadius: BorderRadius.circular(6),
                border: Border.all(
                  color: zoneColor.withValues(alpha: 0.5),
                  width: 1.5,
                ),
              ),
              child: Column(
                children: [
                  Text(
                    session.currentPowerW.toStringAsFixed(0),
                    style: TextStyle(
                      fontSize: 80,
                      fontWeight: FontWeight.bold,
                      color: zoneColor,
                      height: 1,
                    ),
                  ),
                  const Text('watts',
                      style: TextStyle(color: Colors.grey, fontSize: 16)),
                  const SizedBox(height: 8),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Container(
                        padding: const EdgeInsets.symmetric(
                            horizontal: 12, vertical: 4),
                        decoration: BoxDecoration(
                          color: zoneColor.withValues(alpha: 0.15),
                          borderRadius: BorderRadius.circular(4),
                        ),
                        child: Text(
                          _powerZoneName(session.currentPowerW),
                          style: TextStyle(
                            color: zoneColor,
                            fontSize: 13,
                            fontWeight: FontWeight.w600,
                            letterSpacing: 1.2,
                          ),
                        ),
                      ),
                      if (session.lastCalOk != null) ...[
                        const SizedBox(width: 8),
                        Container(
                          padding: const EdgeInsets.symmetric(
                              horizontal: 8, vertical: 4),
                          decoration: BoxDecoration(
                            color: (session.lastCalOk!
                                    ? const Color(0xFF238636)
                                    : const Color(0xFF9E3B00))
                                .withValues(alpha: 0.25),
                            borderRadius: BorderRadius.circular(4),
                            border: Border.all(
                              color: session.lastCalOk!
                                  ? const Color(0xFF238636)
                                  : const Color(0xFFD29922),
                              width: 1,
                            ),
                          ),
                          child: Text(
                            session.lastCalOk! ? 'CAL OK' : 'CAL --',
                            style: TextStyle(
                              color: session.lastCalOk!
                                  ? const Color(0xFF3FB950)
                                  : const Color(0xFFD29922),
                              fontSize: 10,
                              fontWeight: FontWeight.w600,
                              letterSpacing: 0.8,
                            ),
                          ),
                        ),
                      ],
                    ],
                  ),
                ],
              ),
            ),

            const SizedBox(height: 12),

            // Stats row 1
            Row(
              children: [
                _StatCard(
                  label: 'Cadence',
                  value: session.currentCadenceRpm.toStringAsFixed(0),
                  unit: 'rpm',
                  color: const Color(0xFF79C0FF),
                ),
                const SizedBox(width: 10),
                _StatCard(
                  label: 'GPS Speed',
                  value: session.currentGpsSpeed.toStringAsFixed(1),
                  unit: 'km/h',
                  color: const Color(0xFF56D364),
                ),
              ],
            ),

            const SizedBox(height: 10),

            // Stats row 2
            Row(
              children: [
                _StatCard(
                  label: 'Avg Power',
                  value: avgPower == 0 ? '—' : avgPower.toStringAsFixed(0),
                  unit: 'W',
                  color: Colors.grey,
                ),
                const SizedBox(width: 10),
                _StatCard(
                  label: 'Peak Power',
                  value: session.maxPowerW.toStringAsFixed(0),
                  unit: 'W',
                  color: const Color(0xFFFF7B72),
                ),
              ],
            ),

            const SizedBox(height: 14),

            // Chart label
            const Align(
              alignment: Alignment.centerLeft,
              child: Text(
                'Last 60 seconds',
                style: TextStyle(color: Colors.grey, fontSize: 11,
                    letterSpacing: 0.5),
              ),
            ),
            const SizedBox(height: 6),
            PowerChart(samples: session.samples),

            const Spacer(),

            // Start / Stop button
            SizedBox(
              width: double.infinity,
              height: 52,
              child: isRecording
                  ? ElevatedButton(
                      onPressed:
                          _isStopping ? null : () => _handleStop(app),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.redAccent,
                        foregroundColor: Colors.white,
                        disabledBackgroundColor:
                            Colors.redAccent.withValues(alpha: 0.6),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(4),
                        ),
                      ),
                      child: _isStopping
                          ? const Row(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                SizedBox(
                                  width: 18,
                                  height: 18,
                                  child: CircularProgressIndicator(
                                    strokeWidth: 2,
                                    color: Colors.white,
                                  ),
                                ),
                                SizedBox(width: 10),
                                Text('Saving...'),
                              ],
                            )
                          : const Row(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Icon(Icons.stop_circle_outlined),
                                SizedBox(width: 8),
                                Text('Stop Session'),
                              ],
                            ),
                    )
                  : ElevatedButton.icon(
                      onPressed: isConnected ? app.startRide : null,
                      icon: const Icon(Icons.play_circle_outline),
                      label: Text(isConnected
                          ? 'Start Session'
                          : 'Connect Device First'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: const Color(0xFF238636),
                        foregroundColor: Colors.white,
                        disabledBackgroundColor: Colors.grey.shade800,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(4),
                        ),
                      ),
                    ),
            ),
          ],
        ),
      ),
    );
  }
}

class _StatCard extends StatelessWidget {
  final String label;
  final String value;
  final String unit;
  final Color color;

  const _StatCard({
    required this.label,
    required this.value,
    required this.unit,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: Container(
        padding:
            const EdgeInsets.symmetric(vertical: 12, horizontal: 14),
        decoration: BoxDecoration(
          color: const Color(0xFF161B22),
          borderRadius: BorderRadius.circular(4),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(label,
                style: const TextStyle(
                    color: Colors.grey,
                    fontSize: 11,
                    letterSpacing: 0.5)),
            const SizedBox(height: 4),
            RichText(
              text: TextSpan(
                children: [
                  TextSpan(
                    text: value,
                    style: TextStyle(
                      color: color,
                      fontSize: 26,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  TextSpan(
                    text: ' $unit',
                    style: const TextStyle(
                        color: Colors.grey, fontSize: 12),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
