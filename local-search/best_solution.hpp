// AI事前作成コード公開元: https://github.com/LBJ-code/ahc-library

#pragma once

#include "direction.hpp"

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ahc {

// 最良の State と score を一組だけ保持する薄いマルチスタート用部品。
// State の生成・コピー方法や探索ループは呼出側に残す。
// score が同値のときは最初の State を残す（strict な比較）。
template <class State, class Score = double>
class BestSolution {
  public:
    using state_type = State;
    using score_type = Score;

    explicit BestSolution(const Objective objective = Objective::minimize)
        : objective_(objective) {
        if (objective_ != Objective::minimize &&
            objective_ != Objective::maximize) {
            throw std::invalid_argument("unknown best-solution objective");
        }
    }

    Objective objective() const noexcept { return objective_; }
    bool empty() const noexcept { return !state_.has_value(); }
    bool has_value() const noexcept { return state_.has_value(); }

    const State& state() const {
        if (!state_) {
            throw std::logic_error("best solution is empty");
        }
        return *state_;
    }

    const State& best_state() const { return state(); }

    const Score& score() const {
        if (!score_) {
            throw std::logic_error("best solution score is empty");
        }
        return *score_;
    }

    const Score& best_score() const { return score(); }

    // 候補が best を更新したときだけ State/score を保存し、true を返す。
    // NaN score は順序を持たないため候補として無視する。
    bool consider(const State& candidate_state, const Score& candidate_score) {
        if (!valid_score(candidate_score) ||
            (score_ && !is_better(candidate_score, *score_))) {
            return false;
        }
        state_ = candidate_state;
        score_ = candidate_score;
        return true;
    }

    bool consider(State&& candidate_state, const Score& candidate_score) {
        if (!valid_score(candidate_score) ||
            (score_ && !is_better(candidate_score, *score_))) {
            return false;
        }
        state_ = std::move(candidate_state);
        score_ = candidate_score;
        return true;
    }

    void clear() noexcept {
        state_.reset();
        score_.reset();
    }

    void reset() noexcept { clear(); }

  private:
    static bool valid_score(const Score& score) noexcept {
        if constexpr (std::is_floating_point_v<Score>) {
            return score == score;
        }
        return true;
    }

    bool is_better(const Score& lhs, const Score& rhs) const {
        return objective_ == Objective::minimize ? lhs < rhs : lhs > rhs;
    }

    Objective objective_;
    std::optional<State> state_;
    std::optional<Score> score_;
};

}  // namespace ahc
