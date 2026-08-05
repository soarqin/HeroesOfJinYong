#include "scene/logic/rle.hh"

#include "test_support.hh"

#include <iostream>
#include <string>

namespace {

void testValidRleRowsAreAccepted() {
    // 1x1 image, one row containing one skipped pixel and no payload.
    const std::string data{
        '\x01', '\x00', '\x01', '\x00',
        '\x00', '\x00', '\x00', '\x00',
        '\x02', '\x01', '\x00',
    };
    HOJY_CHECK_EQ(hojy::scene::logic::validateRleData(data), true);
}

void testTruncatedRleRowsAreRejected() {
    const std::string data{
        '\x01', '\x00', '\x01', '\x00',
        '\x02', '\x01',
    };
    HOJY_CHECK_EQ(hojy::scene::logic::validateRleData(data), false);
}

}

int main() {
    try {
        testValidRleRowsAreAccepted();
        testTruncatedRleRowsAreRejected();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
