#pragma once

// This is a proposed protocol to dump the contents of registers

#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

#include "utility/dump.hpp"

namespace norb {
    template <size_t reg_count_, typename RegType_ = uint32_t>
    class RegisterDumper {
    private:
        std::ofstream file_;
        int line_number_;
        bool enabled_ = false;  // Flag to disable dumping

    public:
        RegisterDumper(const std::string &filename, bool enabled = true) : line_number_(0), enabled_(enabled) {
            // Clear the file at bootup and keep it open
            if (filename.empty()) return;
            if (not enabled_) return;  // If dumping is disabled, do not create the file
            file_.open(filename, std::ios::trunc);
            if (not file_.is_open()) {
                throw std::runtime_error("Failed to open file for register dumping: " + filename);
            }
        }

        ~RegisterDumper() {
            if (file_.is_open()) {
                file_.close();
            }
        }

        void dump(uint32_t pc_at_commit, const std::array<RegType_, reg_count_> &reg_snapshot) {
            dump_impl(pc_at_commit, reg_snapshot);
        }

    private:
        void dump_impl(uint32_t pc_at_commit, const std::array<RegType_, reg_count_> &reg_snapshot) {
            if (not enabled_) return;
            if (not file_.is_open()) return;  // do nothing (empty filename)
            std::ostringstream oss;
            oss << "[" << norb::pad_with_zero(++line_number_, 4) << "] ";  // line number for each line
            oss << norb::hex(pc_at_commit) << " | ";  // PC at commit
            for (size_t i = 0; i < reg_count_; ++i) {
                const auto reg_value = reg_snapshot[i];
                if (reg_value == 0)
                    oss << "R" << i << "(" << 0 << ")";
                else
                    oss << "R" << i << "(" << reg_value << "=" << norb::hex(reg_value) << ")";
                if (i < reg_count_ - 1) oss << " ";
            }
            oss << "\n";

            // Write to the persistent file stream
            file_ << oss.str();
            file_.flush();  // Ensure data is written immediately
        }
    };
}  // namespace norb
