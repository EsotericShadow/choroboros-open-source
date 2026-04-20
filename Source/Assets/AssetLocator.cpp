#include "AssetLocator.h"

namespace choroboros::assets
{
namespace
{
juce::File getPlatformAppDataBase(juce::File::SpecialLocationType locationType)
{
    auto base = juce::File::getSpecialLocation(locationType);

#if JUCE_MAC
    base = base.getChildFile("Application Support");
#endif

    return base;
}

juce::File getUserAssetRoot()
{
    return getPlatformAppDataBase(juce::File::userApplicationDataDirectory)
        .getChildFile("Kaizen Strategic AI")
        .getChildFile("Choroboros")
        .getChildFile("Assets");
}

juce::String getNamedPackDirectoryName()
{
    return "ChoroborosAssets-" + juce::String(kExpectedPackVersion);
}

void appendPackCandidates(juce::Array<juce::File>& candidates, const juce::File& root)
{
    if (root == juce::File())
        return;

    candidates.addIfNotAlreadyThere(root.getChildFile(kExpectedPackVersion));
    candidates.addIfNotAlreadyThere(root.getChildFile(getNamedPackDirectoryName()));
    candidates.addIfNotAlreadyThere(root);
}
}

juce::File AssetLocator::getSharedAssetRoot()
{
    return getPlatformAppDataBase(juce::File::commonApplicationDataDirectory)
        .getChildFile("Kaizen Strategic AI")
        .getChildFile("Choroboros")
        .getChildFile("Assets");
}

juce::File AssetLocator::getExpectedPackDirectory()
{
    return getSharedAssetRoot().getChildFile(kExpectedPackVersion);
}

juce::Array<juce::File> AssetLocator::getCandidatePackDirectories()
{
    juce::Array<juce::File> candidates;

    if (const auto overrideDir = juce::SystemStats::getEnvironmentVariable(kAssetPackOverrideEnvVar, {});
        overrideDir.isNotEmpty())
    {
        const juce::File overrideFile(overrideDir);
        appendPackCandidates(candidates, overrideFile);
        candidates.addIfNotAlreadyThere(overrideFile);
    }

    appendPackCandidates(candidates, getUserAssetRoot());
    appendPackCandidates(candidates, getSharedAssetRoot());

    return candidates;
}

juce::File AssetLocator::resolvePackDirectory()
{
    for (const auto& candidate : getCandidatePackDirectories())
    {
        if (candidate.isDirectory())
            return candidate;
    }

    return {};
}
}
