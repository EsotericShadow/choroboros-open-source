#include "AssetRepository.h"
#include <juce_core/juce_core.h>

namespace choroboros::assets
{
namespace
{
juce::Image toSoftwareImage(const juce::Image& image)
{
    if (!image.isValid())
        return {};

    juce::SoftwareImageType softwareType;
    return softwareType.convert(image);
}
}

AssetRepository& AssetRepository::instance()
{
    static AssetRepository repository;
    return repository;
}

void AssetRepository::reset()
{
    const std::lock_guard<std::mutex> lock(mutex);
    packResolutionComplete = false;
    packDirectory = juce::File();
    manifest = {};
    manifestLoaded = false;
    binaryCache.clear();
    imageCache.clear();
    loggedAssetKeys.clear();
}

juce::String AssetRepository::getActivePackDirectory() const
{
    const std::lock_guard<std::mutex> lock(mutex);
    return packDirectory.getFullPathName();
}

juce::String AssetRepository::getActivePackVersion() const
{
    const std::lock_guard<std::mutex> lock(mutex);
    return manifestLoaded && manifest.isValid() ? manifest.getPackVersion() : juce::String();
}

bool AssetRepository::isUsingExternalPack() const
{
    const std::lock_guard<std::mutex> lock(mutex);
    return packDirectory.isDirectory();
}

void AssetRepository::ensurePackResolutionLocked()
{
    if (packResolutionComplete)
        return;

    packDirectory = juce::File();
    manifestLoaded = false;

    if (kUseExternalAssetPack)
    {
        packDirectory = AssetLocator::resolvePackDirectory();

        if (packDirectory.isDirectory())
        {
            const auto manifestFile = packDirectory.getChildFile("manifest.json");
            manifestLoaded = manifest.loadFromFile(manifestFile);
        }
    }

    packResolutionComplete = true;
}

juce::File AssetRepository::resolveExternalAssetFileLocked(const juce::String& assetId, juce::String& description) const
{
    description.clear();

    if (!packDirectory.isDirectory())
        return {};

    const auto relativePath = (manifestLoaded && manifest.isValid())
        ? manifest.resolveRelativePath(assetId)
        : assetId;

    const auto file = packDirectory.getChildFile(relativePath);
    if (file.existsAsFile())
    {
        description = file.getFullPathName();
        return file;
    }

    return {};
}

void AssetRepository::logResolutionOnceLocked(const juce::String& assetId,
                                              AssetSource source,
                                              const juce::String& description)
{
    const auto key = assetId + "|" + assetSourceToString(source);
    if (!loggedAssetKeys.insert(key).second)
        return;

    juce::Logger::writeToLog("AssetRepository: " + assetId
                             + " -> " + assetSourceToString(source)
                             + (description.isNotEmpty() ? " (" + description + ")" : ""));
}

LoadedBinaryAsset AssetRepository::loadData(const juce::String& assetId)
{
    std::lock_guard<std::mutex> lock(mutex);
    ensurePackResolutionLocked();

    if (const auto found = binaryCache.find(assetId); found != binaryCache.end())
        return found->second;

    LoadedBinaryAsset result;
    result.assetId = assetId;

    juce::String externalDescription;
    const auto externalFile = resolveExternalAssetFileLocked(assetId, externalDescription);
    if (externalFile.existsAsFile())
    {
        if (externalFile.loadFileAsData(result.data))
        {
            result.source = AssetSource::externalPack;
            result.sourceDescription = externalDescription;
        }
    }

    if (!result.isValid() && kAllowEmbeddedAssetFallback)
        result = EmbeddedAssetFallback::loadData(assetId);

    if (!result.isValid())
        result.sourceDescription = "asset_missing";

    logResolutionOnceLocked(assetId, result.source, result.sourceDescription);
    binaryCache.emplace(assetId, result);
    return result;
}

LoadedImageAsset AssetRepository::loadImage(const juce::String& assetId, bool softwareImage)
{
    const auto cacheKey = assetId + (softwareImage ? "|software" : "|native");

    {
        const std::lock_guard<std::mutex> lock(mutex);
        if (const auto found = imageCache.find(cacheKey); found != imageCache.end())
            return found->second;
    }

    const auto binaryAsset = loadData(assetId);

    LoadedImageAsset result;
    result.assetId = assetId;
    result.source = binaryAsset.source;
    result.sourceDescription = binaryAsset.sourceDescription;

    if (binaryAsset.isValid())
    {
        auto decoded = juce::ImageFileFormat::loadFrom(binaryAsset.data.getData(), binaryAsset.data.getSize());
        if (softwareImage)
            decoded = toSoftwareImage(decoded);
        result.image = decoded;
    }

    const std::lock_guard<std::mutex> lock(mutex);
    imageCache.emplace(cacheKey, result);
    return result;
}
}
