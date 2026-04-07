#pragma once

#include "KznFormat.h"
#include <string>

namespace kzn {

enum class KznSignatureStatus {
    Unsigned,
    ValidServer,
    ValidSession,
    InvalidSignature,
    FormatError,
    UnknownKeyId,
    UnsupportedVersion,
    Malformed
};

struct KznSignatureReport {
    KznSignatureStatus status;
    uint32_t           keyId = 0;
    std::string        reason;
};

// Verify using the built-in trust store
KznSignatureReport verifySignature(const KznFileData& data);

// Verify using an explicit public key (for testing)
KznSignatureReport verifySignatureWithKey(const KznFileData& data,
                                          const uint8_t publicKey[32]);

} // namespace kzn
