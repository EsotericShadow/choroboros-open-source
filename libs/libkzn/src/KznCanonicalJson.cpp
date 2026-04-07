#include "KznCanonicalJson.h"
#include <cmath>
#include <cstdio>
#include <sstream>

namespace kzn {

namespace {

// Escape a string for JSON output. Only standard JSON escaping.
void appendEscapedString(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    out += '"';
}

// Format a double per RFC 8785 / JCS rules:
// - No NaN/Infinity
// - No unnecessary trailing zeros
// - Use shortest representation
std::string formatNumber(double v) {
    if (!std::isfinite(v))
        return ""; // error sentinel

    // If it's an integer value, emit without decimal point
    if (v == static_cast<double>(static_cast<int64_t>(v)) &&
        v >= -9007199254740992.0 && v <= 9007199254740992.0) {
        return std::to_string(static_cast<int64_t>(v));
    }

    // Use enough precision for round-trip
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", v);

    // Try shorter representations to find the shortest that round-trips
    for (int prec = 1; prec < 17; ++prec) {
        char shorter[64];
        std::snprintf(shorter, sizeof(shorter), "%.*g", prec, v);
        double parsed = 0;
        std::sscanf(shorter, "%lf", &parsed);
        if (parsed == v) {
            return shorter;
        }
    }

    return buf;
}

bool serializeImpl(std::string& out, const JsonValue& val) {
    return std::visit([&](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            out += "null";
            return true;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            out += v ? "true" : "false";
            return true;
        }
        else if constexpr (std::is_same_v<T, int64_t>) {
            out += std::to_string(v);
            return true;
        }
        else if constexpr (std::is_same_v<T, double>) {
            auto s = formatNumber(v);
            if (s.empty()) return false; // non-finite
            out += s;
            return true;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            appendEscapedString(out, v);
            return true;
        }
        else if constexpr (std::is_same_v<T, JsonArray>) {
            out += '[';
            for (size_t i = 0; i < v.size(); ++i) {
                if (i > 0) out += ',';
                if (!serializeImpl(out, v[i])) return false;
            }
            out += ']';
            return true;
        }
        else if constexpr (std::is_same_v<T, JsonObject>) {
            out += '{';
            bool first = true;
            // std::map is already sorted by key (lexicographic ASCII)
            for (const auto& [key, value] : v) {
                if (!first) out += ',';
                first = false;
                appendEscapedString(out, key);
                out += ':';
                if (!serializeImpl(out, value)) return false;
            }
            out += '}';
            return true;
        }
        else {
            return false;
        }
    }, val.value);
}

} // anonymous namespace

std::string serializeCanonical(const JsonValue& val) {
    std::string out;
    if (!serializeImpl(out, val))
        return {};
    return out;
}

std::string serializeCanonical(const JsonObject& obj) {
    return serializeCanonical(JsonValue(obj));
}

} // namespace kzn
