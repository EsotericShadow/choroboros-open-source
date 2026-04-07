
# Test vector generator — builds .kzn test files
add_executable(generate_test_vectors
    generate_test_vectors.cpp
    ../src/KznFormat.cpp
    ../src/KznCanonicalJson.cpp
    ../vendor/monocypher/monocypher.c
    ../vendor/monocypher/monocypher-ed25519.c
)
target_include_directories(generate_test_vectors PRIVATE
    ../include
    ../vendor/monocypher
    ../src
)

# Format tests
add_executable(test_format
    test_format.cpp
    ../src/KznFormat.cpp
    ../src/KznCanonicalJson.cpp
    ../vendor/monocypher/monocypher.c
    ../vendor/monocypher/monocypher-ed25519.c
)
target_include_directories(test_format PRIVATE
    ../include
    ../vendor/monocypher
    ../src
)

# Verifier tests
add_executable(test_verifier
    test_verifier.cpp
    ../src/KznFormat.cpp
    ../src/KznVerifier.cpp
    ../src/KznCanonicalJson.cpp
    ../vendor/monocypher/monocypher.c
    ../vendor/monocypher/monocypher-ed25519.c
)
target_include_directories(test_verifier PRIVATE
    ../include
    ../vendor/monocypher
    ../src
)

# Canonical JSON tests
add_executable(test_canonical_json
    test_canonical_json.cpp
    ../src/KznCanonicalJson.cpp
)
target_include_directories(test_canonical_json PRIVATE
    ../include
    ../src
)

# Custom target to generate test vectors before running tests
add_custom_target(generate_vectors
    COMMAND generate_test_vectors "${CMAKE_CURRENT_SOURCE_DIR}/test_vectors"
    DEPENDS generate_test_vectors
    COMMENT "Generating .kzn test vector files"
)

add_dependencies(test_verifier generate_vectors)
add_dependencies(test_format generate_vectors)
