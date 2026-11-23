#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace dpg_statistics {

struct PressureSummary {
  float minimum;
  float maximum;
  float average;
};

// Fixed, non-overlapping windows of valid samples. No sample history or heap use.
class PressureStatistics {
 public:
  void set_period(uint32_t now_ms, uint32_t period_ms) {
    // Re-applying the selected HA option must not discard an active window.
    if (!initialized_ || period_ms != period_ms_)
      reset(now_ms, period_ms);
  }

  void reset(uint32_t now_ms, uint32_t period_ms) {
    window_start_ms_ = now_ms;
    period_ms_ = period_ms;
    initialized_ = true;
    clear_();
  }

  template<typename Publish> void advance(uint32_t now_ms, Publish publish) {
    if (!initialized_ || period_ms_ == 0)
      return;
    // Unsigned subtraction handles millis() rollover. Advancing by the period,
    // rather than by now_ms, prevents timer jitter from drifting the windows.
    while (static_cast<uint32_t>(now_ms - window_start_ms_) >= period_ms_) {
      publish(summary_());
      clear_();
      window_start_ms_ += period_ms_;
    }
  }

  template<typename Publish> void sample(uint32_t now_ms, float value, Publish publish) {
    // Close elapsed windows BEFORE adding a reading at/after their boundary.
    advance(now_ms, publish);
    if (!initialized_ || !std::isfinite(value))
      return;
    if (count_ == 0) {
      minimum_ = maximum_ = value;
    } else {
      if (value < minimum_) minimum_ = value;
      if (value > maximum_) maximum_ = value;
    }
    sum_ += static_cast<double>(value);
    ++count_;
  }

 private:
  PressureSummary summary_() const {
    if (count_ == 0) {
      const float unknown = std::numeric_limits<float>::quiet_NaN();
      return {unknown, unknown, unknown};
    }
    return {minimum_, maximum_, static_cast<float>(sum_ / count_)};
  }

  void clear_() {
    minimum_ = maximum_ = 0.0f;
    sum_ = 0.0;
    count_ = 0;
  }

  bool initialized_{false};
  uint32_t window_start_ms_{0};
  uint32_t period_ms_{0};
  uint32_t count_{0};
  float minimum_{0.0f};
  float maximum_{0.0f};
  double sum_{0.0};
};

// ESPHome inserts local includes after generated global declarations. Keeping
// the instance here avoids referring to this custom type before it is included.
inline PressureStatistics statistics;

}  // namespace dpg_statistics
