#pragma once

#include <util/fifobuffer.hh>

#include <functional>
#include <cstdint>

namespace hojy::audio {

class Resampler final {
public:
    enum DataType {
        F32  = 0,  F64,  I32,  I16,
    };
    using InputCallback = std::function<size_t (void*, size_t)>;
    Resampler(std::uint32_t channels, double sampleRateIn, double sampleRateOut, DataType typeIn, DataType typeOut);
    inline void setInputCallback(InputCallback callback);
    size_t read(void *data, size_t size);
    size_t write(const void *data, size_t size);

private:
    static size_t readCB(void * input_fn_state, void const **data, size_t len);

private:
    void *resampler_ = nullptr;
    InputCallback inputCB_;
    util::FIFOBuffer buffer_;
    double rate_ = 0.;
    size_t sampleSizeIn_, sampleSizeOut_ = 0;
};

}
