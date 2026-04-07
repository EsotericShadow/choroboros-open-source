#pragma once

#include <cstdint>
#include <string>

namespace kzn {

// Session certificates — Milestone B (stub)

struct SessionCertificate {
    uint8_t  ephemeralPubKey[32]{};
    uint8_t  ephemeralPrivKey[32]{};
    uint8_t  serverCertSignature[64]{};
    int64_t  expiresAtUnixMs = 0;
    int32_t  usageCap        = 0;
    int32_t  usageCount      = 0;
    uint32_t keyId           = 0;
    std::string certId;
};

// Stub — returns false, not yet implemented
bool signWithSessionCert(
    const SessionCertificate& cert,
    const uint8_t* signingMessage,
    size_t signingMessageLen,
    uint8_t outSignature[64]
);

} // namespace kzn
