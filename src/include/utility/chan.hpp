// chan.hpp
// - implements a Channel over bus.hpp

#pragma once

#include <memory>
#include <stdexcept>

#include "buffered.hpp"
#include "bus.hpp"

namespace norb {
    template <typename T>
    class ChannelReader;
    template <typename T>
    class ChannelWriter;

    template <typename T>
    class ChannelReader {
    private:
        std::shared_ptr<Buffered<T>> data_;
        std::shared_ptr<Buffered<bool>> has_data_;

    public:
        ChannelReader(std::shared_ptr<Buffered<T>> data_, std::shared_ptr<Buffered<bool>> has_data_) :
            data_(std::move(data_)), has_data_(std::move(has_data_)) {}
        ChannelReader() : data_(nullptr), has_data_(nullptr) {}

        [[nodiscard]] bool has_data() const { return has_data_->read(); }

        T read() {
            if (!has_data()) {
                throw std::runtime_error("No data available to read");
            }
            has_data_->write(false);
            return data_->read();
        }
    };


    template <typename T>
    class ChannelWriter {
    private:
        std::shared_ptr<Buffered<T>> data_;
        std::shared_ptr<Buffered<bool>> has_data_;

    public:
        ChannelWriter(std::shared_ptr<Buffered<T>> data_, std::shared_ptr<Buffered<bool>> has_data_) :
            data_(std::move(data_)), has_data_(std::move(has_data_)) {}
        ChannelWriter() : data_(nullptr), has_data_(nullptr) {}

        [[nodiscard]] bool is_connected() const { return data_ != nullptr && has_data_ != nullptr; }

        [[nodiscard]] bool has_data() const { return has_data_->read(); }

        void write(const T& value) {
            if (has_data()) {
                throw std::runtime_error("Cannot write: data already available");
            }
            has_data_->write(true);
            data_->write(value);
        }
    };

    // Factory function to create connected reader and writer
    template <typename T>
    void make_channel(ChannelWriter<T> &writer, ChannelReader<T> &reader) {
        auto data = std::make_shared<Buffered<T>>();
        auto has_data = std::make_shared<Buffered<bool>>(false);

        reader = ChannelReader<T>(data, has_data);
        writer = ChannelWriter<T>(data, has_data);
    }
}  // namespace norb
