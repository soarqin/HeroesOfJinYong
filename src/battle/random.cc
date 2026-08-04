#include "random.hh"

#include <stdexcept>
#include <utility>

namespace hojy::battle {

SequenceRandom::SequenceRandom(std::initializer_list<int> values) {
    values_.reserve(values.size());
    for (const auto value: values) {
        values_.push_back(static_cast<std::int64_t>(value));
    }
}

SequenceRandom::SequenceRandom(std::vector<int> values) {
    values_.reserve(values.size());
    for (const auto value: values) {
        values_.push_back(value);
    }
}

SequenceRandom::SequenceRandom(std::vector<std::int64_t> values): values_(std::move(values)) {
}

int SequenceRandom::next(int upperExclusive) {
    if (upperExclusive <= 1 || upperExclusive > OriginalRandomBoundMax) {
        return 0;
    }
    return consume(0, upperExclusive - 1);
}

int SequenceRandom::next(int minimum, int maximum) {
    if (minimum > maximum) {
        throw std::invalid_argument("minimum must not exceed maximum");
    }
    return consume(minimum, maximum);
}

int SequenceRandom::consume(int minimum, int maximum) {
    if (index_ >= values_.size()) {
        throw std::out_of_range("battle random sequence exhausted");
    }
    const auto raw = values_[index_++];
    const auto width = static_cast<std::int64_t>(maximum)
        - static_cast<std::int64_t>(minimum) + 1;
    const auto normalized = ((raw % width) + width) % width;
    const auto result = static_cast<std::int64_t>(minimum) + normalized;
    calls_.push_back(RandomCall{
        minimum, maximum, raw, static_cast<int>(result),
    });
    return static_cast<int>(result);
}

std::size_t SequenceRandom::callCount() const noexcept {
    return calls_.size();
}

const std::vector<RandomCall> &SequenceRandom::calls() const noexcept {
    return calls_;
}

int RecordingRandom::next(int upperExclusive) {
    const int result = source_->next(upperExclusive);
    if (upperExclusive > 1 && upperExclusive <= OriginalRandomBoundMax) {
        calls_.push_back(RandomCall{
            0, upperExclusive - 1, static_cast<std::int64_t>(result), result,
        });
    }
    return result;
}

int RecordingRandom::next(int minimum, int maximum) {
    const int result = source_->next(minimum, maximum);
    calls_.push_back(RandomCall{
        minimum, maximum, static_cast<std::int64_t>(result) - minimum, result,
    });
    return result;
}

void RecordingRandom::clear() noexcept {
    calls_.clear();
}

std::size_t RecordingRandom::callCount() const noexcept {
    return calls_.size();
}

const std::vector<RandomCall> &RecordingRandom::calls() const noexcept {
    return calls_;
}

}
