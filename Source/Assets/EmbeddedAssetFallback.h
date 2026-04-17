#pragma once

#include "AssetPackVersion.h"
#include <juce_core/juce_core.h>

namespace choroboros::assets
{
enum class AssetSource
{
    missing,
    externalPack,
    embeddedFallback
};

struct LoadedBinaryAsset
{
    juce::MemoryBlock data;
    AssetSource source = AssetSource::missing;
    juce::String assetId;
    juce::String sourceDescription;

    bool isValid() const noexcept
    {
        return data.getSize() > 0;
    }
};

juce::String assetSourceToString(AssetSource source);

namespace ids
{
inline constexpr auto greenBackgroundOff = "green/green_light_off_backpanel.png";
inline constexpr auto greenBackgroundOn = "green/green_light_on_backpanel.png";
inline constexpr auto blueBackgroundOff = "blue/blue_light_off_backpanel.png";
inline constexpr auto blueBackgroundOn = "blue/blue_light_on_backpanel.png";
inline constexpr auto redBackgroundOff = "red/red_light_off_backpanel.png";
inline constexpr auto redBackgroundOn = "red/red_light_on_backpanel.png";
inline constexpr auto purpleBackgroundOff = "purple/purple_light_off_backpanel.png";
inline constexpr auto purpleBackgroundOn = "purple/purple_light_on_backpanel.png";
inline constexpr auto blackBackgroundOff = "black/black_light_off_backpanel.png";
inline constexpr auto blackBackgroundOn = "black/black_light_on_backpanel.png";

inline constexpr auto greenSliderThumb = "green/green_slider_thumb.png";
inline constexpr auto blueSliderThumb = "blue/blue_slider_thumb.png";
inline constexpr auto redSliderThumb = "red/red_slider_thumb.png";
inline constexpr auto purpleSliderThumb = "purple/purple_slider_thumb.png";
inline constexpr auto blackSliderThumb = "black/black__slider_thumb.png";

inline constexpr auto greenRateKnobOff = "green/rate/green_1_off.png";
inline constexpr auto greenRateKnobOn = "green/rate/green_1_on.png";
inline constexpr auto greenDepthKnobOff = "green/depth/green_2_off.png";
inline constexpr auto greenDepthKnobOn = "green/depth/green_2_on.png";
inline constexpr auto greenOffsetKnobOff = "green/offset/green_3_off.png";
inline constexpr auto greenOffsetKnobOn = "green/offset/green_3_on.png";
inline constexpr auto greenWidthKnobOff = "green/width/green_4_off.png";
inline constexpr auto greenWidthKnobOn = "green/width/green_4_on.png";
inline constexpr auto greenMixKnob = "green/green_mix_knob_spritesheet.png";

inline constexpr auto blueRateKnobOff = "blue/rate/blue_1_off.png";
inline constexpr auto blueRateKnobOn = "blue/rate/blue_1_on.png";
inline constexpr auto blueDepthKnobOff = "blue/depth/blue_2_off.png";
inline constexpr auto blueDepthKnobOn = "blue/depth/blue_2_on.png";
inline constexpr auto blueOffsetKnobOff = "blue/offset/blue_3_off.png";
inline constexpr auto blueOffsetKnobOn = "blue/offset/blue_3_on.png";
inline constexpr auto blueWidthKnobOff = "blue/width/blue_4_off.png";
inline constexpr auto blueWidthKnobOn = "blue/width/blue_4_on.png";
inline constexpr auto blueMixKnob = "blue/blue_mix_knob_spritesheet.png";

inline constexpr auto redRateKnobOff = "red/rate/red_1_off.png";
inline constexpr auto redRateKnobOn = "red/rate/red_1_on.png";
inline constexpr auto redDepthKnobOff = "red/depth/red_2_off.png";
inline constexpr auto redDepthKnobOn = "red/depth/red_2_on.png";
inline constexpr auto redOffsetKnobOff = "red/offset/red_3_off.png";
inline constexpr auto redOffsetKnobOn = "red/offset/red_3_on.png";
inline constexpr auto redWidthKnobOff = "red/width/red_4_off.png";
inline constexpr auto redWidthKnobOn = "red/width/red_4_on.png";
inline constexpr auto redMixKnob = "red/red_mix_knob_spritesheet.png";

inline constexpr auto purpleRateKnobOff = "purple/rate/purple_1_off.png";
inline constexpr auto purpleRateKnobOn = "purple/rate/purple_1_on.png";
inline constexpr auto purpleDepthKnobOff = "purple/depth/purple_2_off.png";
inline constexpr auto purpleDepthKnobOn = "purple/depth/purple_2_on.png";
inline constexpr auto purpleOffsetKnobOff = "purple/offset/purple_3_off.png";
inline constexpr auto purpleOffsetKnobOn = "purple/offset/purple_3_on.png";
inline constexpr auto purpleWidthKnobOff = "purple/width/purple_4_off.png";
inline constexpr auto purpleWidthKnobOn = "purple/width/purple_4_on.png";
inline constexpr auto purpleMixKnob = "purple/purple_mix_knob_spritesheet.png";

inline constexpr auto blackRateKnobOff = "black/rate/black_1_off.png";
inline constexpr auto blackRateKnobOn = "black/rate/black_1_on.png";
inline constexpr auto blackDepthKnobOff = "black/depth/black_2_off.png";
inline constexpr auto blackDepthKnobOn = "black/depth/black_2_on.png";
inline constexpr auto blackOffsetKnobOff = "black/offset/black_3_off.png";
inline constexpr auto blackOffsetKnobOn = "black/offset/black_3_on.png";
inline constexpr auto blackWidthKnobOff = "black/width/black_4_off.png";
inline constexpr auto blackWidthKnobOn = "black/width/black_4_on.png";
inline constexpr auto blackMixKnob = "black/black_mix_knob_spritesheet.png";

inline constexpr auto switchSpriteSheet = "switch_a_spritesheet.png";
}

class EmbeddedAssetFallback
{
public:
    static LoadedBinaryAsset loadData(const juce::String& assetId);
};
}
