/*
 * Choroboros - Modular core assignment contracts
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace choroboros
{
constexpr int kEngineColorCount = 5;
constexpr int kEngineModeCount = 2; // 0=NQ, 1=HQ

enum class CoreId : std::uint8_t
{
    lagrange3 = 0,
    lagrange5,
    cubic,
    thiran,
    bbd,
    tape,
    phase_warp,
    orbit,
    linear,
    ensemble,
    count
};

constexpr std::size_t coreIdCount() noexcept
{
    return static_cast<std::size_t>(CoreId::count);
}

constexpr int modeIndex(bool hqEnabled) noexcept
{
    return hqEnabled ? 1 : 0;
}

constexpr bool hqEnabledFromMode(int mode) noexcept
{
    return mode != 0;
}

struct SlotAssignment
{
    int engineColor = 0;
    bool hqEnabled = false;
    CoreId coreId = CoreId::lagrange3;
};

struct CoreAssignmentTable
{
    std::array<std::array<CoreId, kEngineModeCount>, kEngineColorCount> slot {{
        {{ CoreId::lagrange3, CoreId::lagrange5 }},
        {{ CoreId::cubic, CoreId::thiran }},
        {{ CoreId::bbd, CoreId::tape }},
        {{ CoreId::phase_warp, CoreId::orbit }},
        {{ CoreId::linear, CoreId::ensemble }}
    }};

    constexpr void resetToLegacy() noexcept
    {
        slot[0][0] = CoreId::lagrange3;
        slot[0][1] = CoreId::lagrange5;
        slot[1][0] = CoreId::cubic;
        slot[1][1] = CoreId::thiran;
        slot[2][0] = CoreId::bbd;
        slot[2][1] = CoreId::tape;
        slot[3][0] = CoreId::phase_warp;
        slot[3][1] = CoreId::orbit;
        slot[4][0] = CoreId::linear;
        slot[4][1] = CoreId::ensemble;
    }

    CoreId get(int engineColor, bool hqEnabled) const noexcept
    {
        const int safeEngine = engineColor < 0
            ? 0
            : (engineColor >= kEngineColorCount ? kEngineColorCount - 1 : engineColor);
        return slot[static_cast<std::size_t>(safeEngine)][static_cast<std::size_t>(modeIndex(hqEnabled))];
    }

    void set(int engineColor, bool hqEnabled, CoreId coreId) noexcept
    {
        const int safeEngine = engineColor < 0
            ? 0
            : (engineColor >= kEngineColorCount ? kEngineColorCount - 1 : engineColor);
        slot[static_cast<std::size_t>(safeEngine)][static_cast<std::size_t>(modeIndex(hqEnabled))] = coreId;
    }

    bool operator==(const CoreAssignmentTable& other) const noexcept
    {
        return slot == other.slot;
    }

    bool operator!=(const CoreAssignmentTable& other) const noexcept
    {
        return !(*this == other);
    }
};

struct CorePackageDescriptor
{
    CoreId id = CoreId::lagrange3;
    const char* token = "lagrange3";
    const char* displayName = "Lagrange 3rd";
    const char* macroLabel = "Bloom Macros";
    const char* macroSemantics = "green bloom";
    const char* notes = "";
    bool skipPreEmphasis = false;
    bool bloomWetCharacter = false;
    bool focusWetCharacter = false;
    bool postChorusSaturation = false;
    bool depthCompression = false;
    bool bloomDepthScale = false;
    bool bloomCentreOffset = false;
};

inline constexpr std::array<const char*, kEngineColorCount> kEngineColorTokens {
    "green", "blue", "red", "purple", "black"
};

inline constexpr std::array<const char*, kEngineColorCount> kEngineColorDisplay {
    "Green", "Blue", "Red", "Purple", "Black"
};

inline constexpr std::array<CorePackageDescriptor, coreIdCount()> kCorePackageDescriptors {{
    { CoreId::lagrange3, "lagrange3", "Lagrange 3rd", "Bloom Macros", "green bloom", "Classic interpolation with bloom behavior.", false, true, false, false, false, true, true },
    { CoreId::lagrange5, "lagrange5", "Lagrange 5th", "Bloom Macros", "green bloom", "HQ classic interpolation with bloom behavior.", false, true, false, false, false, true, true },
    { CoreId::cubic, "cubic", "Cubic", "Focus Macros", "blue focus", "Modern cubic interpolation with focus behavior.", false, false, true, false, false, false, false },
    { CoreId::thiran, "thiran", "Thiran Allpass", "Focus Macros", "blue focus", "HQ allpass interpolation with focus behavior.", false, false, true, false, false, false, false },
    { CoreId::bbd, "bbd", "BBD", "BBD Macros", "red vintage bbd", "Bucket-brigade style mode with post saturation.", true, false, false, true, false, false, false },
    { CoreId::tape, "tape", "Tape", "Tape Macros", "red vintage tape", "Tape-style mode with wow/flutter and tone drive.", false, false, false, false, false, false, false },
    { CoreId::phase_warp, "phase_warp", "Phase Warp", "Warp Macros", "purple warp", "Nonlinear phase-warp motion.", false, false, false, false, true, false, false },
    { CoreId::orbit, "orbit", "Orbit", "Orbit Macros", "purple orbit", "2D orbit motion path.", false, false, false, false, true, false, false },
    { CoreId::linear, "linear", "Linear", "Intensity Macros", "black intensity", "Fast linear interpolation intensity mode.", false, false, false, false, false, false, false },
    { CoreId::ensemble, "ensemble", "Linear Ensemble", "Ensemble Macros", "black ensemble", "Dual-voice ensemble mode.", false, false, false, false, false, false, false }
}};

inline bool equalsIgnoreCase(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;

    for (std::size_t i = 0; i < a.size(); ++i)
    {
        const unsigned char ac = static_cast<unsigned char>(a[i]);
        const unsigned char bc = static_cast<unsigned char>(b[i]);
        const unsigned char al = (ac >= 'A' && ac <= 'Z') ? static_cast<unsigned char>(ac + ('a' - 'A')) : ac;
        const unsigned char bl = (bc >= 'A' && bc <= 'Z') ? static_cast<unsigned char>(bc + ('a' - 'A')) : bc;
        if (al != bl)
            return false;
    }

    return true;
}

inline const CorePackageDescriptor& descriptorForCore(CoreId id)
{
    const std::size_t index = static_cast<std::size_t>(id);
    if (index >= kCorePackageDescriptors.size())
        return kCorePackageDescriptors[0];
    return kCorePackageDescriptors[index];
}

inline const char* coreIdToToken(CoreId id)
{
    return descriptorForCore(id).token;
}

inline const char* coreIdToDisplayName(CoreId id)
{
    return descriptorForCore(id).displayName;
}

inline const char* coreIdToMacroLabel(CoreId id)
{
    return descriptorForCore(id).macroLabel;
}

inline bool parseCoreIdToken(std::string_view token, CoreId& outCoreId)
{
    for (const auto& descriptor : kCorePackageDescriptors)
    {
        if (equalsIgnoreCase(token, descriptor.token))
        {
            outCoreId = descriptor.id;
            return true;
        }
    }
    return false;
}

inline int parseEngineColorToken(std::string_view token)
{
    for (int i = 0; i < kEngineColorCount; ++i)
    {
        if (equalsIgnoreCase(token, kEngineColorTokens[static_cast<std::size_t>(i)]))
            return i;
    }
    return -1;
}

inline std::string slotLabel(int engineColor, bool hqEnabled)
{
    const int safeEngine = engineColor < 0
        ? 0
        : (engineColor >= kEngineColorCount ? kEngineColorCount - 1 : engineColor);
    std::string label = kEngineColorTokens[static_cast<std::size_t>(safeEngine)];
    label += hqEnabled ? " hq" : " nq";
    return label;
}

inline std::vector<SlotAssignment> findAssignmentsForCore(const CoreAssignmentTable& table, CoreId coreId)
{
    std::vector<SlotAssignment> assignments;
    assignments.reserve(kEngineColorCount * kEngineModeCount);
    for (int engine = 0; engine < kEngineColorCount; ++engine)
    {
        for (int mode = 0; mode < kEngineModeCount; ++mode)
        {
            const bool hqEnabled = hqEnabledFromMode(mode);
            if (table.get(engine, hqEnabled) == coreId)
                assignments.push_back({ engine, hqEnabled, coreId });
        }
    }
    return assignments;
}

inline bool assignmentIsDuplicate(const CoreAssignmentTable& table, int engineColor, bool hqEnabled, CoreId coreId)
{
    int count = 0;
    for (int engine = 0; engine < kEngineColorCount; ++engine)
    {
        for (int mode = 0; mode < kEngineModeCount; ++mode)
        {
            const bool modeHq = hqEnabledFromMode(mode);
            const CoreId candidate = (engine == engineColor && modeHq == hqEnabled)
                ? coreId
                : table.get(engine, modeHq);
            if (candidate == coreId)
                ++count;
            if (count > 1)
                return true;
        }
    }
    return false;
}

} // namespace choroboros

