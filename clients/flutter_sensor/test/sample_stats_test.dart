import 'package:flutter_test/flutter_test.dart';
import 'package:traceforge_sensor/src/sample_stats.dart';

void main() {
  test('computes observed rate from monotonic source timestamps', () {
    final stats = SampleStats();
    final start = DateTime.utc(2026);

    stats
      ..add(start)
      ..add(start.add(const Duration(milliseconds: 20)))
      ..add(start.add(const Duration(milliseconds: 40)));

    expect(stats.count, 3);
    expect(stats.observedRateHz, closeTo(50, 0.001));
    expect(stats.nonMonotonicTimestamps, 0);
  });

  test('counts non-monotonic timestamps without moving the time window', () {
    final stats = SampleStats();
    final start = DateTime.utc(2026);

    stats
      ..add(start)
      ..add(start.add(const Duration(milliseconds: 20)))
      ..add(start.add(const Duration(milliseconds: 10)));

    expect(stats.count, 3);
    expect(stats.lastTimestamp, start.add(const Duration(milliseconds: 20)));
    expect(stats.nonMonotonicTimestamps, 1);
    expect(stats.observedRateHz, closeTo(50, 0.001));
  });
}
