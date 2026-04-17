#pragma once

#include "AssetLocator.h"
#include "AssetManifest.h"
#include "EmbeddedAssetFallback.h"
#include <juce_graphics/juce_graphics.h>
#include <map>
#include <mutex>
#include <set>

namespace choroboros::assets
{
struct LoadedImageAsset
{
    juce::Image image;
    AssetSource source = AssetSource::missing;
    juce::String assetId;
    juce::String sourceDescription;

    bool isValid() const noexcept
    {
        return image.isValid();
    }
};

class AssetRepository
{
public:
    static AssetRepository& instance();

    LoadedBinaryAsset loadData(const juce::String& assetId);
    LoadedImageAsset loadImage(const juce::String& assetId, bool softwareImage);

    juce::String getActivePackDirectory() const;
    juce::String getActivePackVersion() const;
    bool isUsingExternalPack() const;

    void reset();

private:
    AssetRepository() = default;

    void ensurePackResolutionLocked();
    juce::File resolveExternalAssetFileLocked(const juce::String& assetId, juce::String& description) const;
    void logResolutionOnceLocked(const juce::String& assetId,
                                 AssetSource source,
                                 const juce::String& description);

    mutable std::mutex mutex;
    bool packResolutionComplete = false;
    juce::File packDirectory;
    AssetManifest manifest;
    bool manifestLoaded = false;

    std::map<juce::String, LoadedBinaryAsset> binaryCache;
    std::map<juce::String, LoadedImageAsset> imageCache;
    std::set<juce::String> loggedAssetKeys;
};
}
