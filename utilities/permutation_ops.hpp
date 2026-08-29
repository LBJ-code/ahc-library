// AI pre-created code: https://github.com/LBJ-code/ahc-library
// Header-only neighborhood operations for permutations and random-access sequences.
#ifndef AHC_UTILITIES_PERMUTATION_OPS_HPP
#define AHC_UTILITIES_PERMUTATION_OPS_HPP

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace ahc::permutation_ops {

using index_type = std::size_t;

namespace detail {

template<class Sequence>
[[nodiscard]] constexpr index_type size_of(const Sequence& sequence) noexcept {
    using std::size;
    return static_cast<index_type>(size(sequence));
}

template<class Sequence>
[[nodiscard]] constexpr auto begin_of(Sequence& sequence) noexcept {
    using std::begin;
    return begin(sequence);
}

template<class Sequence>
[[nodiscard]] constexpr bool valid_index(const Sequence& sequence, index_type index) noexcept {
    return index < size_of(sequence);
}

}  // namespace detail

// All index-based operations below use zero-based indices.
// reverse is half-open: [first, last).  relocate moves the element at `from`
// to its final index `to` after the move.  Thus relocate(from=1, to=3) turns
// [a,b,c,d] into [a,c,d,b], and its inverse is relocate(from=3, to=1).

template<class Sequence>
[[nodiscard]] bool swap_at(Sequence& sequence, index_type first, index_type second) {
    if (!detail::valid_index(sequence, first) || !detail::valid_index(sequence, second)) {
        return false;
    }
    auto begin = detail::begin_of(sequence);
    std::iter_swap(begin + static_cast<std::ptrdiff_t>(first),
                   begin + static_cast<std::ptrdiff_t>(second));
    return true;
}

template<class Sequence>
[[nodiscard]] bool reverse_at(
    Sequence& sequence,
    index_type first,
    index_type last
) {
    const index_type size = detail::size_of(sequence);
    if (first > last || last > size) {
        return false;
    }
    auto begin = detail::begin_of(sequence);
    std::reverse(begin + static_cast<std::ptrdiff_t>(first),
                 begin + static_cast<std::ptrdiff_t>(last));
    return true;
}

template<class Sequence>
[[nodiscard]] bool relocate_at(
    Sequence& sequence,
    index_type from,
    index_type to
) {
    const index_type size = detail::size_of(sequence);
    if (from >= size || to >= size) {
        return false;
    }
    if (from == to) {
        return true;
    }

    auto begin = detail::begin_of(sequence);
    auto from_iterator = begin + static_cast<std::ptrdiff_t>(from);
    auto to_iterator = begin + static_cast<std::ptrdiff_t>(to);
    if (from < to) {
        // [from, to + 1) -> left rotation by one.  The moved value ends at to.
        std::rotate(from_iterator, from_iterator + 1, to_iterator + 1);
    } else {
        // [to, from + 1) -> right rotation by one.  The moved value ends at to.
        std::rotate(to_iterator, from_iterator, from_iterator + 1);
    }
    return true;
}

struct SwapOperation {
    index_type first = 0;
    index_type second = 0;

    constexpr SwapOperation() noexcept = default;
    constexpr SwapOperation(index_type first_value, index_type second_value) noexcept
        : first(first_value), second(second_value) {}

    friend constexpr bool operator==(const SwapOperation&, const SwapOperation&) noexcept = default;

    [[nodiscard]] constexpr SwapOperation inverse() const noexcept {
        // Swapping twice is the identity.  Keeping the original order makes
        // the inverse explicit without changing the operation's value.
        return *this;
    }

    template<class Sequence>
    [[nodiscard]] bool apply(Sequence& sequence) const {
        return swap_at(sequence, first, second);
    }

    template<class Sequence>
    [[nodiscard]] bool undo(Sequence& sequence) const {
        return inverse().apply(sequence);
    }
};

struct ReverseOperation {
    index_type first = 0;
    index_type last = 0;

    constexpr ReverseOperation() noexcept = default;
    constexpr ReverseOperation(index_type first_value, index_type last_value) noexcept
        : first(first_value), last(last_value) {}

    friend constexpr bool operator==(const ReverseOperation&, const ReverseOperation&) noexcept = default;

    [[nodiscard]] constexpr ReverseOperation inverse() const noexcept {
        return *this;
    }

    template<class Sequence>
    [[nodiscard]] bool apply(Sequence& sequence) const {
        return reverse_at(sequence, first, last);
    }

    template<class Sequence>
    [[nodiscard]] bool undo(Sequence& sequence) const {
        return inverse().apply(sequence);
    }
};

struct RelocateOperation {
    index_type from = 0;
    index_type to = 0;

    constexpr RelocateOperation() noexcept = default;
    constexpr RelocateOperation(index_type from_value, index_type to_value) noexcept
        : from(from_value), to(to_value) {}

    friend constexpr bool operator==(const RelocateOperation&, const RelocateOperation&) noexcept = default;

    [[nodiscard]] constexpr RelocateOperation inverse() const noexcept {
        return {to, from};
    }

    template<class Sequence>
    [[nodiscard]] bool apply(Sequence& sequence) const {
        return relocate_at(sequence, from, to);
    }

    template<class Sequence>
    [[nodiscard]] bool undo(Sequence& sequence) const {
        return inverse().apply(sequence);
    }
};

using Swap = SwapOperation;
using Reverse = ReverseOperation;
using Relocate = RelocateOperation;
using Operation = std::variant<SwapOperation, ReverseOperation, RelocateOperation>;

[[nodiscard]] constexpr SwapOperation swap_operation(
    index_type first,
    index_type second
) noexcept {
    return {first, second};
}

[[nodiscard]] constexpr ReverseOperation reverse_operation(
    index_type first,
    index_type last
) noexcept {
    return {first, last};
}

[[nodiscard]] constexpr RelocateOperation relocate_operation(
    index_type from,
    index_type to
) noexcept {
    return {from, to};
}

[[nodiscard]] constexpr SwapOperation make_swap(
    index_type first,
    index_type second
) noexcept {
    return swap_operation(first, second);
}

[[nodiscard]] constexpr ReverseOperation make_reverse(
    index_type first,
    index_type last
) noexcept {
    return reverse_operation(first, last);
}

[[nodiscard]] constexpr RelocateOperation make_relocate(
    index_type from,
    index_type to
) noexcept {
    return relocate_operation(from, to);
}

template<class Sequence>
[[nodiscard]] bool apply(const SwapOperation& operation, Sequence& sequence) {
    return operation.apply(sequence);
}

template<class Sequence>
[[nodiscard]] bool apply(const ReverseOperation& operation, Sequence& sequence) {
    return operation.apply(sequence);
}

template<class Sequence>
[[nodiscard]] bool apply(const RelocateOperation& operation, Sequence& sequence) {
    return operation.apply(sequence);
}

template<class Sequence>
[[nodiscard]] bool apply(const Operation& operation, Sequence& sequence) {
    return std::visit(
        [&sequence](const auto& concrete_operation) {
            return concrete_operation.apply(sequence);
        },
        operation
    );
}

template<class Sequence>
[[nodiscard]] bool swap_positions(Sequence& sequence, index_type first, index_type second) {
    return swap_at(sequence, first, second);
}

template<class Sequence>
[[nodiscard]] bool reverse_segment(Sequence& sequence, index_type first, index_type last) {
    return reverse_at(sequence, first, last);
}

template<class Sequence>
[[nodiscard]] bool relocate_element(Sequence& sequence, index_type from, index_type to) {
    return relocate_at(sequence, from, to);
}

[[nodiscard]] constexpr SwapOperation inverse(const SwapOperation& operation) noexcept {
    return operation.inverse();
}

[[nodiscard]] constexpr ReverseOperation inverse(const ReverseOperation& operation) noexcept {
    return operation.inverse();
}

[[nodiscard]] constexpr RelocateOperation inverse(const RelocateOperation& operation) noexcept {
    return operation.inverse();
}

[[nodiscard]] inline Operation inverse(const Operation& operation) {
    return std::visit(
        [](const auto& concrete_operation) -> Operation {
            return concrete_operation.inverse();
        },
        operation
    );
}

template<class Sequence, class OperationType>
[[nodiscard]] bool apply_with_inverse(
    Sequence& sequence,
    const OperationType& operation
) {
    // This helper intentionally does not mutate the operation.  Callers can
    // retain inverse(operation) if they need to undo a successful move.
    return apply(operation, sequence);
}

}  // namespace ahc::permutation_ops

namespace ahc {

// The short names are also available directly under ahc.  The nested
// namespace remains useful when a program has other operation types nearby.
using permutation_ops::Operation;
using permutation_ops::Relocate;
using permutation_ops::RelocateOperation;
using permutation_ops::Reverse;
using permutation_ops::ReverseOperation;
using permutation_ops::Swap;
using permutation_ops::SwapOperation;
using permutation_ops::apply;
using permutation_ops::apply_with_inverse;
using permutation_ops::inverse;
using permutation_ops::make_relocate;
using permutation_ops::make_reverse;
using permutation_ops::make_swap;
using permutation_ops::relocate_at;
using permutation_ops::relocate_element;
using permutation_ops::relocate_operation;
using permutation_ops::reverse_at;
using permutation_ops::reverse_segment;
using permutation_ops::reverse_operation;
using permutation_ops::swap_at;
using permutation_ops::swap_positions;
using permutation_ops::swap_operation;

}  // namespace ahc

#endif  // AHC_UTILITIES_PERMUTATION_OPS_HPP
