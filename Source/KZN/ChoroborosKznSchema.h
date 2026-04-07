/*
 * Choroboros - .kzn payload schema constants
 */

#pragma once

#include <cstdint>

namespace choroboros { namespace kzn_schema {
    inline constexpr const char* SCHEMA_VERSION = "1.0";
    inline constexpr const char* PLUGIN_ID = "choroboros";
    inline constexpr const char* PLUGIN_VERSION = "1.0";
    inline constexpr const char* TYPE_PRESET = "preset";
    inline constexpr const char* TYPE_ENGINE = "engine";
    inline constexpr uint32_t KEY_ID_UNSIGNED = 0;
    inline constexpr uint32_t KEY_ID_TEST = 1;
}}
