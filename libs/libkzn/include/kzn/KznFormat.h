#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace kzn {

// ---- Constants ----

constexpr uint8_t  KZN_MAGIC[4]          = { 0x4B, 0x5A, 0x4E, 0x31 }; // "KZN1"
constexpr uint16_t KZN_HEADER_SIZE_V1    = 128;
constexpr uint16_t KZN_FORMAT_VERSION    = 1;
constexpr uint16_t KZN_SIGNATURE_LENGTH  = 64;
constexpr uint32_t KZN_SIGNATURE_OFFSET  = 64;
constexpr uint32_t KZN_MAX_PAYLOAD_SIZE  = 65536; // 64KB
constexpr uint8_t  KZN_SIG_SCHEME_UNSIGNED = 0;
constexpr uint8_t  KZN_SIG_SCHEME_SERVER   = 1;
constexpr uint8_t  KZN_SIG_SCHEME_SESSION  = 2;
constexpr uint8_t  KZN_C14N_RAW_BYTES     = 0;
constexpr uint8_t  KZN_C14N_JCS           = 1;
constexpr uint8_t  KZN_PAYLOAD_TYPE_JSON  = 1;

// ---- Flags bitfield ----

constexpr uint32_t KZN_FLAG_HAS_SIGNATURE       = (1u << 0);
constexpr uint32_t KZN_FLAG_EXPECT_SESSION_CERT  = (1u << 1);
constexpr uint32_t KZN_FLAG_PAYLOAD_IS_CANONICAL = (1u << 2);
constexpr uint32_t KZN_FLAG_STRICT_IMPORT        = (1u << 3);

// ---- Error codes ----

enum class KznError {
    OK,
    BadMagic,
    UnsupportedVersion,
    TruncatedFile,
    PayloadTooLarge,
    InvalidJson,
    IOError,
    InvalidField,
    NonFiniteNumber
};

// ---- File data ----

struct KznFileData {
    std::string jsonPayload;      // raw JSON bytes
    uint8_t     signature[64]{};  // Ed25519 signature (zeros if unsigned)
    uint16_t    formatVersion = KZN_FORMAT_VERSION;
    uint32_t    flags         = 0;
    uint8_t     sigScheme     = KZN_SIG_SCHEME_UNSIGNED;
    uint8_t     c14nScheme    = KZN_C14N_RAW_BYTES;
    uint8_t     payloadType   = KZN_PAYLOAD_TYPE_JSON;
    uint32_t    keyId         = 0;
};

// ---- Read / Write ----

KznError writeKznFile(const std::string& path, const KznFileData& data);
KznError readKznFile(const std::string& path, KznFileData& outData);

// ---- Header helpers ----

// Build the raw 128-byte header from KznFileData
void buildHeader(const KznFileData& data, uint8_t outHeader[KZN_HEADER_SIZE_V1]);

// Build the signing message: header(sig zeroed) || payload
std::vector<uint8_t> buildSigningMessage(const KznFileData& data);

} // namespace kzn
