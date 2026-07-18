#pragma once

#include "yyjson.h"

#include <cstddef>
#include <string_view>

namespace serdde_wire::detail {

class JsonSourceLocator {
  public:
    JsonSourceLocator(std::string_view source, yyjson_val* target)
        : source_(source), target_(target) {}

    std::size_t locate(yyjson_val* root) {
        cursor_ = 0;
        found_ = source_.size();
        visit(root);
        return found_;
    }

  private:
    std::string_view source_;
    yyjson_val* target_ = nullptr;
    std::size_t cursor_ = 0;
    std::size_t found_ = 0;

    void skip_space() {
        while (cursor_ < source_.size()) {
            const char ch = source_[cursor_];
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
                return;
            }
            ++cursor_;
        }
    }

    void skip_string() {
        if (cursor_ >= source_.size() || source_[cursor_] != '"') {
            return;
        }
        ++cursor_;
        while (cursor_ < source_.size()) {
            const char ch = source_[cursor_++];
            if (ch == '"') {
                return;
            }
            if (ch == '\\' && cursor_ < source_.size()) {
                ++cursor_;
            }
        }
    }

    void skip_scalar() {
        while (cursor_ < source_.size()) {
            const char ch = source_[cursor_];
            if (ch == ',' || ch == ']' || ch == '}' || ch == ' ' || ch == '\t' ||
                ch == '\n' || ch == '\r') {
                return;
            }
            ++cursor_;
        }
    }

    void visit(yyjson_val* value) {
        skip_space();
        if (found_ != source_.size()) {
            return;
        }
        if (value == target_) {
            found_ = cursor_;
            return;
        }
        if (yyjson_is_str(value)) {
            skip_string();
            return;
        }
        if (yyjson_is_arr(value)) {
            if (cursor_ < source_.size()) ++cursor_;
            const std::size_t size = yyjson_arr_size(value);
            for (std::size_t i = 0; i < size; ++i) {
                visit(yyjson_arr_get(value, i));
                if (found_ != source_.size()) return;
                skip_space();
                if (i + 1 < size && cursor_ < source_.size()) ++cursor_;
            }
            skip_space();
            if (cursor_ < source_.size()) ++cursor_;
            return;
        }
        if (yyjson_is_obj(value)) {
            if (cursor_ < source_.size()) ++cursor_;
            yyjson_obj_iter iterator = yyjson_obj_iter_with(value);
            yyjson_val* key = nullptr;
            std::size_t index = 0;
            while ((key = yyjson_obj_iter_next(&iterator)) != nullptr) {
                skip_space();
                skip_string();
                skip_space();
                if (cursor_ < source_.size()) ++cursor_;
                visit(yyjson_obj_iter_get_val(key));
                if (found_ != source_.size()) return;
                skip_space();
                if (++index < yyjson_obj_size(value) && cursor_ < source_.size()) ++cursor_;
            }
            skip_space();
            if (cursor_ < source_.size()) ++cursor_;
            return;
        }
        skip_scalar();
    }
};

inline std::size_t json_value_offset(std::string_view source, yyjson_val* root,
                                     yyjson_val* target) {
    if (root == nullptr || target == nullptr) {
        return 0;
    }
    const std::size_t found = JsonSourceLocator(source, target).locate(root);
    return found == source.size() ? 0 : found;
}

} // namespace serdde_wire::detail
