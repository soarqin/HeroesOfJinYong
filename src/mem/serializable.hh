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

#include <string>
#include <vector>
#include <iostream>

#ifdef __GNUC__
#define ATTR_PACKED __attribute__((packed))
#else
#define ATTR_PACKED
#endif

namespace hojy::mem {

class Serializable {
public:
    virtual ~Serializable() = default;
    void serialize(std::string&);
    void deserialize(const std::string&);
    virtual Serializable &operator>>(std::ostream &ostm) { return *this; }
    virtual Serializable &operator<<(std::istream &istm) { return *this; }
};

template<typename T>
class SerializableStruct: public Serializable {
public:
    T *operator->() { return &data_; }
    const T *operator->() const { return &data_; }

    Serializable &operator>>(std::ostream &ostm) override {
        ostm.write(reinterpret_cast<const char*>(&data_), sizeof(data_));
        return *this;
    }
    Serializable &operator<<(std::istream &istm) override {
        istm.read(reinterpret_cast<char*>(&data_), sizeof(data_));
        return *this;
    }

private:
    T data_;
};

template<typename T>
class SerializableStructVec: public Serializable {
public:
    T *operator[](size_t index) { return index < data_.size() ? &data_[index] : nullptr; }
    const T *operator[](size_t index) const { return index < data_.size() ? &data_[index] : nullptr; }
    [[nodiscard]] size_t size() const { return data_.size(); }

    Serializable &operator>>(std::ostream &ostm) override {
        for (auto &data: data_) {
            ostm.write(reinterpret_cast<const char *>(&data), sizeof(data));
        }
        return *this;
    }

    Serializable &operator<<(std::istream &istm) override {
        data_.clear();
        while (!istm.eof()) {
            T data;
            istm.read(reinterpret_cast<char *>(&data), sizeof(data));
            if (!istm.fail()) {
                data_.emplace_back(data);
            }
        }
        return *this;
    }

private:
    std::vector<T> data_;
};

}
