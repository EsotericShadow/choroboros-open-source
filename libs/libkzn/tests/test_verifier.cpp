// test_verifier.cpp — Tests for KznVerifier signature verification.

#include "kzn/KznVerifier.h"
#include "kzn/KznFormat.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

static std::string testVectorsDir;

static void findTestVectorsDir(const char* argv0) {
    namespace fs = std::filesystem;
    std::vector<std::string> candidates = {
        "test_vectors",
        "tests/test_vectors",
        "../tests/test_vectors",
        "../../tests/test_vectors",
    };
    if (argv0) {
        auto execDir = fs::path(argv0).parent_path();
        candidates.push_back((execDir / "test_vectors").string());
        candidates.push_back((execDir / "../test_vectors").string());
        candidates.push_back((execDir / "../../tests/test_vectors").string());
    }
    for (const auto& dir : candidates) {
        if (fs::exists(dir + "/valid_server_signed.kzn")) {
            testVectorsDir = dir;
            return;
        }
    }
    std::fprintf(stderr, "ERROR: Cannot find test_vectors directory\n");
    std::exit(1);
}

// Test public key hex: 327f37657868e9e65b125b070903461464d0e556cd1928bbbed4ac509429db5f
static uint8_t hexToNibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    return 0;
}

static void hexToBytes(const char* hex, uint8_t* out, size_t len) {
    for (size_t i = 0; i < len; i++)
        out[i] = (hexToNibble(hex[i*2]) << 4) | hexToNibble(hex[i*2+1]);
}

#define TEST(name) static void name()
#define RUN(name) do { std::printf("  " #name "... "); name(); std::printf("PASS\n"); } while(0)

TEST(testValidServerSignature) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/valid_server_signed.kzn", data);
    assert(err == kzn::KznError::OK);

    auto report = kzn::verifySignature(data);
    assert(report.status == kzn::KznSignatureStatus::ValidServer);
    assert(report.keyId == 1);
}

TEST(testValidServerSignatureWithExplicitKey) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/valid_server_signed.kzn", data);
    assert(err == kzn::KznError::OK);

    uint8_t pubKey[32];
    hexToBytes("327f37657868e9e65b125b070903461464d0e556cd1928bbbed4ac509429db5f", pubKey, 32);

    auto report = kzn::verifySignatureWithKey(data, pubKey);
    assert(report.status == kzn::KznSignatureStatus::ValidServer);
}

TEST(testTamperedPayload) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/tampered_payload.kzn", data);
    assert(err == kzn::KznError::OK);

    auto report = kzn::verifySignature(data);
    assert(report.status == kzn::KznSignatureStatus::InvalidSignature);
}

TEST(testTamperedHeader) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/tampered_header.kzn", data);
    assert(err == kzn::KznError::OK);

    auto report = kzn::verifySignature(data);
    assert(report.status == kzn::KznSignatureStatus::InvalidSignature);
}

TEST(testUnknownKeyId) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/wrong_key_id.kzn", data);
    assert(err == kzn::KznError::OK);

    auto report = kzn::verifySignature(data);
    assert(report.status == kzn::KznSignatureStatus::UnknownKeyId);
}

TEST(testUnsignedFile) {
    kzn::KznFileData data;
    auto err = kzn::readKznFile(testVectorsDir + "/valid_unsigned.kzn", data);
    assert(err == kzn::KznError::OK);

    auto report = kzn::verifySignature(data);
    assert(report.status == kzn::KznSignatureStatus::Unsigned);
}

int main(int argc, char* argv[]) {
    findTestVectorsDir(argv[0]);
    std::printf("test_verifier:\n");

    RUN(testValidServerSignature);
    RUN(testValidServerSignatureWithExplicitKey);
    RUN(testTamperedPayload);
    RUN(testTamperedHeader);
    RUN(testUnknownKeyId);
    RUN(testUnsignedFile);

    std::printf("All test_verifier tests passed!\n");
    return 0;
}
