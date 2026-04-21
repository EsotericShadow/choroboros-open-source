#pragma once

#include "AssetPackVersion.h"
#include <juce_core/juce_core.h>

namespace choroboros::assets
{
class AssetLocator
{
public:
    static juce::File getSharedAssetRoot();
    static juce::File getExpectedPackDirectory();
    static juce::Array<juce::File> getCandidatePackDirectories();
    static juce::File resolvePackDirectoryFromCandidates(const juce::Array<juce::File>& candidates);
    static juce::File resolvePackDirectory();
};
}
