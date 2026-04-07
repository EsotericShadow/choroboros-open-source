#include "kzn/KznVerifier.h"
#include "kzn/KznFormat.h"
#include "monocypher.h"
#include "monocypher-ed25519.h"
#include <cstring>

namespace kzn {

// ---- XOR-obfuscated test public key (key_id = 1) ----
// Public key hex: 327f37657868e9e65b125b070903461464d0e556cd1928bbbed4ac509429db5f
// XOR mask: random bytes used to obfuscate at rest

static const uint8_t kXorMask1[32] = {
    0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18,
    0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90,
    0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18,
    0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90
};

// public_key XOR mask = obfuscated bytes
// 0x32^0xa1=0x93, 0x7f^0xb2=0xcd, 0x37^0xc3=0xf4, 0x65^0xd4=0xb1,
// 0x78^0xe5=0x9d, 0x68^0xf6=0x9e, 0xe9^0x07=0xee, 0xe6^0x18=0xfe,
// 0x5b^0x29=0x72, 0x12^0x3a=0x28, 0x5b^0x4b=0x10, 0x07^0x5c=0x5b,
// 0x09^0x6d=0x64, 0x03^0x7e=0x7d, 0x46^0x8f=0xc9, 0x14^0x90=0x84,
// 0x64^0xa1=0xc5, 0xd0^0xb2=0x62, 0xe5^0xc3=0x26, 0x56^0xd4=0x82,
// 0xcd^0xe5=0x28, 0x19^0xf6=0xef, 0x28^0x07=0x2f, 0xbb^0x18=0xa3,
// 0xbe^0x29=0x97, 0xd4^0x3a=0xee, 0xac^0x4b=0xe7, 0x50^0x5c=0x0c,
// 0x94^0x6d=0xf9, 0x29^0x7e=0x57, 0xdb^0x8f=0x54, 0x5f^0x90=0xcf
static const uint8_t kObfuscatedKey1[32] = {
    0x93, 0xcd, 0xf4, 0xb1, 0x9d, 0x9e, 0xee, 0xfe,
    0x72, 0x28, 0x10, 0x5b, 0x64, 0x7d, 0xc9, 0x84,
    0xc5, 0x62, 0x26, 0x82, 0x28, 0xef, 0x2f, 0xa3,
    0x97, 0xee, 0xe7, 0x0c, 0xf9, 0x57, 0x54, 0xcf
};

struct KznTrustEntry {
    uint32_t keyId;
    const uint8_t* obfuscatedKey;
    const uint8_t* xorMask;
};

static const KznTrustEntry kTrustStore[] = {
    { 1, kObfuscatedKey1, kXorMask1 },
};

static bool deobfuscateKey(uint32_t keyId, uint8_t out[32]) {
    for (const auto& entry : kTrustStore) {
        if (entry.keyId == keyId) {
            for (int i = 0; i < 32; i++)
                out[i] = entry.obfuscatedKey[i] ^ entry.xorMask[i];
            return true;
        }
    }
    return false;
}

static bool isSignatureAllZeros(const uint8_t sig[64]) {
    for (int i = 0; i < 64; i++) {
        if (sig[i] != 0) return false;
    }
    return true;
}

KznSignatureReport verifySignatureWithKey(const KznFileData& data,
                                          const uint8_t publicKey[32]) {
    KznSignatureReport report;
    report.keyId = data.keyId;

    // If unsigned
    if (data.sigScheme == KZN_SIG_SCHEME_UNSIGNED || isSignatureAllZeros(data.signature)) {
        report.status = KznSignatureStatus::Unsigned;
        return report;
    }

    // Session cert — not yet implemented
    if (data.sigScheme == KZN_SIG_SCHEME_SESSION) {
        report.status = KznSignatureStatus::Malformed;
        report.reason = "Session certificate verification not yet implemented";
        return report;
    }

    // Server signature verification
    if (data.sigScheme != KZN_SIG_SCHEME_SERVER) {
        report.status = KznSignatureStatus::Malformed;
        report.reason = "Unknown sig_scheme";
        return report;
    }

    // Mandatory bounds checks
    if (data.jsonPayload.size() > KZN_MAX_PAYLOAD_SIZE) {
        report.status = KznSignatureStatus::Malformed;
        report.reason = "Payload too large";
        return report;
    }

    // Build signing message
    auto signingMessage = buildSigningMessage(data);

    // Verify with Monocypher
    int result = crypto_ed25519_check(
        data.signature,
        publicKey,
        signingMessage.data(),
        signingMessage.size()
    );

    if (result == 0) {
        report.status = KznSignatureStatus::ValidServer;
    } else {
        report.status = KznSignatureStatus::InvalidSignature;
        report.reason = "Signature verification failed";
    }

    return report;
}

KznSignatureReport verifySignature(const KznFileData& data) {
    KznSignatureReport report;
    report.keyId = data.keyId;

    // If unsigned
    if (data.sigScheme == KZN_SIG_SCHEME_UNSIGNED || isSignatureAllZeros(data.signature)) {
        report.status = KznSignatureStatus::Unsigned;
        return report;
    }

    // Look up public key by key_id
    uint8_t publicKey[32];
    if (!deobfuscateKey(data.keyId, publicKey)) {
        report.status = KznSignatureStatus::UnknownKeyId;
        report.reason = "Unknown key_id: " + std::to_string(data.keyId);
        return report;
    }

    return verifySignatureWithKey(data, publicKey);
}

} // namespace kzn
