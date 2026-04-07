// test_canonical_json.cpp — Tests for the canonical JSON emitter.

#include "KznCanonicalJson.h"
#include <cassert>
#include <cstdio>
#include <cstring>

#define TEST(name) static void name()
#define RUN(name) do { std::printf("  " #name "... "); name(); std::printf("PASS\n"); } while(0)

TEST(testEmptyObject) {
    kzn::JsonObject obj;
    auto result = kzn::serializeCanonical(obj);
    assert(result == "{}");
}

TEST(testSortedKeys) {
    kzn::JsonObject obj;
    obj["zebra"] = kzn::JsonValue("z");
    obj["alpha"] = kzn::JsonValue("a");
    obj["middle"] = kzn::JsonValue("m");

    auto result = kzn::serializeCanonical(obj);
    assert(result == "{\"alpha\":\"a\",\"middle\":\"m\",\"zebra\":\"z\"}");
}

TEST(testNoWhitespace) {
    kzn::JsonObject obj;
    obj["a"] = kzn::JsonValue(1);
    obj["b"] = kzn::JsonValue(2);
    auto result = kzn::serializeCanonical(obj);
    // Should have no spaces, no newlines
    assert(result.find(' ') == std::string::npos);
    assert(result.find('\n') == std::string::npos);
    assert(result == "{\"a\":1,\"b\":2}");
}

TEST(testNestedObjectsSorted) {
    kzn::JsonObject inner;
    inner["z"] = kzn::JsonValue(1);
    inner["a"] = kzn::JsonValue(2);

    kzn::JsonObject outer;
    outer["inner"] = kzn::JsonValue(std::move(inner));

    auto result = kzn::serializeCanonical(outer);
    assert(result == "{\"inner\":{\"a\":2,\"z\":1}}");
}

TEST(testBooleans) {
    kzn::JsonObject obj;
    obj["f"] = kzn::JsonValue(false);
    obj["t"] = kzn::JsonValue(true);
    auto result = kzn::serializeCanonical(obj);
    assert(result == "{\"f\":false,\"t\":true}");
}

TEST(testNull) {
    kzn::JsonObject obj;
    obj["n"] = kzn::JsonValue(nullptr);
    auto result = kzn::serializeCanonical(obj);
    assert(result == "{\"n\":null}");
}

TEST(testStringEscaping) {
    kzn::JsonObject obj;
    obj["s"] = kzn::JsonValue("hello \"world\"\nline2");
    auto result = kzn::serializeCanonical(obj);
    assert(result == "{\"s\":\"hello \\\"world\\\"\\nline2\"}");
}

TEST(testArray) {
    kzn::JsonArray arr;
    arr.push_back(kzn::JsonValue(1));
    arr.push_back(kzn::JsonValue("two"));
    arr.push_back(kzn::JsonValue(false));

    kzn::JsonObject obj;
    obj["arr"] = kzn::JsonValue(std::move(arr));
    auto result = kzn::serializeCanonical(obj);
    assert(result == "{\"arr\":[1,\"two\",false]}");
}

TEST(testEmptyArray) {
    kzn::JsonObject obj;
    obj["arr"] = kzn::JsonValue(kzn::JsonArray{});
    auto result = kzn::serializeCanonical(obj);
    assert(result == "{\"arr\":[]}");
}

TEST(testIntegerNumbers) {
    kzn::JsonObject obj;
    obj["neg"] = kzn::JsonValue(-42);
    obj["pos"] = kzn::JsonValue(100);
    obj["zero"] = kzn::JsonValue(0);
    auto result = kzn::serializeCanonical(obj);
    assert(result == "{\"neg\":-42,\"pos\":100,\"zero\":0}");
}

TEST(testDoubleNumbers) {
    kzn::JsonObject obj;
    obj["pi"] = kzn::JsonValue(3.14);
    obj["whole"] = kzn::JsonValue(1.0); // Should serialize as "1" (integer form)
    auto result = kzn::serializeCanonical(obj);
    // 1.0 should become "1" since it's an exact integer
    assert(result.find("\"pi\":3.14") != std::string::npos);
    assert(result.find("\"whole\":1") != std::string::npos);
}

TEST(testPresetPayload) {
    // Build the canonical preset JSON from the spec
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

    auto result = kzn::serializeCanonical(root);

    // Verify key ordering: assets < created_at < creator_id < engine < file_id < ...
    auto assetsPos   = result.find("\"assets\"");
    auto createdPos  = result.find("\"created_at\"");
    auto enginePos   = result.find("\"engine\"");
    auto typePos     = result.find("\"type\"");
    assert(assetsPos < createdPos);
    assert(createdPos < enginePos);
    assert(enginePos < typePos);

    // Verify no whitespace between tokens
    assert(result.find(" :") == std::string::npos);
    assert(result.find(": ") == std::string::npos);
    assert(result.find('\n') == std::string::npos);
}

TEST(testDeterminism) {
    // Same input must produce identical output every time
    kzn::JsonObject obj;
    obj["b"] = kzn::JsonValue(2);
    obj["a"] = kzn::JsonValue(1);

    auto r1 = kzn::serializeCanonical(obj);
    auto r2 = kzn::serializeCanonical(obj);
    auto r3 = kzn::serializeCanonical(obj);
    assert(r1 == r2);
    assert(r2 == r3);
}

int main() {
    std::printf("test_canonical_json:\n");

    RUN(testEmptyObject);
    RUN(testSortedKeys);
    RUN(testNoWhitespace);
    RUN(testNestedObjectsSorted);
    RUN(testBooleans);
    RUN(testNull);
    RUN(testStringEscaping);
    RUN(testArray);
    RUN(testEmptyArray);
    RUN(testIntegerNumbers);
    RUN(testDoubleNumbers);
    RUN(testPresetPayload);
    RUN(testDeterminism);

    std::printf("All test_canonical_json tests passed!\n");
    return 0;
}
