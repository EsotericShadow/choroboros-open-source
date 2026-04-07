#include "kzn/KznSigningClient.h"
#include <cstring>
#include <sstream>

namespace kzn {

// Base64 encoding table
static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const uint8_t* data, size_t len) {
    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

        result += kBase64Chars[(n >> 18) & 0x3F];
        result += kBase64Chars[(n >> 12) & 0x3F];
        result += (i + 1 < len) ? kBase64Chars[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? kBase64Chars[n & 0x3F] : '=';
    }
    return result;
}

static int base64DecodeChar(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static bool base64Decode(const std::string& input, std::vector<uint8_t>& out) {
    out.clear();
    out.reserve((input.size() / 4) * 3);

    for (size_t i = 0; i < input.size(); i += 4) {
        if (i + 3 >= input.size()) return false;

        int a = base64DecodeChar(input[i]);
        int b = base64DecodeChar(input[i + 1]);
        if (a < 0 || b < 0) return false;

        out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));

        if (input[i + 2] != '=') {
            int c = base64DecodeChar(input[i + 2]);
            if (c < 0) return false;
            out.push_back(static_cast<uint8_t>(((b & 0x0F) << 4) | (c >> 2)));

            if (input[i + 3] != '=') {
                int d = base64DecodeChar(input[i + 3]);
                if (d < 0) return false;
                out.push_back(static_cast<uint8_t>(((c & 0x03) << 6) | d));
            }
        }
    }
    return true;
}

void requestSignature(
    HttpPostFn httpPost,
    const std::string& endpoint,
    const std::string& bearerToken,
    const std::string& signingMessageBase64,
    uint32_t keyId,
    std::function<void(const uint8_t* sig, bool success, const std::string& error)> callback)
{
    // Build request JSON
    std::string body = "{\"schema_version\":1,\"message_b64\":\"";
    body += signingMessageBase64;
    body += "\",\"key_id\":";
    body += std::to_string(keyId);
    body += "}";

    std::string authHeader = "Bearer " + bearerToken;

    httpPost(endpoint, authHeader, body,
        [callback](int httpStatus, const std::string& response) {
            if (httpStatus != 200) {
                callback(nullptr, false, "HTTP " + std::to_string(httpStatus) + ": " + response);
                return;
            }

            // Parse signature_b64 from response JSON
            // Simple extraction — find "signature_b64":"..."
            auto pos = response.find("\"signature_b64\"");
            if (pos == std::string::npos) {
                callback(nullptr, false, "Missing signature_b64 in response");
                return;
            }

            pos = response.find('"', pos + 15);
            if (pos == std::string::npos) {
                callback(nullptr, false, "Malformed response");
                return;
            }
            pos++; // skip opening quote

            auto endPos = response.find('"', pos);
            if (endPos == std::string::npos) {
                callback(nullptr, false, "Malformed response");
                return;
            }

            std::string sigB64 = response.substr(pos, endPos - pos);

            std::vector<uint8_t> sigBytes;
            if (!base64Decode(sigB64, sigBytes) || sigBytes.size() != 64) {
                callback(nullptr, false, "Invalid signature_b64");
                return;
            }

            callback(sigBytes.data(), true, "");
        }
    );
}

} // namespace kzn
