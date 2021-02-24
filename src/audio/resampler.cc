#include "resampler.hh"

#include <soxr.h>
#include <cmath>

namespace hojy::audio {

void DataTypeToSize(Resampler::DataType type, size_t &size) {
    switch (type) {
    case Resampler::F32:
        size = 4;
        break;
    case Resampler::F64:
        size = 8;
        break;
    case Resampler::I32:
        size = 4;
        break;
    case Resampler::I16:
        size = 2;
        break;
    }
}

Resampler::Resampler(std::uint32_t channels, double sampleRateIn, double sampleRateOut, DataType typeIn, DataType typeOut) {
    auto io_spec = soxr_io_spec(soxr_datatype_t(typeIn), soxr_datatype_t(typeOut));
    auto *resampler = soxr_create(sampleRateIn, sampleRateOut, channels, nullptr, &io_spec, nullptr, nullptr);
    resampler_ = resampler;
    rate_ = sampleRateOut / sampleRateIn;
    DataTypeToSize(typeIn, sampleSizeIn_);
    DataTypeToSize(typeOut, sampleSizeOut_);
    buffer_.setUnitSize(sampleSizeOut_);
}

void Resampler::setInputCallback(Resampler::InputCallback callback) {
    inputCB_ = std::move(callback);
    if (inputCB_) {
        soxr_set_input_fn(static_cast<soxr_t>(resampler_), readCB, this, 65536);
    } else {
        soxr_set_input_fn(static_cast<soxr_t>(resampler_), nullptr, this, 65536);
    }
}

size_t Resampler::read(void *data, size_t size) {
    if (inputCB_) {
        return soxr_output(static_cast<soxr_t>(resampler_), data, size / sampleSizeOut_) * sampleSizeOut_;
    }
    return 0;
}

size_t Resampler::write(const void *data, size_t size) {
    if (inputCB_) {
        return 0;
    }
    size_t idone, odone;
    auto isize = size_t(size / sampleSizeIn_);
    auto osize = size_t(std::ceil(isize * rate_));
    auto *odata = buffer_.alloc(osize);
    if (!odata) {
        return 0;
    }
    if (soxr_process(static_cast<soxr_t>(resampler_), data, isize, &idone, odata, osize, &odone) != nullptr) {
        return 0;
    }
    buffer_.push(odata, odone);
    return idone;
}

size_t Resampler::readCB(void *input_fn_state, const void **data, size_t len) {
    return 0;
}

}
