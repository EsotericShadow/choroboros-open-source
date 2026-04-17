#include "AssetLocator.h"

namespace choroboros::assets
{
juce::File AssetLocator::getSharedAssetRoot()
{
    return juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
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
        candidates.add(juce::File(overrideDir));
    }

    candidates.addIfNotAlreadyThere(getExpectedPackDirectory());
    candidates.addIfNotAlreadyThere(getSharedAssetRoot());

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
