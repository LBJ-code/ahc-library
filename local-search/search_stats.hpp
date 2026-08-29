// AI事前作成コード公開元: https://github.com/LBJ-code/ahc-library

#pragma once

#include "direction.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ahc {

// 局所探索の試行数と、受理された試行数・新記録数を記録する小さな集計器。
// Score は比較・コピー可能な型（通常は double / int）を想定する。
template <class Score = double>
class SearchStats {
  public:
    using score_type = Score;
    using count_type = std::uint64_t;

    explicit SearchStats(const Objective objective = Objective::minimize)
        : objective_(objective) {
        if (objective_ != Objective::minimize &&
            objective_ != Objective::maximize) {
            throw std::invalid_argument("unknown search objective");
        }
    }

    void reset() noexcept {
        attempts_ = 0U;
        accepted_ = 0U;
        improved_ = 0U;
        best_.reset();
    }

    Objective objective() const noexcept { return objective_; }

    // 初期状態の score を記録する。これは試行数・改善数に数えない。
    // 初期状態を登録しておくと、その後の improved は「初期値を含む
    // これまでの best を更新した回数」になる。
    void seed(const Score& score) {
        validate_score(score);
        best_ = score;
    }

    void set_initial(const Score& score) { seed(score); }

    bool has_best() const noexcept { return best_.has_value(); }

    const Score& best() const {
        if (!best_) {
            throw std::logic_error("best score is not available");
        }
        return *best_;
    }

    const Score& best_score() const { return best(); }

    std::optional<Score> best_value() const { return best_; }

    std::optional<Score> best_optional() const { return best_; }

    // accepted == true のときだけ候補を現在解へ適用したとみなし、best を
    // 更新する。戻り値は今回 best が更新されたかどうか。
    bool observe(const Score& candidate, const bool accepted) {
        ++attempts_;
        if (!accepted) {
            return false;
        }
        ++accepted_;
        if (!valid_score(candidate)) {
            // NaN を best に入れない。±∞は順序を持つため許可する。
            return false;
        }
        if (!best_ || is_better(candidate, *best_)) {
            best_ = candidate;
            ++improved_;
            return true;
        }
        return false;
    }

    bool record(const Score& candidate, const bool accepted) {
        return observe(candidate, accepted);
    }

    // score を別に管理する場合の低レベル API。observe と混ぜて使っても
    // カウンタの意味は同じである。
    void record_attempt() noexcept { ++attempts_; }

    void record_acceptance() noexcept {
        ++attempts_;
        ++accepted_;
    }

    void record_attempt(const bool accepted) noexcept {
        ++attempts_;
        if (accepted) {
            ++accepted_;
        }
    }

    void record_attempt(const bool accepted, const bool improved) noexcept {
        ++attempts_;
        if (accepted) {
            ++accepted_;
        }
        if (improved) {
            ++improved_;
        }
    }

    // 現在の best との比較を行わず、明示的に改善イベントを加える。
    // accepted のカウントは増やさないので、record_attempt と併用する。
    void record_improvement() noexcept { ++improved_; }

    void record_accepted() noexcept { ++accepted_; }

    void record_improved() noexcept { ++improved_; }

    bool update_best(const Score& score) {
        validate_score(score);
        if (!best_ || is_better(score, *best_)) {
            best_ = score;
            ++improved_;
            return true;
        }
        return false;
    }

    count_type attempts() const noexcept { return attempts_; }
    count_type accepted() const noexcept { return accepted_; }
    count_type improved() const noexcept { return improved_; }

    count_type attempt_count() const noexcept { return attempts_; }
    count_type accepted_count() const noexcept { return accepted_; }
    count_type improved_count() const noexcept { return improved_; }

    double acceptance_rate() const noexcept {
        if (attempts_ == 0U) {
            return 0.0;
        }
        return static_cast<double>(accepted_) / static_cast<double>(attempts_);
    }

    double acceptance_ratio() const noexcept { return acceptance_rate(); }

    double acceptance() const noexcept { return acceptance_rate(); }

    double improvement_rate() const noexcept {
        if (attempts_ == 0U) {
            return 0.0;
        }
        return static_cast<double>(improved_) / static_cast<double>(attempts_);
    }

  private:
    static bool valid_score(const Score& score) noexcept {
        if constexpr (std::is_floating_point_v<Score>) {
            return score == score;  // NaN のみを除外する（±∞は可）。
        } else {
            return true;
        }
    }

    static void validate_score(const Score& score) {
        if (!valid_score(score)) {
            throw std::invalid_argument("score must not be NaN");
        }
    }

    bool is_better(const Score& lhs, const Score& rhs) const {
        if (objective_ == Objective::minimize) {
            return lhs < rhs;
        }
        return lhs > rhs;
    }

    Objective objective_;
    count_type attempts_ = 0U;
    count_type accepted_ = 0U;
    count_type improved_ = 0U;
    std::optional<Score> best_;
};

}  // namespace ahc
