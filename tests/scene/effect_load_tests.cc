#include "content/factors.hh"
#include "content/grpdata.hh"
#include "scene/effect.hh"
#include "test_support.hh"

#include <filesystem>
#include <iostream>

namespace {

void effectLoadCommitsOnlyValidatedSlices() {
    const auto oldPath = std::filesystem::current_path();
    const auto directory = std::filesystem::temp_directory_path() / "hojy-effect-tests";
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory);
    std::filesystem::current_path(directory);

    auto &frames = hojy::content::gFactors.effectFrames;
    frames.fill(0);
    frames[0] = 1;
    frames[1] = 1;
    HOJY_CHECK_EQ(hojy::content::GrpData::saveData("GOOD", {"first", "second"}), true);

    hojy::scene::Effect effect;
    HOJY_CHECK_EQ(effect.load("GOOD"), true);
    HOJY_CHECK_EQ(effect[0].size(), 1U);
    HOJY_CHECK_EQ(effect[0][0], "first");

    frames[0] = -1;
    const auto before = effect[0];
    HOJY_CHECK_EQ(effect.load("GOOD"), false);
    HOJY_CHECK_EQ(effect[0], before);

    frames[0] = 1;
    frames[1] = 1;
    HOJY_CHECK_EQ(hojy::content::GrpData::saveData("EXTRA", {"first", "second", "third"}), true);
    HOJY_CHECK_EQ(effect.load("EXTRA"), false);
    HOJY_CHECK_EQ(effect[0], before);

    std::filesystem::current_path(oldPath, error);
    std::filesystem::remove_all(directory, error);
}

}

int main() {
    try {
        effectLoadCommitsOnlyValidatedSlices();
    } catch (const std::exception &exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
    return 0;
}
