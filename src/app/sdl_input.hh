#pragma once

#include "input.hh"
#include "input_repeat.hh"

namespace hojy::app {

class SdlInputCollector final {
public:
    void collect(InputQueue &queue);

private:
    InputRepeater inputRepeater_;
};

}
