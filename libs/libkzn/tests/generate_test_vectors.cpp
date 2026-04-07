// Generates all .kzn test vector files programmatically.
// Uses the TEST-ONLY keypair from keypair.json.

#include "kzn/KznFormat.h"
#include "KznCanonicalJson.h"
#include "monocypher.h"
#include "monocypher-ed25519.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// Test keypair (from keypair.json — TEST ONLY)
static const char* kPrivateKeyHex = "1b6ca1d8cf888c1d2a6ef7e51797b01efffb5d1fbda6a7f4365be51e9bcad6b1";
static const char* kPublicKeyHex  = "327f37657868e9e65b125b070903461464d0e556cd1928bbbed4ac509429db5f";

static uint8_t hexCharToNibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    return 0;
}

static void hexToBytes(const char* hex, uint8_t* out, size_t outLen) {
    for (size_t i = 0; i < outLen; i++) {
        out[i] = (hexCharToNibble(hex[i * 2]) << 4) | hexCharToNibble(hex[i * 2 + 1]);
    }
}

// Build a sample preset JSON payload using canonical emitter
static std::string buildSamplePayload() {
    kzn::JsonObject engine;
    engine["color"]  = kzn::JsonValue(16.0);
    engine["depth"]  = kzn::JsonValue(21.0);
    engine["hq"]     = kzn::JsonValue(false);
    engine["id"]     = kzn::JsonValue("green");
    engine["mix"]    = kzn::JsonValue(50.0);
    engine["offset"] = kzn::JsonValue(33.0);
    engine["rate"]   = kzn::JsonValue(1.2);
    engine["width"]  = kzn::JsonValue(150.0);

    kzn::JsonObject plugin;
    plugin["id"]      = kzn::JsonValue("choroboros");
    plugin["version"] = kzn::JsonValue("1.0");

    kzn::JsonArray tags;
    tags.push_back(kzn::JsonValue("pad"));
    tags.push_back(kzn::JsonValue("ambient"));

    kzn::JsonObject preset;
    preset["description"] = kzn::JsonValue("");
    preset["name"]        = kzn::JsonValue("Warm Pad");
    preset["tags"]        = kzn::JsonValue(std::move(tags));

    kzn::JsonObject root;
    root["assets"]         = kzn::JsonValue(kzn::JsonObject{});
    root["created_at"]     = kzn::JsonValue("2026-04-01T14:32:00Z");
    root["creator_id"]     = kzn::JsonValue("abc-123");
    root["engine"]         = kzn::JsonValue(std::move(engine));
    root["file_id"]        = kzn::JsonValue("def-456");
    root["generator"]      = kzn::JsonValue("Choroboros 1.0.0");
    root["key_id"]         = kzn::JsonValue(1);
    root["plugin"]         = kzn::JsonValue(std::move(plugin));
    root["preset"]         = kzn::JsonValue(std::move(preset));
    root["schema_version"] = kzn::JsonValue("1.0");
    root["type"]           = kzn::JsonValue("preset");

    return kzn::serializeCanonical(root);
}

// Sign data with the test private key using Monocypher v4
// Monocypher v4 crypto_eddsa_sign requires a 64-byte expanded secret key.
// crypto_eddsa_key_pair(secret_key[64], public_key[32], seed[32]) expands the seed.
static void signData(const uint8_t* message, size_t messageLen,
                     const uint8_t seed[32], uint8_t signature[64]) {
    uint8_t secretKey[64];
    uint8_t publicKey[32];
    uint8_t seedCopy[32];
    std::memcpy(seedCopy, seed, 32);
    crypto_ed25519_key_pair(secretKey, publicKey, seedCopy);
    crypto_ed25519_sign(signature, secretKey, message, messageLen);
    crypto_wipe(secretKey, 64);
}

static bool writeRawFile(const std::string& path, const uint8_t* data, size_t len) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
    return f.good();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: generate_test_vectors <output_dir>\n");
        return 1;
    }

    std::string outDir = argv[1];

    // Load test keys
    uint8_t privateKey[32], publicKey[32];
    hexToBytes(kPrivateKeyHex, privateKey, 32);
    hexToBytes(kPublicKeyHex, publicKey, 32);

    // Verify keypair is correct using Monocypher v4 API
    {
        uint8_t seedCopy[32], expandedKey[64], derivedPub[32];
        std::memcpy(seedCopy, privateKey, 32);
        crypto_ed25519_key_pair(expandedKey, derivedPub, seedCopy);
        if (std::memcmp(derivedPub, publicKey, 32) != 0) {
            std::fprintf(stderr, "ERROR: Keypair verification failed!\n");
            return 1;
        }
        crypto_wipe(expandedKey, 64);
    }

    std::string payload = buildSamplePayload();
    std::printf("Sample payload (%zu bytes): %s\n", payload.size(), payload.c_str());

    // ---- 1. valid_server_signed.kzn ----
    {
        kzn::KznFileData data;
        data.jsonPayload   = payload;
        data.sigScheme     = kzn::KZN_SIG_SCHEME_SERVER;
        data.flags         = kzn::KZN_FLAG_HAS_SIGNATURE | kzn::KZN_FLAG_PAYLOAD_IS_CANONICAL;
        data.keyId         = 1;
        data.formatVersion = kzn::KZN_FORMAT_VERSION;
        data.c14nScheme    = kzn::KZN_C14N_RAW_BYTES;
        data.payloadType   = kzn::KZN_PAYLOAD_TYPE_JSON;

        // Build signing message (with sig zeroed)
        auto sigMsg = kzn::buildSigningMessage(data);

        // Sign
        signData(sigMsg.data(), sigMsg.size(), privateKey, data.signature);

        auto err = kzn::writeKznFile(outDir + "/valid_server_signed.kzn", data);
        std::printf("valid_server_signed.kzn: %s\n", err == kzn::KznError::OK ? "OK" : "FAILED");
    }

    // ---- 2. valid_unsigned.kzn ----
    {
        kzn::KznFileData data;
        data.jsonPayload   = payload;
        data.sigScheme     = kzn::KZN_SIG_SCHEME_UNSIGNED;
        data.flags         = kzn::KZN_FLAG_PAYLOAD_IS_CANONICAL;
        data.keyId         = 0;
        data.formatVersion = kzn::KZN_FORMAT_VERSION;
        std::memset(data.signature, 0, 64);

        auto err = kzn::writeKznFile(outDir + "/valid_unsigned.kzn", data);
        std::printf("valid_unsigned.kzn: %s\n", err == kzn::KznError::OK ? "OK" : "FAILED");
    }

    // ---- 3. tampered_payload.kzn ----
    {
        kzn::KznFileData data;
        data.jsonPayload   = payload;
        data.sigScheme     = kzn::KZN_SIG_SCHEME_SERVER;
        data.flags         = kzn::KZN_FLAG_HAS_SIGNATURE | kzn::KZN_FLAG_PAYLOAD_IS_CANONICAL;
        data.keyId         = 1;
        data.formatVersion = kzn::KZN_FORMAT_VERSION;

        // Sign with correct payload first
        auto sigMsg = kzn::buildSigningMessage(data);
        signData(sigMsg.data(), sigMsg.size(), privateKey, data.signature);

        // Now tamper with the payload (flip one byte)
        if (!data.jsonPayload.empty()) {
            data.jsonPayload[data.jsonPayload.size() / 2] ^= 0x01;
        }

        auto err = kzn::writeKznFile(outDir + "/tampered_payload.kzn", data);
        std::printf("tampered_payload.kzn: %s\n", err == kzn::KznError::OK ? "OK" : "FAILED");
    }

    // ---- 4. tampered_header.kzn ----
    {
        kzn::KznFileData data;
        data.jsonPayload   = payload;
        data.sigScheme     = kzn::KZN_SIG_SCHEME_SERVER;
        data.flags         = kzn::KZN_FLAG_HAS_SIGNATURE | kzn::KZN_FLAG_PAYLOAD_IS_CANONICAL;
        data.keyId         = 1;
        data.formatVersion = kzn::KZN_FORMAT_VERSION;

        // Sign correctly
        auto sigMsg = kzn::buildSigningMessage(data);
        signData(sigMsg.data(), sigMsg.size(), privateKey, data.signature);

        // Write to file, then tamper with a non-signature header byte
        auto err = kzn::writeKznFile(outDir + "/tampered_header.kzn", data);
        if (err == kzn::KznError::OK) {
            // Reopen and flip a flags byte (offset 8)
            std::fstream f(outDir + "/tampered_header.kzn",
                           std::ios::binary | std::ios::in | std::ios::out);
            if (f) {
                f.seekp(8);
                uint8_t b;
                f.read(reinterpret_cast<char*>(&b), 1);
                b ^= 0x08; // flip a flag bit
                f.seekp(8);
                f.write(reinterpret_cast<const char*>(&b), 1);
            }
        }
        std::printf("tampered_header.kzn: %s\n", err == kzn::KznError::OK ? "OK" : "FAILED");
    }

    // ---- 5. wrong_key_id.kzn ----
    {
        kzn::KznFileData data;
        data.jsonPayload   = payload;
        data.sigScheme     = kzn::KZN_SIG_SCHEME_SERVER;
        data.flags         = kzn::KZN_FLAG_HAS_SIGNATURE | kzn::KZN_FLAG_PAYLOAD_IS_CANONICAL;
        data.keyId         = 99; // Unknown key_id
        data.formatVersion = kzn::KZN_FORMAT_VERSION;

        auto sigMsg = kzn::buildSigningMessage(data);
        signData(sigMsg.data(), sigMsg.size(), privateKey, data.signature);

        auto err = kzn::writeKznFile(outDir + "/wrong_key_id.kzn", data);
        std::printf("wrong_key_id.kzn: %s\n", err == kzn::KznError::OK ? "OK" : "FAILED");
    }

    // ---- 6. truncated.kzn ----
    {
        kzn::KznFileData data;
        data.jsonPayload   = payload;
        data.sigScheme     = kzn::KZN_SIG_SCHEME_UNSIGNED;
        data.formatVersion = kzn::KZN_FORMAT_VERSION;

        // Write full file, then truncate it mid-payload
        auto err = kzn::writeKznFile(outDir + "/truncated.kzn", data);
        if (err == kzn::KznError::OK) {
            // Truncate to header + half payload
            size_t truncSize = kzn::KZN_HEADER_SIZE_V1 + data.jsonPayload.size() / 2;
            // Read, truncate, rewrite
            std::ifstream fin(outDir + "/truncated.kzn", std::ios::binary);
            std::vector<uint8_t> buf(truncSize);
            fin.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(truncSize));
            fin.close();
            writeRawFile(outDir + "/truncated.kzn", buf.data(), truncSize);
        }
        std::printf("truncated.kzn: %s\n", err == kzn::KznError::OK ? "OK" : "FAILED");
    }

    // ---- 7. bad_magic.kzn ----
    {
        kzn::KznFileData data;
        data.jsonPayload   = payload;
        data.sigScheme     = kzn::KZN_SIG_SCHEME_UNSIGNED;
        data.formatVersion = kzn::KZN_FORMAT_VERSION;

        auto err = kzn::writeKznFile(outDir + "/bad_magic.kzn", data);
        if (err == kzn::KznError::OK) {
            // Overwrite magic bytes
            std::fstream f(outDir + "/bad_magic.kzn",
                           std::ios::binary | std::ios::in | std::ios::out);
            if (f) {
                const char badMagic[] = "BAD!";
                f.seekp(0);
                f.write(badMagic, 4);
            }
        }
        std::printf("bad_magic.kzn: %s\n", err == kzn::KznError::OK ? "OK" : "FAILED");
    }

    // ---- 8. oversized.kzn ----
    {
        kzn::KznFileData data;
        // Create payload > 64KB
        data.jsonPayload = std::string(kzn::KZN_MAX_PAYLOAD_SIZE + 1, 'X');
        data.sigScheme     = kzn::KZN_SIG_SCHEME_UNSIGNED;
        data.formatVersion = kzn::KZN_FORMAT_VERSION;

        // writeKznFile will reject this, so we build manually
        uint8_t header[kzn::KZN_HEADER_SIZE_V1];
        kzn::buildHeader(data, header);

        std::ofstream f(outDir + "/oversized.kzn", std::ios::binary);
        if (f) {
            f.write(reinterpret_cast<const char*>(header), kzn::KZN_HEADER_SIZE_V1);
            f.write(data.jsonPayload.data(), static_cast<std::streamsize>(data.jsonPayload.size()));
            std::printf("oversized.kzn: OK\n");
        } else {
            std::printf("oversized.kzn: FAILED\n");
        }
    }

    std::printf("\nAll test vectors generated in %s/\n", outDir.c_str());

    // Clean up sensitive key material
    crypto_wipe(privateKey, 32);

    return 0;
}
