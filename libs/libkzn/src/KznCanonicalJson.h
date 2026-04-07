#pragma once

#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace kzn {

// A minimal JSON value type for canonical serialization.
// Supports: null, bool, int64, double, string, array, object (sorted map).
struct JsonValue;

using JsonObject = std::map<std::string, JsonValue>;
using JsonArray  = std::vector<JsonValue>;

struct JsonValue {
    using Variant = std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        double,
        std::string,
        JsonArray,
        JsonObject
    >;

    Variant value;

    JsonValue() : value(nullptr) {}
    JsonValue(std::nullptr_t) : value(nullptr) {}
    JsonValue(bool v) : value(v) {}
    JsonValue(int v) : value(static_cast<int64_t>(v)) {}
    JsonValue(int64_t v) : value(v) {}
    JsonValue(double v) : value(v) {}
    JsonValue(const char* v) : value(std::string(v)) {}
    JsonValue(const std::string& v) : value(v) {}
    JsonValue(std::string&& v) : value(std::move(v)) {}
    JsonValue(const JsonArray& v) : value(v) {}
    JsonValue(JsonArray&& v) : value(std::move(v)) {}
    JsonValue(const JsonObject& v) : value(v) {}
    JsonValue(JsonObject&& v) : value(std::move(v)) {}
};

// Produces deterministic JSON bytes from a JsonValue tree.
// Sorted keys, no whitespace, ASCII-only keys, finite numbers.
// Returns empty string on error (e.g., non-finite number).
std::string serializeCanonical(const JsonValue& val);

// Convenience: serialize a JsonObject directly
std::string serializeCanonical(const JsonObject& obj);

} // namespace kzn
