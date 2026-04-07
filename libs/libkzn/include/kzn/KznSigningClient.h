#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace kzn {

// Plugin provides its own HTTP implementation via this callback
using HttpPostFn = std::function<void(
    const std::string& url,
    const std::string& authHeader,
    const std::string& body,
    std::function<void(int httpStatus, const std::string& response)> callback
)>;

void requestSignature(
    HttpPostFn httpPost,
    const std::string& endpoint,
    const std::string& bearerToken,
    const std::string& signingMessageBase64,
    uint32_t keyId,
    std::function<void(const uint8_t* sig, bool success, const std::string& error)> callback
);

} // namespace kzn
