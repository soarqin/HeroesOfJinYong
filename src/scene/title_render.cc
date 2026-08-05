#include "title.hh"
#include "title_render.hh"

#include "util/math.hh"

#include <stdexcept>
#include <type_traits>

namespace hojy::scene {
namespace {

struct TitleRenderLayout final {
    int backgroundX = 0;
    int backgroundY = 0;
    int backgroundWidth = 0;
    int backgroundHeight = 0;
    std::pair<int, int> scale{1, 1};
};

TitleRenderLayout layoutFor(const Texture &background, int width, int height) {
    TitleRenderLayout result;
    result.backgroundWidth = width;
    result.backgroundHeight = width * background.height() / background.width();
    if (result.backgroundHeight > height) {
        result.backgroundHeight = height;
        result.backgroundWidth = height * background.width() / background.height();
    }
    result.backgroundX = (width - result.backgroundWidth) / 2;
    result.backgroundY = (height - result.backgroundHeight) / 2;
    result.scale = result.backgroundY == 0
        ? util::calcSmallestDivision(height, 200)
        : util::calcSmallestDivision(width, 320);
    return result;
}

}

bool Title::prepareTextResources() {
    if (!renderer_ || !renderer_->ttf()) { return false; }
    auto *ttf = renderer_->ttf();
    bool ready = ttf->prepareText(L"0123456789 +-/:", fontSize_);
    std::visit([&ready, ttf, this](const auto &snapshot) {
        using Snapshot = std::decay_t<decltype(snapshot)>;
        if constexpr (std::is_same<Snapshot, TitleNameEntrySnapshot>::value) {
            ready = ttf->prepareText(snapshot.displayText, fontSize_) && ready;
        } else if constexpr (std::is_same<Snapshot, TitlePreviewSnapshot>::value) {
            ready = ttf->prepareText(snapshot.prompt, fontSize_) && ready;
            for (const auto &choice: snapshot.choices) {
                ready = ttf->prepareText(choice, fontSize_) && ready;
            }
            for (const auto &property: snapshot.properties) {
                ready = ttf->prepareText(property.displayText, fontSize_) && ready;
            }
        }
    }, snapshot_);
    return ready;
}

void Title::makeCache() {
    if (!renderer_ || !big_) {
        throw std::runtime_error("title render resources are unavailable");
    }
    cacheBegin();
    renderer_->clear(0, 0, 0, 255);

    const auto layout = layoutFor(*big_, width_, height_);
    renderer_->renderTexture(
        big_, layout.backgroundX, layout.backgroundY,
        layout.backgroundWidth, layout.backgroundHeight,
        0, 0, big_->width(), big_->height(), false);

    std::visit([this, &layout](const auto &snapshot) {
        using Snapshot = std::decay_t<decltype(snapshot)>;
        const auto *logo = titleTextureMgr_[0];
        if constexpr (std::is_same<Snapshot, TitleMainMenuSnapshot>::value) {
            const auto *selection = titleTextureMgr_[1 + snapshot.selectedIndex];
            if (!logo || !selection) {
                throw std::runtime_error("title menu texture is unavailable");
            }
            const auto x = (width_ - (logo->width() + logo->originX())
                            * layout.scale.first / layout.scale.second) / 2;
            const auto y = height_ - 65 * layout.scale.first / layout.scale.second;
            const auto offsets = detail::titleMenuSelectionOffsets(y, layout.scale);
            renderer_->renderTexture(logo, x, y, layout.scale);
            renderer_->renderTexture(
                selection, x, offsets[static_cast<std::size_t>(snapshot.selectedIndex)],
                layout.scale);
        } else if constexpr (std::is_same<Snapshot, TitleLoadMenuSnapshot>::value) {
            const auto *load = titleTextureMgr_[4];
            const auto *selection = titleTextureMgr_[5 + snapshot.selectedIndex];
            if (!logo || !load || !selection) {
                throw std::runtime_error("title load texture is unavailable");
            }
            const auto x = (width_ - (logo->width() + logo->originX())
                            * layout.scale.first / layout.scale.second) / 2;
            const auto y = height_ - 65 * layout.scale.first / layout.scale.second;
            const auto offsets = detail::titleMenuSelectionOffsets(y, layout.scale);
            renderer_->renderTexture(logo, x, y, layout.scale);
            renderer_->renderTexture(load, x, y, layout.scale);
            renderer_->renderTexture(
                selection, x, offsets[static_cast<std::size_t>(snapshot.selectedIndex)],
                layout.scale);
        } else if constexpr (std::is_same<Snapshot, TitleNameEntrySnapshot>::value) {
            auto *ttf = renderer_->ttf();
            if (!ttf) { throw std::runtime_error("title font is unavailable"); }
            const auto lineHeight = ttf->fontSize() + 5;
            const auto y = height_ - lineHeight * 5;
            ttf->setColor(236, 236, 236);
            ttf->setAltColor(2, 224, 180, 32);
            ttf->renderPrepared(snapshot.displayText, width_ / 4, y, false);
        } else if constexpr (std::is_same<Snapshot, TitlePreviewSnapshot>::value) {
            auto *ttf = renderer_->ttf();
            if (!ttf) { throw std::runtime_error("title font is unavailable"); }
            const auto lineHeight = ttf->fontSize() + 5;
            const auto propertyHeight = lineHeight - 2 - 5 / 4;
            const auto colWidth = ttf->fontSize() * 21 / 4;
            const auto baseX = (width_ - colWidth * 4 + 20) / 2;
            auto y = height_ - lineHeight * 5;
            ttf->setColor(236, 236, 236);
            ttf->setAltColor(2, 224, 180, 32);
            ttf->renderPrepared(snapshot.prompt, baseX, y, false);
            y += lineHeight * 2;
            for (const auto &property: snapshot.properties) {
                const auto x = baseX + property.column * colWidth;
                const auto propertyY = y + property.row * lineHeight;
                if (property.highlighted) {
                    const auto textWidth = ttf->preparedStringWidth(property.displayText);
                    renderer_->fillRect(
                        x, propertyY, textWidth + 2, propertyHeight,
                        property.background.red, property.background.green,
                        property.background.blue, 255);
                }
                ttf->setColor(
                    property.foreground.red, property.foreground.green,
                    property.foreground.blue);
                ttf->renderPrepared(
                    property.displayText, x, propertyY, property.shadow);
            }
            const auto choicesY = y + lineHeight * 3;
            for (std::size_t index = 0; index < snapshot.choices.size(); ++index) {
                const auto &choice = snapshot.choices[index];
                ttf->setColor(
                    index == static_cast<std::size_t>(snapshot.confirmationIndex)
                        ? 252 : 236,
                    index == static_cast<std::size_t>(snapshot.confirmationIndex)
                        ? 236 : 236,
                    index == static_cast<std::size_t>(snapshot.confirmationIndex)
                        ? 132 : 236);
                ttf->renderPrepared(
                    choice, baseX + colWidth * static_cast<int>(index), choicesY,
                    false);
            }
        }
    }, snapshot_);
    cacheEnd();
}

}
