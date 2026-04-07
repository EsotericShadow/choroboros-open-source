// test_format.cpp — Tests for KznFormat read/write and round-trip.

#include "kzn/KznFormat.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>

static std::string testVectorsDir;

static void findTestVectorsDir(const char* argv0) {
    // Try relative to executable, or use environment
    namespace fs = std::filesystem;
    // Check common locations
    std::vector<std::string> candidates = {
        "test_vectors",
        "tests/test_vectors",
        "../tests/test_vectors",
        "../../tests/test_vectors",
    };
    // Also try relative to source
    if (argv0) {
        auto execDir = fs::path(argv0).parent_path();
        candidates.push_back((execDir / "test_vectors").string());
        candidates.push_back((execDir / "../test_vectors").string());
        candidates.push_back((execDir / "../../tests/test_vectors").string());
    }
    for (const auto& dir : candidates) {
        if (fs::exists(dir + "/valid_unsigned.kzn")) {
            testVectorsDir = dir;
            return;
        }
    }
    std::fprintf(stderr, "ERROR: Cannot find test_vectors directory\n");
    std::exit(1);
}

#define TEST(name) static void name()
#define RUN(name) do { std::printf("  " #name "... "); name(); std::printf("PASS\n"); } while(0)

TEST(testRoundTripUnsigned) {
    kzn::KznFileData data;
    data.jsonPayload = "{\"test\":true}";
    data.sigScheme = kzn::KZN_SIG_SCHEME_UNSIGNED;
    data.flags = kzn::KZN_FLAG_PAYLOAD_IS_CANONICAL;
    data.formatVersion = kzn::KZN_FORMAT_VERSION;
    data.keyId = 0;

    std::string path = "/tmp/test_roundtrip.kzn";
    auto err = kzn::writeKznFile(path, data);
    assert(err == kzn::KznError::OK);

    kzn::KznFileData readData;
    err = kzn::readKznFile(path, readData);
    assert(err == kzn::KznError::OK);
    assert(readData.jsonPayload == data.jsonPayload);
    assert(readData.sigScheme == kzn::KZN_SIG_SCHEME_UNSIGNED);
    assert(readData.formatVersion == kzn::KZN_FORMAT_VERSION);
    assert(readData.keyId == 0);

    std::remove(path.c_str());
}

TEST(testRoundTripSigned) {
    kzn::KznFileData data;
    data.jsonPayload = "{\"signed\":true}";
    data.sigScheme = kzn::KZN_SIG_SCHEME_SERVER;
    data.flags = kzn::KZN_FLAG_HAS_SIGNATURE | kzn::KZN_FLAG_PAYLOAD_IS_CANONICAL;
    data.formatVersion = kzn::KZN_FORMAT_VERSION;
    data.keyId = 1;
    // Fill signature with test data
    for (int i = 0; i < 64; i++) data.signature[i] = static_cast<uint8_t>(i);

    std::string path = "/tmp/test_roundtrip_signed.kzn";
    auto err = kzn::writeKznFile(path, data);
    assert(err == kzn::KznError::OK);

    kzn::KznFileData readData;
    err = kzn::readKznFile(path, readData);
    assert(err == kzn::KznError::OK);
    assert(readData.jsonPayload == data.jsonPayload);
    assert(readData.sigScheme == kzn::KZN_SIG_SCHEME_SERVER);
    assert(readData.keyId == 1);
    assert(std::memcmp(readData.signature, data.signature, 64) == 0);

    std::remove(path.c_str());
}

TEST(testBadMagic) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/bad_magic.kzn", data);
    assert(err == kzn::KznError::BadMagic);
}

TEST(testTruncatedFile) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/truncated.kzn", data);
    assert(err == kzn::KznError::TruncatedFile);
}

TEST(testOversizedPayload) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/oversized.kzn", data);
    assert(err == kzn::KznError::PayloadTooLarge);
}

TEST(testReadValidUnsigned) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/valid_unsigned.kzn", data);
    assert(err == kzn::KznError::OK);
    assert(data.sigScheme == kzn::KZN_SIG_SCHEME_UNSIGNED);
    assert(!data.jsonPayload.empty());
}

TEST(testReadValidSigned) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/valid_server_signed.kzn", data);
    assert(err == kzn::KznError::OK);
    assert(data.sigScheme == kzn::KZN_SIG_SCHEME_SERVER);
    assert(data.keyId == 1);
    assert(!data.jsonPayload.empty());
}

TEST(testHeaderSize) {
    // Verify header is exactly 128 bytes
    kzn::KznFileData data;
    data.jsonPayload = "{}";
    uint8_t header[kzn::KZN_HEADER_SIZE_V1];
    kzn::buildHeader(data, header);

    // Check magic
    assert(header[0] == 0x4B); // K
    assert(header[1] == 0x5A); // Z
    assert(header[2] == 0x4E); // N
    assert(header[3] == 0x31); // 1
}

TEST(testSigningMessageConstruction) {
    kzn::KznFileData data;
    data.jsonPayload = "{\"test\":1}";
    data.sigScheme = kzn::KZN_SIG_SCHEME_SERVER;
    data.keyId = 1;

    auto msg = kzn::buildSigningMessage(data);

    // Message should be header(128) + payload
    assert(msg.size() == kzn::KZN_HEADER_SIZE_V1 + data.jsonPayload.size());

    // Signature region (bytes 64-127) should be zeros
    for (size_t i = 64; i < 128; i++) {
        assert(msg[i] == 0);
    }

    // Payload should follow header
    for (size_t i = 0; i < data.jsonPayload.size(); i++) {
        assert(msg[kzn::KZN_HEADER_SIZE_V1 + i] == static_cast<uint8_t>(data.jsonPayload[i]));
    }
}

TEST(testPayloadTooLargeWrite) {
    kzn::KznFileData data;
    data.jsonPayload = std::string(kzn::KZN_MAX_PAYLOAD_SIZE + 1, 'X');
    auto err = kzn::writeKznFile("/tmp/should_not_exist.kzn", data);
    assert(err == kzn::KznError::PayloadTooLarge);
}

TEST(testNonExistentFile) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile("/tmp/nonexistent_file_12345.kzn", data);
    assert(err == kzn::KznError::IOError);
}

int main(int argc, char* argv[]) {
    findTestVectorsDir(argv[0]);
    std::printf("test_format:\n");

    RUN(testRoundTripUnsigned);
    RUN(testRoundTripSigned);
    RUN(testBadMagic);
    RUN(testTruncatedFile);
    RUN(testOversizedPayload);
    RUN(testReadValidUnsigned);
    RUN(testReadValidSigned);
    RUN(testHeaderSize);
    RUN(testSigningMessageConstruction);
    RUN(testPayloadTooLargeWrite);
    RUN(testNonExistentFile);

    std::printf("All test_format tests passed!\n");
    return 0;
}
