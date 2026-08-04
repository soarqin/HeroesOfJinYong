#include "random.hh"

#include <stdexcept>
#include <utility>

namespace hojy::battle {

SequenceRandom::SequenceRandom(std::initializer_list<int> values): values_(values) {
}

SequenceRandom::SequenceRandom(std::vector<int> values): values_(std::move(values)) {
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
    const int raw = values_[index_++];
    const int width = maximum - minimum + 1;
    const int normalized = ((raw % width) + width) % width;
    const int result = minimum + normalized;
    calls_.push_back(RandomCall{minimum, maximum, raw, result});
    return result;
}

std::size_t SequenceRandom::callCount() const noexcept {
    return calls_.size();
}

const std::vector<RandomCall> &SequenceRandom::calls() const noexcept {
    return calls_;
}

}
