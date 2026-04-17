#include "EmbeddedAssetFallback.h"
#include "BinaryData.h"
#include <vector>

namespace choroboros::assets
{
namespace
{
struct EmbeddedSpec
{
    const char* assetId;
    const char* resourceName;
};

#if CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK
static const std::vector<EmbeddedSpec> kEmbeddedSpecs {
    { ids::greenBackgroundOff, "green_light_off_backpanel_png" },
    { ids::greenBackgroundOn, "green_light_on_backpanel_png" },
    { ids::blueBackgroundOff, "blue_light_off_backpanel_png" },
    { ids::blueBackgroundOn, "blue_light_on_backpanel_png" },
    { ids::redBackgroundOff, "red_light_off_backpanel_png" },
    { ids::redBackgroundOn, "red_light_on_backpanel_png" },
    { ids::purpleBackgroundOff, "purple_light_off_backpanel_png" },
    { ids::purpleBackgroundOn, "purple_light_on_backpanel_png" },
    { ids::blackBackgroundOff, "black_light_off_backpanel_png" },
    { ids::blackBackgroundOn, "black_light_on_backpanel_png" },
    { ids::greenSliderThumb, "green_slider_thumb_png" },
    { ids::blueSliderThumb, "blue_slider_thumb_png" },
    { ids::redSliderThumb, "red_slider_thumb_png" },
    { ids::purpleSliderThumb, "purple_slider_thumb_png" },
    { ids::blackSliderThumb, "black__slider_thumb_png" },
    { ids::greenRateKnobOff, "green_1_off_png" },
    { ids::greenRateKnobOn, "green_1_on_png" },
    { ids::greenDepthKnobOff, "green_2_off_png" },
    { ids::greenDepthKnobOn, "green_2_on_png" },
    { ids::greenOffsetKnobOff, "green_3_off_png" },
    { ids::greenOffsetKnobOn, "green_3_on_png" },
    { ids::greenWidthKnobOff, "green_4_off_png" },
    { ids::greenWidthKnobOn, "green_4_on_png" },
    { ids::greenMixKnob, "green_mix_knob_spritesheet_png" },
    { ids::blueRateKnobOff, "blue_1_off_png" },
    { ids::blueRateKnobOn, "blue_1_on_png" },
    { ids::blueDepthKnobOff, "blue_2_off_png" },
    { ids::blueDepthKnobOn, "blue_2_on_png" },
    { ids::blueOffsetKnobOff, "blue_3_off_png" },
    { ids::blueOffsetKnobOn, "blue_3_on_png" },
    { ids::blueWidthKnobOff, "blue_4_off_png" },
    { ids::blueWidthKnobOn, "blue_4_on_png" },
    { ids::blueMixKnob, "blue_mix_knob_spritesheet_png" },
    { ids::redRateKnobOff, "red_1_off_png" },
    { ids::redRateKnobOn, "red_1_on_png" },
    { ids::redDepthKnobOff, "red_2_off_png" },
    { ids::redDepthKnobOn, "red_2_on_png" },
    { ids::redOffsetKnobOff, "red_3_off_png" },
    { ids::redOffsetKnobOn, "red_3_on_png" },
    { ids::redWidthKnobOff, "red_4_off_png" },
    { ids::redWidthKnobOn, "red_4_on_png" },
    { ids::redMixKnob, "red_mix_knob_spritesheet_png" },
    { ids::purpleRateKnobOff, "purple_1_off_png" },
    { ids::purpleRateKnobOn, "purple_1_on_png" },
    { ids::purpleDepthKnobOff, "purple_2_off_png" },
    { ids::purpleDepthKnobOn, "purple_2_on_png" },
    { ids::purpleOffsetKnobOff, "purple_3_off_png" },
    { ids::purpleOffsetKnobOn, "purple_3_on_png" },
    { ids::purpleWidthKnobOff, "purple_4_off_png" },
    { ids::purpleWidthKnobOn, "purple_4_on_png" },
    { ids::purpleMixKnob, "purple_mix_knob_spritesheet_png" },
    { ids::blackRateKnobOff, "black_1_off_png" },
    { ids::blackRateKnobOn, "black_1_on_png" },
    { ids::blackDepthKnobOff, "black_2_off_png" },
    { ids::blackDepthKnobOn, "black_2_on_png" },
    { ids::blackOffsetKnobOff, "black_3_off_png" },
    { ids::blackOffsetKnobOn, "black_3_on_png" },
    { ids::blackWidthKnobOff, "black_4_off_png" },
    { ids::blackWidthKnobOn, "black_4_on_png" },
    { ids::blackMixKnob, "black_mix_knob_spritesheet_png" },
    { ids::switchSpriteSheet, "switch_a_spritesheet_png" }
};
#endif
}

juce::String assetSourceToString(AssetSource source)
{
    switch (source)
    {
        case AssetSource::externalPack: return "external_pack";
        case AssetSource::embeddedFallback: return "embedded_fallback";
        case AssetSource::missing: break;
    }

    return "missing";
}

LoadedBinaryAsset EmbeddedAssetFallback::loadData(const juce::String& assetId)
{
    LoadedBinaryAsset result;
    result.assetId = assetId;

#if ! CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK
    return result;
#else
    for (const auto& spec : kEmbeddedSpecs)
    {
        if (assetId == spec.assetId)
        {
            int dataSize = 0;
            if (const auto* data = BinaryData::getNamedResource(spec.resourceName, dataSize);
                data != nullptr && dataSize > 0)
            {
                result.data.append(data, static_cast<size_t>(dataSize));
                result.source = AssetSource::embeddedFallback;
                result.sourceDescription = "BinaryData::" + juce::String(spec.resourceName);
            }
            return result;
        }
    }

    return result;
#endif
}
}
