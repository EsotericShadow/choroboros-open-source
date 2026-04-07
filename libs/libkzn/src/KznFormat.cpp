#include "kzn/KznFormat.h"
#include <cstring>
#include <fstream>

namespace kzn {

namespace {

// Little-endian read/write helpers
inline void writeLE16(uint8_t* dst, uint16_t v) {
    dst[0] = static_cast<uint8_t>(v);
    dst[1] = static_cast<uint8_t>(v >> 8);
}

inline void writeLE32(uint8_t* dst, uint32_t v) {
    dst[0] = static_cast<uint8_t>(v);
    dst[1] = static_cast<uint8_t>(v >> 8);
    dst[2] = static_cast<uint8_t>(v >> 16);
    dst[3] = static_cast<uint8_t>(v >> 24);
}

inline uint16_t readLE16(const uint8_t* src) {
    return static_cast<uint16_t>(src[0]) |
           (static_cast<uint16_t>(src[1]) << 8);
}

inline uint32_t readLE32(const uint8_t* src) {
    return static_cast<uint32_t>(src[0]) |
           (static_cast<uint32_t>(src[1]) << 8) |
           (static_cast<uint32_t>(src[2]) << 16) |
           (static_cast<uint32_t>(src[3]) << 24);
}

} // anonymous namespace

void buildHeader(const KznFileData& data, uint8_t outHeader[KZN_HEADER_SIZE_V1]) {
    std::memset(outHeader, 0, KZN_HEADER_SIZE_V1);

    // 0x00: magic "KZN1"
    std::memcpy(outHeader + 0, KZN_MAGIC, 4);
    // 0x04: header_size
    writeLE16(outHeader + 4, KZN_HEADER_SIZE_V1);
    // 0x06: format_version
    writeLE16(outHeader + 6, data.formatVersion);
    // 0x08: flags
    writeLE32(outHeader + 8, data.flags);
    // 0x0C: sig_scheme
    outHeader[12] = data.sigScheme;
    // 0x0D: c14n_scheme
    outHeader[13] = data.c14nScheme;
    // 0x0E: payload_type
    outHeader[14] = data.payloadType;
    // 0x0F: reserved0
    outHeader[15] = 0;
    // 0x10: key_id
    writeLE32(outHeader + 16, data.keyId);
    // 0x14: payload_offset
    writeLE32(outHeader + 20, KZN_HEADER_SIZE_V1);
    // 0x18: payload_length
    writeLE32(outHeader + 24, static_cast<uint32_t>(data.jsonPayload.size()));
    // 0x1C: signature_offset
    writeLE32(outHeader + 28, KZN_SIGNATURE_OFFSET);
    // 0x20: signature_length
    writeLE32(outHeader + 32, KZN_SIGNATURE_LENGTH);
    // 0x24: footer_length
    writeLE32(outHeader + 36, 0);
    // 0x28: reserved1 (24 bytes) — already zeroed
    // 0x40: signature (64 bytes)
    std::memcpy(outHeader + KZN_SIGNATURE_OFFSET, data.signature, 64);
}

std::vector<uint8_t> buildSigningMessage(const KznFileData& data) {
    uint8_t header[KZN_HEADER_SIZE_V1];
    buildHeader(data, header);

    // Zero the signature region (bytes 64-127)
    std::memset(header + KZN_SIGNATURE_OFFSET, 0, KZN_SIGNATURE_LENGTH);

    // signing_message = header(sig zeroed) || payload
    std::vector<uint8_t> msg;
    msg.reserve(KZN_HEADER_SIZE_V1 + data.jsonPayload.size());
    msg.insert(msg.end(), header, header + KZN_HEADER_SIZE_V1);
    msg.insert(msg.end(), data.jsonPayload.begin(), data.jsonPayload.end());
    return msg;
}

KznError writeKznFile(const std::string& path, const KznFileData& data) {
    if (data.jsonPayload.size() > KZN_MAX_PAYLOAD_SIZE)
        return KznError::PayloadTooLarge;

    uint8_t header[KZN_HEADER_SIZE_V1];
    buildHeader(data, header);

    std::ofstream file(path, std::ios::binary);
    if (!file)
        return KznError::IOError;

    file.write(reinterpret_cast<const char*>(header), KZN_HEADER_SIZE_V1);
    file.write(data.jsonPayload.data(), static_cast<std::streamsize>(data.jsonPayload.size()));

    if (!file.good())
        return KznError::IOError;

    return KznError::OK;
}

KznError readKznFile(const std::string& path, KznFileData& outData) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return KznError::IOError;

    auto fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0);

    // Must have at least a full header
    if (fileSize < KZN_HEADER_SIZE_V1)
        return KznError::TruncatedFile;

    uint8_t header[KZN_HEADER_SIZE_V1];
    file.read(reinterpret_cast<char*>(header), KZN_HEADER_SIZE_V1);
    if (!file.good())
        return KznError::IOError;

    // Validate magic
    if (std::memcmp(header, KZN_MAGIC, 4) != 0)
        return KznError::BadMagic;

    // Read header fields
    uint16_t headerSize    = readLE16(header + 4);
    uint16_t formatVersion = readLE16(header + 6);
    uint32_t flags         = readLE32(header + 8);
    uint8_t  sigScheme     = header[12];
    uint8_t  c14nScheme    = header[13];
    uint8_t  payloadType   = header[14];
    uint32_t keyId         = readLE32(header + 16);
    uint32_t payloadOffset = readLE32(header + 20);
    uint32_t payloadLength = readLE32(header + 24);

    // Validate header size
    if (headerSize < KZN_HEADER_SIZE_V1)
        return KznError::TruncatedFile;

    // Validate version (reject if major > supported)
    if (formatVersion > KZN_FORMAT_VERSION)
        return KznError::UnsupportedVersion;

    // Validate payload bounds
    if (payloadLength > KZN_MAX_PAYLOAD_SIZE)
        return KznError::PayloadTooLarge;

    if (static_cast<size_t>(payloadOffset) + payloadLength > fileSize)
        return KznError::TruncatedFile;

    // Read payload
    std::string payload(payloadLength, '\0');
    file.seekg(payloadOffset);
    file.read(payload.data(), payloadLength);
    if (!file.good() && payloadLength > 0)
        return KznError::IOError;

    // Populate output
    outData.jsonPayload   = std::move(payload);
    outData.formatVersion = formatVersion;
    outData.flags         = flags;
    outData.sigScheme     = sigScheme;
    outData.c14nScheme    = c14nScheme;
    outData.payloadType   = payloadType;
    outData.keyId         = keyId;
    std::memcpy(outData.signature, header + KZN_SIGNATURE_OFFSET, 64);

    return KznError::OK;
}

} // namespace kzn
