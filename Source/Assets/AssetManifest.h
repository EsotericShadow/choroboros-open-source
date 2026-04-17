#pragma once

#include "AssetPackVersion.h"
#include <juce_core/juce_core.h>
#include <optional>
#include <vector>

namespace choroboros::assets
{
struct ManifestEntry
{
    juce::String id;
    juce::String relativePath;
    juce::String sha256;
    int64_t sizeBytes = 0;
};

class AssetManifest
{
public:
    bool loadFromFile(const juce::File& file);

    bool isValid() const noexcept { return valid; }
    bool isCompatible() const noexcept;

    const juce::File& getSourceFile() const noexcept { return sourceFile; }
    const juce::String& getPackName() const noexcept { return packName; }
    const juce::String& getPackVersion() const noexcept { return packVersion; }
    int getSchemaVersion() const noexcept { return schemaVersion; }

    std::optional<ManifestEntry> findEntry(const juce::String& assetId) const;
    juce::String resolveRelativePath(const juce::String& assetId) const;

private:
    juce::File sourceFile;
    juce::String packName;
    juce::String packVersion;
    int schemaVersion = 0;
    bool valid = false;
    std::vector<ManifestEntry> entries;
};
}
