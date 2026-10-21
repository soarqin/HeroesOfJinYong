#include "item_selection_controller.hh"

#include <utility>

namespace hojy::scene {

FunctionItemSelectionController::FunctionItemSelectionController(
        SelectFunction selectFunction, CancelFunction cancelFunction):
    selectFunction_(std::move(selectFunction)),
    cancelFunction_(std::move(cancelFunction)) {
}

void FunctionItemSelectionController::select(ItemSelectionHost &host,
                                              std::int16_t itemId) {
    if (selectFunction_) { selectFunction_(host, itemId); }
}

void FunctionItemSelectionController::cancel(ItemSelectionHost &host) {
    if (cancelFunction_) { cancelFunction_(host); }
}

void FunctionItemSelectionController::bindItems(
        const std::vector<ItemViewEntrySnapshot> &) {
}

}
