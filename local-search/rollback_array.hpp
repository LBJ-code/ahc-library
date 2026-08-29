// AI事前作成コード公開元: https://github.com/LBJ-code/ahc-library

#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace ahc {

// 変更前の値を必要な間だけログに残す配列。
//
//   auto mark = a.snapshot();
//   a[index] = value;
//   a.rollback(mark);       // mark 以降だけ元に戻す
//   // または a.commit(mark); // 元に戻さず、mark を確定する
//
// snapshot は入れ子にできる。commit は指定した checkpoint だけを閉じ、
// 外側の checkpoint が残っている限り、その外側からの rollback は可能。
// 要素型 T はコピー構築・コピー代入可能である必要がある。
template <class T>
class RollbackArray {
  private:
    struct Change {
        std::size_t index;
        T old_value;
    };

    struct Checkpoint {
        std::size_t history_size;
        std::size_t serial;
    };

  public:
    class Snapshot {
      public:
        Snapshot() = default;

        explicit operator bool() const noexcept {
            return owner_ != nullptr;
        }

      private:
        friend class RollbackArray;

        Snapshot(const RollbackArray* owner,
                 const std::size_t history_size,
                 const std::size_t serial)
            : owner_(owner), history_size_(history_size), serial_(serial) {}

        const RollbackArray* owner_ = nullptr;
        std::size_t history_size_ = 0U;
        std::size_t serial_ = 0U;
    };

    using value_type = T;
    using size_type = std::size_t;
    using snapshot_type = Snapshot;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    RollbackArray() = default;

    explicit RollbackArray(const size_type size)
        : values_(size), touch_serial_(size, 0U) {}

    RollbackArray(const size_type size, const T& value)
        : values_(size, value), touch_serial_(size, 0U) {}

    RollbackArray(std::initializer_list<T> values)
        : values_(values), touch_serial_(values_.size(), 0U) {}

    explicit RollbackArray(const std::vector<T>& values)
        : values_(values), touch_serial_(values_.size(), 0U) {}

    template <class InputIt>
    RollbackArray(InputIt first, InputIt last)
        : values_(first, last), touch_serial_(values_.size(), 0U) {}

    // checkpoint を持ったままのコピーは意図しない token の共有を生むため、
    // 値だけをコピーし、取引履歴は空にする。
    RollbackArray(const RollbackArray& other)
        : values_(other.values_), touch_serial_(values_.size(), 0U) {}

    RollbackArray& operator=(const RollbackArray& other) {
        if (this != &other) {
            values_ = other.values_;
            touch_serial_.assign(values_.size(), 0U);
            history_.clear();
            checkpoints_.clear();
            next_serial_ = 1U;
        }
        return *this;
    }

    RollbackArray(RollbackArray&& other) noexcept(
        std::is_nothrow_move_constructible_v<std::vector<T>>)
        : values_(std::move(other.values_)),
          touch_serial_(std::move(other.touch_serial_)),
          history_(std::move(other.history_)),
          checkpoints_(std::move(other.checkpoints_)),
          next_serial_(other.next_serial_) {
        // 既存 token は移動元を所有者としているため、移動後には使えない。
        other.history_.clear();
        other.checkpoints_.clear();
        other.next_serial_ = 1U;
    }

    RollbackArray& operator=(RollbackArray&& other) noexcept(
        std::is_nothrow_move_assignable_v<std::vector<T>>) {
        if (this != &other) {
            values_ = std::move(other.values_);
            touch_serial_ = std::move(other.touch_serial_);
            history_ = std::move(other.history_);
            checkpoints_ = std::move(other.checkpoints_);
            next_serial_ = other.next_serial_;
            other.history_.clear();
            other.checkpoints_.clear();
            other.next_serial_ = 1U;
        }
        return *this;
    }

    size_type size() const noexcept { return values_.size(); }
    bool empty() const noexcept { return values_.empty(); }

    // operator[] は参照を返す通常の配列 API。snapshot 中は、アクセス時点の
    // 値を checkpoint ごとに最大1回だけログへコピーする（読み取りだけでも
    // ログは増えるが rollback は安全）。
    T& operator[](const size_type index) { return mutable_at(index); }
    const T& operator[](const size_type index) const { return values_.at(index); }

    T& at(const size_type index) { return mutable_at(index); }
    const T& at(const size_type index) const { return values_.at(index); }

    // 明示的な書き込み API。operator[] より意図が明確で、差分ログも1回だけ。
    void set(const size_type index, const T& value) {
        mutable_at(index) = value;
    }

    void fill(const T& value) {
        for (size_type index = 0U; index < values_.size(); ++index) {
            mutable_at(index) = value;
        }
    }

    // 非 const の走査・data 取得は、内部のどの要素が変更されるか分からない
    // ため、checkpoint 中は全要素を先にログへ入れる。
    iterator begin() {
        touch_all();
        return values_.begin();
    }
    iterator end() {
        touch_all();
        return values_.end();
    }
    const_iterator begin() const noexcept { return values_.begin(); }
    const_iterator end() const noexcept { return values_.end(); }
    const_iterator cbegin() const noexcept { return values_.cbegin(); }
    const_iterator cend() const noexcept { return values_.cend(); }

    T* data() {
        touch_all();
        return values_.data();
    }
    const T* data() const noexcept { return values_.data(); }

    Snapshot snapshot() {
        const Snapshot token{this, history_.size(), next_serial_++};
        checkpoints_.push_back({token.history_size_, token.serial_});
        return token;
    }

    Snapshot checkpoint() { return snapshot(); }

    bool is_active(const Snapshot& token) const noexcept {
        if (token.owner_ != this) {
            return false;
        }
        return find_checkpoint(token) != checkpoints_.end();
    }

    void rollback(const Snapshot& token) {
        const auto checkpoint = require_checkpoint(token);

        while (history_.size() > checkpoint->history_size) {
            Change& change = history_.back();
            values_.at(change.index) = std::move(change.old_value);
            history_.pop_back();
        }

        // 対象より内側の checkpoint も、その変更履歴と一緒に消える。
        const auto index = static_cast<size_type>(
            std::distance(checkpoints_.begin(), checkpoint));
        checkpoints_.erase(checkpoints_.begin() + static_cast<std::ptrdiff_t>(index),
                           checkpoints_.end());
        if (checkpoints_.empty()) {
            history_.clear();
        }
    }

    void commit(const Snapshot& token) {
        const auto checkpoint = require_checkpoint(token);
        checkpoints_.erase(checkpoint);
        if (checkpoints_.empty()) {
            history_.clear();
        }
    }

    // 引数なし版は最も内側の checkpoint を対象にする。
    void rollback() {
        if (checkpoints_.empty()) {
            throw std::logic_error("no active rollback snapshot");
        }
        const Checkpoint& checkpoint = checkpoints_.back();
        Snapshot token;
        token.owner_ = this;
        token.history_size_ = checkpoint.history_size;
        token.serial_ = checkpoint.serial;
        rollback(token);
    }

    void commit() {
        if (checkpoints_.empty()) {
            throw std::logic_error("no active rollback snapshot");
        }
        const Checkpoint& checkpoint = checkpoints_.back();
        Snapshot token;
        token.owner_ = this;
        token.history_size_ = checkpoint.history_size;
        token.serial_ = checkpoint.serial;
        commit(token);
    }

    void clear() {
        values_.clear();
        touch_serial_.clear();
        history_.clear();
        checkpoints_.clear();
        next_serial_ = 1U;
    }

  private:
    typename std::vector<Checkpoint>::const_iterator
    find_checkpoint(const Snapshot& token) const noexcept {
        return std::find_if(
            checkpoints_.cbegin(), checkpoints_.cend(),
            [&token](const Checkpoint& checkpoint) {
                return checkpoint.serial == token.serial_ &&
                       checkpoint.history_size == token.history_size_;
            });
    }

    typename std::vector<Checkpoint>::iterator
    find_checkpoint(const Snapshot& token) noexcept {
        return std::find_if(
            checkpoints_.begin(), checkpoints_.end(),
            [&token](const Checkpoint& checkpoint) {
                return checkpoint.serial == token.serial_ &&
                       checkpoint.history_size == token.history_size_;
            });
    }

    typename std::vector<Checkpoint>::iterator
    require_checkpoint(const Snapshot& token) {
        if (token.owner_ != this) {
            throw std::invalid_argument("snapshot belongs to another array");
        }
        auto checkpoint = find_checkpoint(token);
        if (checkpoint == checkpoints_.end()) {
            throw std::invalid_argument("snapshot is no longer active");
        }
        return checkpoint;
    }

    T& mutable_at(const size_type index) {
        T& value = values_.at(index);
        if (!checkpoints_.empty()) {
            const std::size_t serial = checkpoints_.back().serial;
            if (touch_serial_[index] != serial) {
                history_.push_back({index, value});
                touch_serial_[index] = serial;
            }
        }
        return value;
    }

    void touch_all() {
        if (checkpoints_.empty()) {
            return;
        }
        for (size_type index = 0U; index < values_.size(); ++index) {
            (void)mutable_at(index);
        }
    }

    std::vector<T> values_;
    // 最も内側の checkpoint で最後にログへ入れた serial。これにより、
    // 読み取りや同じ要素への連続書き込みで履歴が無制限に膨らまない。
    std::vector<std::size_t> touch_serial_;
    std::vector<Change> history_;
    std::vector<Checkpoint> checkpoints_;
    std::size_t next_serial_ = 1U;
};

}  // namespace ahc
