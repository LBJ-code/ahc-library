// AI pre-created code: https://github.com/LBJ-code/ahc-library
// Header-only weighted sampling with non-negative weights and O(log n) draws.
#ifndef AHC_UTILITIES_WEIGHTED_SAMPLER_HPP
#define AHC_UTILITIES_WEIGHTED_SAMPLER_HPP

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>
#include <random>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace ahc {

// A small Fenwick tree.  `prefix_sum(k)` is the sum of [0, k), while
// `lower_bound(target)` returns the first index whose cumulative sum is
// strictly greater than target.  Consequently a target in [0, total) always
// maps to a valid index, including when some entries have zero weight.
template<class Value = long double>
requires std::is_arithmetic_v<Value> &&
         (!std::same_as<std::remove_cv_t<Value>, bool>)
class FenwickTree {
public:
    using value_type = Value;
    using size_type = std::size_t;

    FenwickTree() = default;
    explicit FenwickTree(size_type size) : tree_(size + 1, Value{}) {}

    [[nodiscard]] size_type size() const noexcept {
        return tree_.empty() ? 0 : tree_.size() - 1;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    void assign(size_type size) {
        tree_.assign(size + 1, Value{});
    }

    // Build in O(n) from a sequence of point values.
    template<std::ranges::input_range Range>
    bool build(const Range& values) {
        std::vector<Value> copied;
        for (const auto& value : values) {
            copied.push_back(static_cast<Value>(value));
        }
        tree_.assign(copied.size() + 1, Value{});
        for (size_type i = 0; i < copied.size(); ++i) {
            const size_type node = i + 1;
            tree_[node] += copied[i];
            // Fenwick parent = node + lowbit(node).  Using node itself here
            // would only be correct for powers of two and corrupts all other
            // prefix sums during O(n) construction.
            const size_type parent = node + (node & (~node + 1));
            if (parent < tree_.size()) {
                tree_[parent] += tree_[node];
            }
        }
        return true;
    }

    [[nodiscard]] Value at(size_type index) const noexcept {
        if (index >= size()) {
            return Value{};
        }
        return prefix_sum(index + 1) - prefix_sum(index);
    }

    // Add delta to one point.  Invalid indices are ignored and reported as
    // false; callers can therefore use this class without unchecked writes.
    bool add(size_type index, Value delta) {
        if (index >= size()) {
            return false;
        }
        for (size_type i = index + 1; i < tree_.size(); i += i & (~i + 1)) {
            tree_[i] += delta;
        }
        return true;
    }

    [[nodiscard]] Value prefix_sum(size_type end) const noexcept {
        end = std::min(end, size());
        Value result{};
        for (size_type i = end; i != 0; i &= i - 1) {
            result += tree_[i];
        }
        return result;
    }

    [[nodiscard]] Value range_sum(size_type first, size_type last) const noexcept {
        if (first > last) {
            return Value{};
        }
        first = std::min(first, size());
        last = std::min(last, size());
        if (first > last) {
            return Value{};
        }
        return prefix_sum(last) - prefix_sum(first);
    }

    [[nodiscard]] Value total() const noexcept {
        return prefix_sum(size());
    }

    [[nodiscard]] size_type lower_bound(Value target) const noexcept {
        const Value sum = total();
        if (!(target >= Value{}) || !(target < sum)) {
            return target < Value{} ? 0 : size();
        }

        size_type bit = 1;
        while ((bit << 1) <= size() && (bit << 1) > bit) {
            bit <<= 1;
        }
        size_type index = 0;
        Value accumulated{};
        while (bit != 0) {
            const size_type next = index + bit;
            if (next <= size() && accumulated + tree_[next] <= target) {
                index = next;
                accumulated += tree_[next];
            }
            bit >>= 1;
        }
        // target < total guarantees index < size, but min keeps this method
        // safe even for a tree containing unusual floating-point roundoff.
        return std::min(index, size() - 1);
    }

private:
    std::vector<Value> tree_;
};

template<class Weight = double>
requires std::is_arithmetic_v<Weight> &&
         (!std::same_as<std::remove_cv_t<Weight>, bool>)
class WeightedSampler {
public:
    using weight_type = Weight;
    using value_type = long double;
    using size_type = std::size_t;
    using result_type = std::optional<size_type>;
    using unit_random_type = std::function<long double()>;

    WeightedSampler() = default;

    explicit WeightedSampler(size_type size)
        : weights_(size, Weight{}), tree_(size) {}

    WeightedSampler(size_type size, std::uint64_t seed)
        : weights_(size, Weight{}), tree_(size), engine_(seed) {}

    WeightedSampler(std::initializer_list<Weight> values) {
        if (!assign(values)) {
            throw std::invalid_argument("WeightedSampler weights must be finite and non-negative");
        }
    }

    WeightedSampler(
        std::initializer_list<Weight> values,
        unit_random_type random_unit
    ) : random_unit_(std::move(random_unit)) {
        if (!assign(values)) {
            throw std::invalid_argument("WeightedSampler weights must be finite and non-negative");
        }
    }

    template<std::ranges::input_range Range>
    explicit WeightedSampler(const Range& values) {
        if (!assign(values)) {
            throw std::invalid_argument("WeightedSampler weights must be finite and non-negative");
        }
    }

    template<std::ranges::input_range Range>
    WeightedSampler(const Range& values, unit_random_type random_unit)
        : random_unit_(std::move(random_unit)) {
        if (!assign(values)) {
            throw std::invalid_argument("WeightedSampler weights must be finite and non-negative");
        }
    }

    [[nodiscard]] size_type size() const noexcept {
        return weights_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return weights_.empty();
    }

    [[nodiscard]] Weight weight(size_type index) const noexcept {
        if (index >= weights_.size()) {
            return Weight{};
        }
        return weights_[index];
    }

    [[nodiscard]] std::span<const Weight> weights() const noexcept {
        return weights_;
    }

    [[nodiscard]] value_type total_weight() const noexcept {
        return total_;
    }

    [[nodiscard]] value_type total() const noexcept {
        return total_weight();
    }

    template<std::ranges::input_range Range>
    bool assign(const Range& values) {
        std::vector<Weight> copied;
        for (const auto& value : values) {
            Weight converted_value{};
            if (!convert_weight(value, converted_value)) {
                return false;
            }
            copied.push_back(converted_value);
        }

        std::vector<value_type> converted;
        converted.reserve(copied.size());
        value_type new_total{};
        for (const Weight value : copied) {
            const value_type converted_value = static_cast<value_type>(value);
            new_total += converted_value;
            if (!std::isfinite(new_total)) {
                return false;
            }
            converted.push_back(converted_value);
        }

        FenwickTree<value_type> new_tree;
        new_tree.build(converted);
        weights_ = std::move(copied);
        tree_ = std::move(new_tree);
        total_ = new_total;
        return true;
    }

    bool set(size_type index, Weight new_weight) {
        if (index >= weights_.size() || !valid_weight(new_weight)) {
            return false;
        }
        const value_type converted = static_cast<value_type>(new_weight);
        const value_type new_total = total_ - static_cast<value_type>(weights_[index]) + converted;
        if (!std::isfinite(new_total)) {
            return false;
        }
        const value_type delta = converted - static_cast<value_type>(weights_[index]);
        weights_[index] = new_weight;
        tree_.add(index, delta);
        total_ = new_total;
        return true;
    }

    bool update(size_type index, Weight new_weight) {
        return set(index, new_weight);
    }

    bool set_weight(size_type index, Weight new_weight) {
        return set(index, new_weight);
    }

    bool update_weight(size_type index, Weight new_weight) {
        return set(index, new_weight);
    }

    template<class Delta>
    requires std::is_arithmetic_v<std::remove_cvref_t<Delta>>
    bool add(size_type index, Delta delta) {
        if (index >= weights_.size()) {
            return false;
        }
        const value_type candidate = static_cast<value_type>(weights_[index]) +
                                     static_cast<value_type>(delta);
        if (!std::isfinite(candidate) || candidate < value_type{}) {
            return false;
        }
        if constexpr (std::is_integral_v<Weight>) {
            if (candidate > static_cast<value_type>(std::numeric_limits<Weight>::max())) {
                return false;
            }
        }
        return set(index, static_cast<Weight>(candidate));
    }

    template<class Delta>
    requires std::is_arithmetic_v<std::remove_cvref_t<Delta>>
    bool increment(size_type index, Delta delta) {
        return add(index, delta);
    }

    bool push_back(Weight new_weight) {
        if (!valid_weight(new_weight)) {
            return false;
        }
        const value_type converted = static_cast<value_type>(new_weight);
        if (!std::isfinite(total_ + converted)) {
            return false;
        }
        weights_.push_back(new_weight);
        // Appending changes the Fenwick shape; rebuilding remains linear and
        // is used only for this setup-style operation.  Sampling and updates
        // remain O(log n).
        std::vector<value_type> converted_weights;
        converted_weights.reserve(weights_.size());
        for (const Weight value : weights_) {
            converted_weights.push_back(static_cast<value_type>(value));
        }
        tree_.build(converted_weights);
        total_ += converted;
        return true;
    }

    bool append(Weight new_weight) {
        return push_back(new_weight);
    }

    void clear() noexcept {
        weights_.clear();
        tree_.assign(0);
        total_ = value_type{};
    }

    void set_random_unit(unit_random_type random_unit) {
        random_unit_ = std::move(random_unit);
    }

    void clear_random_unit() {
        random_unit_ = {};
    }

    // Draw using the injected [0, 1] source when present, otherwise the
    // sampler's private engine.  A zero total returns nullopt.
    [[nodiscard]] result_type sample() const {
        if (random_unit_) {
            return sample_unit(random_unit_());
        }
        std::uniform_real_distribution<value_type> distribution(value_type{0}, value_type{1});
        return sample_unit(distribution(engine_));
    }

    // A standard uniform random bit generator (mt19937, pcg, etc.) is sampled
    // with a uniform real distribution.  A plain callable returning a number
    // in [0, 1] is accepted as an injected unit source as well.
    template<class RandomSource>
    [[nodiscard]] result_type sample(RandomSource& source) const {
        if constexpr (std::uniform_random_bit_generator<RandomSource>) {
            std::uniform_real_distribution<value_type> distribution(value_type{0}, value_type{1});
            return sample_unit(distribution(source));
        } else if constexpr (std::invocable<RandomSource&>) {
            return sample_unit(static_cast<value_type>(std::invoke(source)));
        } else {
            static_assert(std::uniform_random_bit_generator<RandomSource>,
                          "sample(source) expects a uniform random bit generator or a callable returning [0, 1]");
        }
    }

    template<class RandomSource>
    requires (!std::is_lvalue_reference_v<RandomSource>) &&
             std::invocable<RandomSource&>
    [[nodiscard]] result_type sample(RandomSource&& source) const {
        if constexpr (std::uniform_random_bit_generator<std::remove_cvref_t<RandomSource>>) {
            std::uniform_real_distribution<value_type> distribution(value_type{0}, value_type{1});
            return sample_unit(distribution(source));
        } else {
            return sample_with(std::forward<RandomSource>(source));
        }
    }

    template<class RandomSource>
    [[nodiscard]] result_type sample_with(RandomSource&& source) const {
        return sample_unit(static_cast<value_type>(std::invoke(std::forward<RandomSource>(source))));
    }

    [[nodiscard]] result_type sample_index() const {
        return sample();
    }

    template<class RandomSource>
    [[nodiscard]] result_type sample_index(RandomSource& source) const {
        return sample(source);
    }

private:
    template<class Value>
    [[nodiscard]] static bool valid_weight(Value value) noexcept {
        using value_type_without_cv = std::remove_cv_t<Value>;
        if constexpr (!std::is_arithmetic_v<value_type_without_cv>) {
            return false;
        } else {
            if constexpr (std::is_floating_point_v<value_type_without_cv>) {
                if (!std::isfinite(value)) {
                    return false;
                }
            }
            if constexpr (std::is_signed_v<value_type_without_cv>) {
                return value >= Value{};
            } else {
                return true;
            }
        }
    }

    template<class Value>
    [[nodiscard]] static bool convert_weight(Value value, Weight& destination) noexcept {
        if (!valid_weight(value)) {
            return false;
        }
        const value_type converted = static_cast<value_type>(value);
        if (!std::isfinite(converted)) {
            return false;
        }
        if (converted > static_cast<value_type>(std::numeric_limits<Weight>::max())) {
            return false;
        }
        if constexpr (std::is_signed_v<Weight>) {
            if (converted < static_cast<value_type>(std::numeric_limits<Weight>::lowest())) {
                return false;
            }
        }
        destination = static_cast<Weight>(value);
        return true;
    }

    [[nodiscard]] result_type sample_unit(value_type unit) const {
        const value_type tree_total = tree_.total();
        if (!(tree_total > value_type{}) || !std::isfinite(tree_total)) {
            return std::nullopt;
        }
        if (std::isnan(unit)) {
            return std::nullopt;
        }
        if (unit <= value_type{}) {
            return tree_.lower_bound(value_type{});
        }
        value_type target = unit * tree_total;
        if (unit >= value_type{1} || !(target < tree_total)) {
            // lower_bound expects target < total.  Clamping the target itself
            // (rather than the unit before multiplication) also works when
            // total_ is very large and multiplication rounds up to total_.
            target = std::nextafter(tree_total, value_type{0});
        }
        const size_type index = tree_.lower_bound(target);
        if (index >= weights_.size()) {
            // With target < tree_total this indicates a malformed/overflowed
            // tree rather than a valid draw.  Do not scan linearly here: the
            // sampler's selection operation remains O(log n).
            return std::nullopt;
        }
        return index;
    }

    std::vector<Weight> weights_;
    FenwickTree<value_type> tree_;
    value_type total_ = 0;
    mutable std::mt19937_64 engine_{std::random_device{}()};
    unit_random_type random_unit_;
};

template<class Weight = double>
using WeightedRandomSampler = WeightedSampler<Weight>;

}  // namespace ahc

#endif  // AHC_UTILITIES_WEIGHTED_SAMPLER_HPP
