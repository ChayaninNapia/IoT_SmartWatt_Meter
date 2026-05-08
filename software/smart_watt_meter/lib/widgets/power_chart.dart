import 'package:flutter/material.dart';
import '../models/ride_sample.dart';

class PowerChart extends StatelessWidget {
  final List<RideSample> samples;
  final int windowSec;

  const PowerChart({super.key, required this.samples, this.windowSec = 60});

  @override
  Widget build(BuildContext context) {
    final recent = samples.length > windowSec
        ? samples.sublist(samples.length - windowSec)
        : samples;

    if (recent.isEmpty) {
      return Container(
        height: 120,
        decoration: BoxDecoration(
          color: const Color(0xFF161B22),
          borderRadius: BorderRadius.circular(4),
        ),
        child: const Center(
          child: Text('No data', style: TextStyle(color: Colors.grey, fontSize: 12)),
        ),
      );
    }

    final maxPow = recent.map((s) => s.powerW).reduce((a, b) => a > b ? a : b);
    final chartMax = maxPow < 100 ? 100.0 : (maxPow * 1.2);
    final topLabel = chartMax.toInt();
    final midLabel = (chartMax / 2).toInt();

    return Container(
      height: 120,
      decoration: BoxDecoration(
        color: const Color(0xFF161B22),
        borderRadius: BorderRadius.circular(4),
      ),
      padding: const EdgeInsets.only(left: 4, right: 8, top: 8, bottom: 8),
      child: Row(
        children: [
          // Y-axis labels
          SizedBox(
            width: 36,
            child: Column(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              crossAxisAlignment: CrossAxisAlignment.end,
              children: [
                Text('${topLabel}W',
                    style: const TextStyle(color: Colors.grey, fontSize: 9)),
                Text('${midLabel}W',
                    style: const TextStyle(color: Colors.grey, fontSize: 9)),
                const Text('0W',
                    style: TextStyle(color: Colors.grey, fontSize: 9)),
              ],
            ),
          ),
          const SizedBox(width: 6),
          Expanded(
            child: CustomPaint(
              painter: _LineChartPainter(samples: recent, chartMax: chartMax),
              child: const SizedBox.expand(),
            ),
          ),
        ],
      ),
    );
  }
}

class _LineChartPainter extends CustomPainter {
  final List<RideSample> samples;
  final double chartMax;

  _LineChartPainter({required this.samples, required this.chartMax});

  Color _zoneColor(double power) {
    if (power < 100) return const Color(0xFF58A6FF);
    if (power < 160) return const Color(0xFF3FB950);
    if (power < 220) return const Color(0xFFD29922);
    if (power < 280) return const Color(0xFFF78166);
    return const Color(0xFFFF4E4E);
  }

  @override
  void paint(Canvas canvas, Size size) {
    if (samples.length < 2) return;

    // Grid lines
    final gridPaint = Paint()
      ..color = Colors.white.withValues(alpha: 0.06)
      ..strokeWidth = 1;
    canvas.drawLine(Offset(0, 0), Offset(size.width, 0), gridPaint);
    canvas.drawLine(Offset(0, size.height / 2),
        Offset(size.width, size.height / 2), gridPaint);
    canvas.drawLine(
        Offset(0, size.height), Offset(size.width, size.height), gridPaint);

    // Compute points
    final points = <Offset>[];
    for (int i = 0; i < samples.length; i++) {
      final x = (i / (samples.length - 1)) * size.width;
      final y = size.height -
          (samples[i].powerW / chartMax).clamp(0.0, 1.0) * size.height;
      points.add(Offset(x, y));
    }

    final lineColor = _zoneColor(samples.last.powerW);

    // Fill under line
    final fillPath = Path()..moveTo(points.first.dx, size.height);
    for (final p in points) {
      fillPath.lineTo(p.dx, p.dy);
    }
    fillPath.lineTo(points.last.dx, size.height);
    fillPath.close();

    canvas.drawPath(
      fillPath,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            lineColor.withValues(alpha: 0.35),
            lineColor.withValues(alpha: 0.0),
          ],
        ).createShader(Rect.fromLTWH(0, 0, size.width, size.height)),
    );

    // Line
    final linePath = Path()..moveTo(points.first.dx, points.first.dy);
    for (int i = 1; i < points.length; i++) {
      linePath.lineTo(points[i].dx, points[i].dy);
    }
    canvas.drawPath(
      linePath,
      Paint()
        ..color = lineColor
        ..strokeWidth = 2
        ..style = PaintingStyle.stroke
        ..strokeCap = StrokeCap.round
        ..strokeJoin = StrokeJoin.round,
    );

    // Current value dot
    canvas.drawCircle(points.last, 4,
        Paint()..color = const Color(0xFF0D1117));
    canvas.drawCircle(
        points.last, 3, Paint()..color = lineColor);
  }

  @override
  bool shouldRepaint(_LineChartPainter old) =>
      old.samples.length != samples.length ||
      (samples.isNotEmpty &&
          old.samples.isNotEmpty &&
          old.samples.last.powerW != samples.last.powerW);
}
