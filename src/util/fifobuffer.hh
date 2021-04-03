/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <list>
#include <string>
#include <mutex>
#include <cstring>

namespace hojy::util {

class FIFOBuffer final {
    struct Slice {
        void *buf;
        size_t size;
    };

public:
    inline ~FIFOBuffer() {
        reset();
    }
    inline void setUnitSize(size_t usize = 1) { unitSize_ = usize; }
    inline void reset() {
        std::unique_lock<std::mutex> lk(mutex_);
        for (auto &s: slices_) {
            delete[] static_cast<std::uint8_t *>(s.buf);
        }
        slices_.clear();
        pos_ = 0;
        size_ = 0;
    }
    [[nodiscard]] inline void *alloc(size_t size) const {
        if (!size) {
            return nullptr;
        }
        return new(std::nothrow) std::uint8_t[unitSize_ * size];
    }
    inline static void dealloc(void *buf) {
        delete[] static_cast<std::uint8_t *>(buf);
    }
    inline void push(void *buf, size_t size) {
        std::unique_lock<std::mutex> lk(mutex_);
        slices_.emplace_back(Slice{buf, size});
        size_ += size;
    }
    inline size_t pop(void *data, size_t count, size_t space) {
        std::unique_lock<std::mutex> lk(mutex_);
        auto *buf = static_cast<std::uint8_t *>(data);
        size_t size = count;
        size_t step = unitSize_;
        bool needSkip = space > step;
        while (!slices_.empty() && size > 0) {
            auto &slice = slices_.front();
            if (pos_ + size >= slice.size) {
                size_t readsz = slice.size - pos_;
                if (needSkip) {
                    auto *rbuf = static_cast<std::uint8_t *>(slice.buf) + pos_ * step;
                    for (size_t i = readsz; i; --i) {
                        memcpy(buf, rbuf, step);
                        buf += space;
                        rbuf += step;
                    }
                } else {
                    memcpy(buf, static_cast<std::uint8_t *>(slice.buf) + pos_ * step, readsz * step);
                    buf += readsz * step;
                }
                size -= readsz;
                pos_ = 0;
                dealloc(slice.buf);
                slices_.pop_front();
            } else {
                if (needSkip) {
                    auto *rbuf = static_cast<std::uint8_t *>(slice.buf) + pos_ * step;
                    for (size_t i = size; i; --i) {
                        memcpy(buf, rbuf, step);
                        buf += space;
                        rbuf += step;
                    }
                } else {
                    memcpy(buf, static_cast<std::uint8_t *>(slice.buf) + pos_ * step, size * step);
                }
                pos_ += size;
                size = 0;
            }
        }
        count -= size;
        size_ -= count;
        return count;
    }
    inline size_t size() {
        std::unique_lock<std::mutex> lk(mutex_);
        return size_;
    }

private:
    std::list<Slice> slices_;
    size_t unitSize_ = 1;
    size_t pos_ = 0;
    size_t size_ = 0;
    std::mutex mutex_;
};

}
