#pragma once

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace hojy::battle {

struct RandomCall {
    int minimum;
    int maximum;
    int rawValue;
    int result;
};

class RandomSource {
public:
    virtual ~RandomSource() = default;
    virtual int next(int upperExclusive) = 0;
    virtual int next(int minimum, int maximum) = 0;
};

class SequenceRandom final: public RandomSource {
public:
    explicit SequenceRandom(std::initializer_list<int> values);
    explicit SequenceRandom(std::vector<int> values);

    int next(int upperExclusive) override;
    int next(int minimum, int maximum) override;

    [[nodiscard]] std::size_t callCount() const noexcept;
    [[nodiscard]] const std::vector<RandomCall> &calls() const noexcept;

private:
    int consume(int minimum, int maximum);

    std::vector<int> values_;
    std::vector<RandomCall> calls_;
    std::size_t index_ = 0;
};

}
