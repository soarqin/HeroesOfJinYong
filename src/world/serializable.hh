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
#include <utility>

namespace hojy::world::state {

class Serializable {
public:
    virtual ~Serializable() = default;
    void serialize(std::string&);
    [[nodiscard]] bool deserialize(const std::string&);
    virtual Serializable &operator>>(std::ostream &ostm) { return *this; }
    virtual Serializable &operator<<(std::istream &istm) { return *this; }

protected:
    [[nodiscard]] virtual bool validSerializedSize(size_t size) const { return true; }
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
        T candidate{};
        istm.read(reinterpret_cast<char *>(&candidate), sizeof(candidate));
        if (istm) {
            data_ = candidate;
        }
        return *this;
    }

private:
    [[nodiscard]] bool validSerializedSize(size_t size) const override { return size == sizeof(T); }

private:
    T data_{};
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
        std::vector<T> candidate;
        while (true) {
            T data{};
            istm.read(reinterpret_cast<char *>(&data), sizeof(data));
            const auto count = istm.gcount();
            if (count == 0 && istm.eof()) {
                istm.clear(istm.rdstate() & ~std::ios::failbit);
                data_ = std::move(candidate);
                return *this;
            }
            if (count != static_cast<std::streamsize>(sizeof(data))) {
                return *this;
            }
            candidate.emplace_back(data);
        }
    }

private:
    [[nodiscard]] bool validSerializedSize(size_t size) const override { return size % sizeof(T) == 0; }

private:
    std::vector<T> data_;
};

}
