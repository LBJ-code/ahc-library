// AI事前作成コード公開元: https://github.com/LBJ-code/ahc-library

#pragma once

#include "direction.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>

namespace ahc {

namespace detail {

// 不正な進捗率を温度計算に伝播させない。NaN は未開始、±∞は端点とする。
inline double clamp_progress(const double value) noexcept {
    if (std::isnan(value)) {
        return 0.0;
    }
    if (value <= 0.0) {
        return 0.0;
    }
    if (value >= 1.0) {
        return 1.0;
    }
    return value;
}

inline void validate_linear_temperature(const double start,
                                        const double end) {
    if (!std::isfinite(start) || !std::isfinite(end) || start < 0.0 ||
        end < 0.0) {
        throw std::invalid_argument(
            "linear temperature must be finite and non-negative");
    }
}

inline void validate_exponential_temperature(const double start,
                                             const double end) {
    if (!std::isfinite(start) || !std::isfinite(end) || start <= 0.0 ||
        end <= 0.0) {
        throw std::invalid_argument(
            "exponential temperature must be finite and positive");
    }
}

template <class URBG>
bool accept_worse(const double worsening, const double temperature, URBG& rng) {
    // temperature <= 0, NaN, -∞ は「貪欲」に扱う。改善・同値は呼出側で
    // 先に返しているため、ここへ来る値は必ず悪化側である。
    if (!(temperature > 0.0)) {
        return false;
    }

    // +∞温度では exp(-worsening / T) = 1 とする。
    if (std::isinf(temperature)) {
        return true;
    }

    // exp() を呼ばず、確率の対数と一様乱数の対数を比較する。worsening
    // が非常に大きい場合も underflow/overflow を受理判定へ持ち込まない。
    const double log_probability = -worsening / temperature;
    if (std::isnan(log_probability)) {
        return false;
    }
    if (log_probability >= 0.0) {
        return true;
    }

    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double sample = uniform(rng);
    if (!(sample > 0.0) || sample >= 1.0) {
        return false;
    }

    // sample == 0/1 や NaN は上で除外した。確率0を誤って通さないよう、
    // 対数値は厳密比較にする。
    return std::log(sample) < log_probability;
}

}  // namespace detail

// 0 <= step / total <= 1 の進捗率。整数・浮動小数を混ぜても呼べる。
// total == 0 は「完了」とみなし 1.0 を返す。NaN や無効な制限時間は完了扱い、
// 正の無限大の制限時間は有限の経過時間なら未開始扱いにする。
template <class Step, class Total>
    requires(std::is_arithmetic_v<Step> && std::is_arithmetic_v<Total>)
inline double progress(const Step step, const Total total) noexcept {
    const long double elapsed = static_cast<long double>(step);
    const long double limit = static_cast<long double>(total);
    if (std::isnan(elapsed) || std::isnan(limit) || limit <= 0.0L) {
        return 1.0;
    }
    if (elapsed <= 0.0L) {
        return 0.0;
    }
    if (elapsed >= limit) {
        return 1.0;
    }
    if (std::isinf(limit)) {
        return 0.0;
    }
    return static_cast<double>(elapsed / limit);
}

// 開始温度から終了温度へ直線的に変化するスケジュール。
class LinearTemperature {
  public:
    LinearTemperature(const double start, const double end)
        : start_(start), end_(end) {
        detail::validate_linear_temperature(start, end);
    }

    double operator()(const double ratio) const noexcept {
        const double p = detail::clamp_progress(ratio);
        if (p == 0.0) {
            return start_;
        }
        if (p == 1.0) {
            return end_;
        }
        // start,end は非負有限なので、この補間は区間外へ発散しない。
        return start_ + (end_ - start_) * p;
    }

    double at(const double ratio) const noexcept { return (*this)(ratio); }
    double temperature(const double ratio) const noexcept { return (*this)(ratio); }
    double start() const noexcept { return start_; }
    double end() const noexcept { return end_; }
    double initial() const noexcept { return start_; }
    double final() const noexcept { return end_; }

  private:
    double start_;
    double end_;
};

// 開始温度から終了温度へ指数的に変化するスケジュール。
// 両端は 0 より大きい有限値でなければならない。
class ExponentialTemperature {
  public:
    ExponentialTemperature(const double start, const double end)
        : start_(start), end_(end), log_start_(0.0), log_end_(0.0) {
        detail::validate_exponential_temperature(start, end);
        log_start_ = std::log(start_);
        log_end_ = std::log(end_);
    }

    double operator()(const double ratio) const noexcept {
        const double p = detail::clamp_progress(ratio);
        if (p == 0.0) {
            return start_;
        }
        if (p == 1.0) {
            return end_;
        }

        // 直接 start * pow(end / start, p) とせず、log 空間で補間する。
        const double value = std::exp(log_start_ + (log_end_ - log_start_) * p);
        // 丸めによるわずかな端数だけを有限範囲へ収める。通常の値では
        // clamp の分岐に入らない。
        if (!std::isfinite(value)) {
            return std::numeric_limits<double>::max();
        }
        return value;
    }

    double at(const double ratio) const noexcept { return (*this)(ratio); }
    double temperature(const double ratio) const noexcept { return (*this)(ratio); }
    double start() const noexcept { return start_; }
    double end() const noexcept { return end_; }
    double initial() const noexcept { return start_; }
    double final() const noexcept { return end_; }

  private:
    double start_;
    double end_;
    double log_start_;
    double log_end_;
};

using LinearTemperatureSchedule = LinearTemperature;
using ExponentialTemperatureSchedule = ExponentialTemperature;

inline double linear_temperature(const double start,
                                 const double end,
                                 const double ratio) {
    return LinearTemperature(start, end)(ratio);
}

inline double exponential_temperature(const double start,
                                      const double end,
                                      const double ratio) {
    return ExponentialTemperature(start, end)(ratio);
}

// delta は「候補 score - 現在 score」。この符号を常に同じにすることで、
// minimize/maximize の取り違えを避ける。
// 改善・同値は必ず受理し、悪化だけ exp(-worsening / temperature) で受理する。
template <class URBG>
bool accept(const Objective objective,
            const double delta,
            const double temperature,
            URBG& rng) {
    if (std::isnan(delta)) {
        return false;
    }

    const bool better = objective == Objective::minimize ? delta < 0.0
                                                          : delta > 0.0;
    const bool equal = delta == 0.0;
    if (better || equal) {
        return true;
    }

    const double worsening = objective == Objective::minimize ? delta : -delta;
    return detail::accept_worse(worsening, temperature, rng);
}

template <class URBG>
bool accept_move(const Objective objective,
                 const double delta,
                 const double temperature,
                 URBG& rng) {
    return accept(objective, delta, temperature, rng);
}

template <class URBG>
bool accept_delta(const Objective objective,
                  const double delta,
                  const double temperature,
                  URBG& rng) {
    return accept(objective, delta, temperature, rng);
}

template <class URBG>
bool accept_minimize(const double delta,
                     const double temperature,
                     URBG& rng) {
    return accept(Objective::minimize, delta, temperature, rng);
}

template <class URBG>
bool accept_maximize(const double delta,
                     const double temperature,
                     URBG& rng) {
    return accept(Objective::maximize, delta, temperature, rng);
}

// score の差をこの関数側で計算する版。最小ループで現在値と候補値を
// 直接持っている場合に使える。
template <class URBG>
bool accept_candidate(const Objective objective,
                      const double current_score,
                      const double candidate_score,
                      const double temperature,
                      URBG& rng) {
    if (std::isnan(current_score) || std::isnan(candidate_score)) {
        return false;
    }
    if (objective == Objective::minimize) {
        if (candidate_score <= current_score) {
            return true;
        }
        return detail::accept_worse(candidate_score - current_score,
                                     temperature, rng);
    }
    if (candidate_score >= current_score) {
        return true;
    }
    return detail::accept_worse(current_score - candidate_score, temperature,
                                rng);
}

template <class URBG>
bool accept_minimize_candidate(const double current_score,
                               const double candidate_score,
                               const double temperature,
                               URBG& rng) {
    return accept_candidate(Objective::minimize, current_score, candidate_score,
                            temperature, rng);
}

template <class URBG>
bool accept_maximize_candidate(const double current_score,
                               const double candidate_score,
                               const double temperature,
                               URBG& rng) {
    return accept_candidate(Objective::maximize, current_score, candidate_score,
                            temperature, rng);
}

}  // namespace ahc
