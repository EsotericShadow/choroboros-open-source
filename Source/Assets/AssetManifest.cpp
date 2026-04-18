#include "AssetManifest.h"
#include <cmath>

namespace choroboros::assets
{
namespace
{
juce::String readStringProperty(const juce::DynamicObject* object, const juce::Identifier& propertyName)
{
    if (object == nullptr)
        return {};

    return object->getProperty(propertyName).toString();
}

int64_t readInt64Property(const juce::DynamicObject* object, const juce::Identifier& propertyName)
{
    if (object == nullptr)
        return 0;

    const auto value = object->getProperty(propertyName);
    if (value.isInt64())
        return static_cast<int64_t>(static_cast<juce::int64>(value));
    if (value.isInt())
        return static_cast<int64_t>(static_cast<int>(value));
    if (value.isDouble())
        return static_cast<int64_t>(std::llround(static_cast<double>(value)));
    return 0;
}
}

bool AssetManifest::loadFromFile(const juce::File& file)
{
    sourceFile = file;
    packName.clear();
    packVersion.clear();
    schemaVersion = 0;
    valid = false;
    entries.clear();

    if (!file.existsAsFile())
        return false;

    const auto jsonText = file.loadFileAsString();
    if (jsonText.isEmpty())
        return false;

    const auto parsed = juce::JSON::parse(jsonText);
    const auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return false;

    schemaVersion = static_cast<int>(readInt64Property(root, "schemaVersion"));
    packName = readStringProperty(root, "packName");
    packVersion = readStringProperty(root, "assetPackVersion");

    const auto assetsVar = root->getProperty("assets");
    if (!assetsVar.isArray())
        return false;

    const auto* assetsArray = assetsVar.getArray();
    if (assetsArray == nullptr)
        return false;

    entries.reserve(assetsArray->size());
    for (const auto& entryVar : *assetsArray)
    {
        const auto* entryObject = entryVar.getDynamicObject();
        if (entryObject == nullptr)
            continue;

        ManifestEntry entry;
        entry.id = readStringProperty(entryObject, "id");
        entry.relativePath = readStringProperty(entryObject, "relativePath");
        entry.sha256 = readStringProperty(entryObject, "sha256");
        entry.sizeBytes = readInt64Property(entryObject, "sizeBytes");

        if (entry.id.isNotEmpty() && entry.relativePath.isNotEmpty())
            entries.push_back(std::move(entry));
    }

    valid = (schemaVersion == kManifestSchemaVersion)
        && packName.isNotEmpty()
        && packVersion.isNotEmpty()
        && !entries.empty();

    return valid;
}

bool AssetManifest::isCompatible() const noexcept
{
    return valid
        && schemaVersion == kManifestSchemaVersion
        && packVersion == kExpectedPackVersion;
}

std::optional<ManifestEntry> AssetManifest::findEntry(const juce::String& assetId) const
{
    for (const auto& entry : entries)
        if (entry.id == assetId)
            return entry;

    return std::nullopt;
}

juce::String AssetManifest::resolveRelativePath(const juce::String& assetId) const
{
    if (const auto entry = findEntry(assetId))
        return entry->relativePath;

    return assetId;
}
}
