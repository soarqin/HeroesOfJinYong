#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace hojy::battle {

inline constexpr int OriginalRandomBoundMax = 30000;

struct RandomCall {
    int minimum;
    int maximum;
    std::int64_t rawValue;
    int result;
};

inline bool operator==(const RandomCall &left, const RandomCall &right) {
    return left.minimum == right.minimum
        && left.maximum == right.maximum
        && left.rawValue == right.rawValue
        && left.result == right.result;
}

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
    explicit SequenceRandom(std::vector<std::int64_t> values);

    int next(int upperExclusive) override;
    int next(int minimum, int maximum) override;

    [[nodiscard]] std::size_t callCount() const noexcept;
    [[nodiscard]] const std::vector<RandomCall> &calls() const noexcept;

private:
    int consume(int minimum, int maximum);

    std::vector<std::int64_t> values_;
    std::vector<RandomCall> calls_;
    std::size_t index_ = 0;
};

class RecordingRandom final: public RandomSource {
public:
    explicit RecordingRandom(RandomSource &source): source_(&source) {
    }

    int next(int upperExclusive) override;
    int next(int minimum, int maximum) override;

    void clear() noexcept;
    [[nodiscard]] std::size_t callCount() const noexcept;
    [[nodiscard]] const std::vector<RandomCall> &calls() const noexcept;

private:
    RandomSource *source_ = nullptr;
    std::vector<RandomCall> calls_;
};

}
