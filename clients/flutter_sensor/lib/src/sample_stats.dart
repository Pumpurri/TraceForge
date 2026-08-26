class SampleStats {
  int get count => _count;
  int _count = 0;
  int _monotonicCount = 0;

  DateTime? get firstTimestamp => _firstTimestamp;
  DateTime? _firstTimestamp;

  DateTime? get lastTimestamp => _lastTimestamp;
  DateTime? _lastTimestamp;

  int get nonMonotonicTimestamps => _nonMonotonicTimestamps;
  int _nonMonotonicTimestamps = 0;

  double get observedRateHz {
    final first = _firstTimestamp;
    final last = _lastTimestamp;
    if (_monotonicCount < 2 || first == null || last == null) {
      return 0;
    }

    final elapsedMicroseconds = last.difference(first).inMicroseconds;
    if (elapsedMicroseconds <= 0) {
      return 0;
    }

    return (_monotonicCount - 1) *
        Duration.microsecondsPerSecond /
        elapsedMicroseconds;
  }

  void add(DateTime timestamp) {
    _count += 1;
    final previousTimestamp = _lastTimestamp;
    if (previousTimestamp != null && !timestamp.isAfter(previousTimestamp)) {
      _nonMonotonicTimestamps += 1;
      return;
    }

    _firstTimestamp ??= timestamp;
    _lastTimestamp = timestamp;
    _monotonicCount += 1;
  }
}
