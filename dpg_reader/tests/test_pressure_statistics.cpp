#include "../pressure_statistics.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using dpg_statistics::PressureStatistics;
using dpg_statistics::PressureSummary;

static void check(const PressureSummary &value, float minimum, float maximum, float average) {
  assert(std::abs(value.minimum - minimum) < 0.0001f);
  assert(std::abs(value.maximum - maximum) < 0.0001f);
  assert(std::abs(value.average - average) < 0.0001f);
}

static void unknown(const PressureSummary &value) {
  assert(std::isnan(value.minimum));
  assert(std::isnan(value.maximum));
  assert(std::isnan(value.average));
}

int main() {
  PressureStatistics stats;
  std::vector<PressureSummary> reports;
  auto publish = [&](const PressureSummary &result) { reports.push_back(result); };

  // Ignore samples before the restored selector establishes the first window.
  stats.sample(1000, 50, publish);
  stats.advance(100000, publish);
  assert(reports.empty());

  // All six selectable periods produce one summary only at their deadline.
  const uint32_t periods[] = {60000U, 300000U, 600000U, 900000U, 1800000U, 3600000U};
  for (uint32_t period : periods) {
    reports.clear();
    stats.reset(1000, period);
    stats.sample(1000, -2, publish);
    stats.sample(2000, 4, publish);
    stats.sample(3000, 10, publish);
    stats.advance(1000 + period - 1, publish);
    assert(reports.empty());
    stats.advance(1000 + period, publish);
    assert(reports.size() == 1);
    check(reports.back(), -2, 10, 4);
    stats.advance(1000 + period, publish);
    assert(reports.size() == 1);  // A timer/sample at the same time cannot double-report.
  }

  // A boundary reading belongs to the next window, never the preceding one.
  reports.clear();
  stats.reset(0, 60000);
  stats.sample(0, 10, publish);
  stats.sample(59999, 20, publish);
  stats.sample(60000, 100, publish);
  assert(reports.size() == 1);
  check(reports.back(), 10, 20, 15);
  stats.advance(120000, publish);
  assert(reports.size() == 2);
  check(reports.back(), 100, 100, 100);

  // A late timer/notification still closes the old window first, without drift.
  reports.clear();
  stats.reset(0, 60000);
  stats.sample(100, 7, publish);
  stats.sample(60020, 13, publish);
  check(reports.back(), 7, 7, 7);
  stats.advance(119999, publish);
  assert(reports.size() == 1);
  stats.advance(120000, publish);
  assert(reports.size() == 2);
  check(reports.back(), 13, 13, 13);

  // Invalid values and communication gaps contribute no invented samples.
  reports.clear();
  stats.reset(0, 60000);
  stats.sample(0, -3, publish);
  stats.sample(1000, NAN, publish);
  stats.sample(2000, INFINITY, publish);
  stats.sample(3000, -INFINITY, publish);
  stats.sample(50000, 3, publish);
  stats.advance(60000, publish);
  check(reports.back(), -3, 3, 0);
  stats.advance(120000, publish);
  unknown(reports.back());
  stats.sample(120001, 0, publish);
  stats.advance(180000, publish);
  check(reports.back(), 0, 0, 0);  // Zero pressure is valid.

  // If the main loop runs late, skipped empty windows are not filled with old data.
  reports.clear();
  stats.reset(0, 60000);
  stats.sample(100, 8, publish);
  stats.advance(180100, publish);
  assert(reports.size() == 3);
  check(reports[0], 8, 8, 8);
  unknown(reports[1]);
  unknown(reports[2]);

  // HA automations may re-apply the current option: keep the active window.
  reports.clear();
  stats.reset(0, 60000);
  stats.sample(100, 2, publish);
  stats.set_period(30000, 60000);
  stats.sample(40000, 6, publish);
  stats.advance(60000, publish);
  assert(reports.size() == 1);
  check(reports.back(), 2, 6, 4);

  // Selecting a different period discards the partial window and starts a full one.
  reports.clear();
  stats.reset(0, 60000);
  stats.sample(100, 999, publish);
  stats.set_period(30000, 300000);
  stats.sample(30001, 5, publish);
  stats.advance(329999, publish);
  assert(reports.empty());
  stats.advance(330000, publish);
  assert(reports.size() == 1);
  check(reports.back(), 5, 5, 5);

  // The clock can wrap during a window (ESP32 millis() wraps roughly every 49 days).
  reports.clear();
  const uint32_t start = UINT32_MAX - 30000U;
  stats.reset(start, 60000);
  stats.sample(start, 2, publish);
  stats.sample(start + 50000U, 6, publish);
  stats.advance(start + 59999U, publish);
  assert(reports.empty());
  stats.advance(start + 60000U, publish);
  assert(reports.size() == 1);
  check(reports.back(), 2, 6, 4);

  // Accumulate the full-rate stream independently of live-report throttling.
  reports.clear();
  stats.reset(0, 60000);
  for (uint32_t i = 0; i < 600; ++i)
    stats.sample(i * 100U, static_cast<float>(i), publish);
  stats.advance(60000, publish);
  assert(reports.size() == 1);
  check(reports.back(), 0, 599, 299.5f);

  std::cout << "Passed pressure statistics tests (all periods, boundaries, gaps, reset, rollover, full-rate mean).\n";
}
