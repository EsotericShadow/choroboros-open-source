/*
 * Choroboros - A chorus that eats its own tail
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CustomLookAndFeel.h"
#include "SmoothedSlider.h"
#include "LabelWithContainer.h"
#include "../Assets/AssetRepository.h"
#include <cmath>
#include <utility>

namespace
{
juce::Image toSoftwareImage(const juce::Image& image)
{
    if (! image.isValid())
        return {};

    juce::SoftwareImageType softwareType;
    return softwareType.convert(image);
}
}

namespace
{
struct SharedThemeAssetCache
{
    juce::CriticalSection lock;
    std::array<CustomLookAndFeel::ThemeAssetPack, 5> packs {};
    std::array<bool, 5> valid { false, false, false, false, false };
};

SharedThemeAssetCache& getSharedThemeAssetCache()
{
    static SharedThemeAssetCache cache;
    return cache;
}

juce::Image loadThemeImage(const juce::String& assetId)
{
    auto image = choroboros::assets::AssetRepository::instance().loadImage(assetId, true).image;
    return image.isValid() ? image : juce::Image {};
}
} // namespace

CustomLookAndFeel::CustomLookAndFeel()
{
}

void CustomLookAndFeel::setHqAnimationState(float progress, bool animationActive, bool hqIsCurrentlyOn) noexcept
{
    hqAnimationProgress = juce::jlimit(0.0f, 1.0f, progress);
    hqAnimationActive = animationActive;
    hqIsOn = hqIsCurrentlyOn;
}

void CustomLookAndFeel::setColorTheme(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    if (currentColorIndex == colorIndex && cachedThemeValid[static_cast<size_t>(colorIndex)])
        return;

    currentColorIndex = colorIndex;
    loadImages(colorIndex);
}

void CustomLookAndFeel::setThemeColorIndexOnly(int colorIndex) noexcept
{
    currentColorIndex = juce::jlimit(0, 4, colorIndex);
}

bool CustomLookAndFeel::isThemeCached(int colorIndex) const noexcept
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    return cachedThemeValid[static_cast<size_t>(colorIndex)];
}

bool CustomLookAndFeel::isThemeAssetPackCached(int colorIndex) noexcept
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    const auto index = static_cast<size_t>(colorIndex);
    const auto& cache = getSharedThemeAssetCache();
    const juce::ScopedLock lock(cache.lock);
    return cache.valid[index];
}

CustomLookAndFeel::ThemeAssetPack CustomLookAndFeel::getOrDecodeThemeAssetPack(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    const auto index = static_cast<size_t>(colorIndex);
    auto& cache = getSharedThemeAssetCache();

    {
        const juce::ScopedLock lock(cache.lock);
        if (cache.valid[index])
            return cache.packs[index];
    }

    auto decoded = decodeThemeAssetPack(colorIndex);

    {
        const juce::ScopedLock lock(cache.lock);
        if (!cache.valid[index])
        {
            cache.packs[index] = decoded;
            cache.valid[index] = true;
        }
        return cache.packs[index];
    }
}

void CustomLookAndFeel::installThemeAssetPack(int colorIndex, ThemeAssetPack&& pack)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    auto& cachedPack = cachedThemeAssets[static_cast<size_t>(colorIndex)];
    cachedPack = std::move(pack);
    cachedThemeValid[static_cast<size_t>(colorIndex)] = true;

    {
        auto& sharedCache = getSharedThemeAssetCache();
        const juce::ScopedLock lock(sharedCache.lock);
        sharedCache.packs[static_cast<size_t>(colorIndex)] = cachedPack;
        sharedCache.valid[static_cast<size_t>(colorIndex)] = true;
    }

    if (currentColorIndex == colorIndex)
        applyThemeAssetPack(cachedPack);
}

juce::Colour CustomLookAndFeel::getThemeAccentColour() const
{
    switch (currentColorIndex)
    {
        case 0: return juce::Colour(0xff9dbd78); // Green
        case 1: return juce::Colour(0xff7fb8ff); // Blue
        case 2: return juce::Colour(0xffff8d8b); // Red
        case 3: return juce::Colour(0xffb88dd8); // Purple
        case 4: return juce::Colour(0xffd4d4d4); // Black
        default: break;
    }

    return juce::Colour(0xff9dbd78);
}

juce::Colour CustomLookAndFeel::getThemePanelColour() const
{
    return juce::Colour(0xff121417).interpolatedWith(getThemeAccentColour(), 0.12f);
}

juce::Colour CustomLookAndFeel::getThemePanelOutlineColour() const
{
    return getThemeAccentColour().withAlpha(0.82f);
}

void CustomLookAndFeel::loadImages(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    auto& cachedPack = cachedThemeAssets[static_cast<size_t>(colorIndex)];
    auto& isCached = cachedThemeValid[static_cast<size_t>(colorIndex)];

    if (!isCached)
    {
        cachedPack = getOrDecodeThemeAssetPack(colorIndex);
        isCached = true;
    }

    applyThemeAssetPack(cachedPack);
}

void CustomLookAndFeel::applyThemeAssetPack(const ThemeAssetPack& pack)
{
    knobBaseImage = pack.knobBaseImage;
    knobIndicatorImage = pack.knobIndicatorImage;
    knobShadowOverlayImage = pack.knobShadowOverlayImage;
    sliderTrackImage = pack.sliderTrackImage;
    sliderThumbImage = pack.sliderThumbImage;
    mixKnobImage = pack.mixKnobImage;
    knobSpriteSheetRateImage = pack.knobSpriteSheetRateImage;
    knobSpriteSheetDepthImage = pack.knobSpriteSheetDepthImage;
    knobSpriteSheetOffsetImage = pack.knobSpriteSheetOffsetImage;
    knobSpriteSheetWidthImage = pack.knobSpriteSheetWidthImage;
    knobSpriteSheetRateOnImage = pack.knobSpriteSheetRateOnImage;
    knobSpriteSheetDepthOnImage = pack.knobSpriteSheetDepthOnImage;
    knobSpriteSheetOffsetOnImage = pack.knobSpriteSheetOffsetOnImage;
    knobSpriteSheetWidthOnImage = pack.knobSpriteSheetWidthOnImage;
    mixKnobSpriteSheetImage = pack.mixKnobSpriteSheetImage;
}

CustomLookAndFeel::ThemeAssetPack CustomLookAndFeel::decodeThemeAssetPack(int colorIndex)
{
    ThemeAssetPack pack;
    colorIndex = juce::jlimit(0, 4, colorIndex);

    switch (colorIndex)
    {
        case 0:
            pack.sliderThumbImage = loadThemeImage(choroboros::assets::ids::greenSliderThumb);
            pack.knobSpriteSheetRateImage = loadThemeImage(choroboros::assets::ids::greenRateKnobOff);
            pack.knobSpriteSheetDepthImage = loadThemeImage(choroboros::assets::ids::greenDepthKnobOff);
            pack.knobSpriteSheetOffsetImage = loadThemeImage(choroboros::assets::ids::greenOffsetKnobOff);
            pack.knobSpriteSheetWidthImage = loadThemeImage(choroboros::assets::ids::greenWidthKnobOff);
            pack.knobSpriteSheetRateOnImage = loadThemeImage(choroboros::assets::ids::greenRateKnobOn);
            pack.knobSpriteSheetDepthOnImage = loadThemeImage(choroboros::assets::ids::greenDepthKnobOn);
            pack.knobSpriteSheetOffsetOnImage = loadThemeImage(choroboros::assets::ids::greenOffsetKnobOn);
            pack.knobSpriteSheetWidthOnImage = loadThemeImage(choroboros::assets::ids::greenWidthKnobOn);
            pack.mixKnobSpriteSheetImage = loadThemeImage(choroboros::assets::ids::greenMixKnob);
            break;
        case 1:
            pack.sliderThumbImage = loadThemeImage(choroboros::assets::ids::blueSliderThumb);
            pack.knobSpriteSheetRateImage = loadThemeImage(choroboros::assets::ids::blueRateKnobOff);
            pack.knobSpriteSheetDepthImage = loadThemeImage(choroboros::assets::ids::blueDepthKnobOff);
            pack.knobSpriteSheetOffsetImage = loadThemeImage(choroboros::assets::ids::blueOffsetKnobOff);
            pack.knobSpriteSheetWidthImage = loadThemeImage(choroboros::assets::ids::blueWidthKnobOff);
            pack.knobSpriteSheetRateOnImage = loadThemeImage(choroboros::assets::ids::blueRateKnobOn);
            pack.knobSpriteSheetDepthOnImage = loadThemeImage(choroboros::assets::ids::blueDepthKnobOn);
            pack.knobSpriteSheetOffsetOnImage = loadThemeImage(choroboros::assets::ids::blueOffsetKnobOn);
            pack.knobSpriteSheetWidthOnImage = loadThemeImage(choroboros::assets::ids::blueWidthKnobOn);
            pack.mixKnobSpriteSheetImage = loadThemeImage(choroboros::assets::ids::blueMixKnob);
            break;
        case 2:
            pack.sliderThumbImage = loadThemeImage(choroboros::assets::ids::redSliderThumb);
            pack.knobSpriteSheetRateImage = loadThemeImage(choroboros::assets::ids::redRateKnobOff);
            pack.knobSpriteSheetDepthImage = loadThemeImage(choroboros::assets::ids::redDepthKnobOff);
            pack.knobSpriteSheetOffsetImage = loadThemeImage(choroboros::assets::ids::redOffsetKnobOff);
            pack.knobSpriteSheetWidthImage = loadThemeImage(choroboros::assets::ids::redWidthKnobOff);
            pack.knobSpriteSheetRateOnImage = loadThemeImage(choroboros::assets::ids::redRateKnobOn);
            pack.knobSpriteSheetDepthOnImage = loadThemeImage(choroboros::assets::ids::redDepthKnobOn);
            pack.knobSpriteSheetOffsetOnImage = loadThemeImage(choroboros::assets::ids::redOffsetKnobOn);
            pack.knobSpriteSheetWidthOnImage = loadThemeImage(choroboros::assets::ids::redWidthKnobOn);
            pack.mixKnobSpriteSheetImage = loadThemeImage(choroboros::assets::ids::redMixKnob);
            break;
        case 3:
            pack.sliderThumbImage = loadThemeImage(choroboros::assets::ids::purpleSliderThumb);
            pack.knobSpriteSheetRateImage = loadThemeImage(choroboros::assets::ids::purpleRateKnobOff);
            pack.knobSpriteSheetDepthImage = loadThemeImage(choroboros::assets::ids::purpleDepthKnobOff);
            pack.knobSpriteSheetOffsetImage = loadThemeImage(choroboros::assets::ids::purpleOffsetKnobOff);
            pack.knobSpriteSheetWidthImage = loadThemeImage(choroboros::assets::ids::purpleWidthKnobOff);
            pack.knobSpriteSheetRateOnImage = loadThemeImage(choroboros::assets::ids::purpleRateKnobOn);
            pack.knobSpriteSheetDepthOnImage = loadThemeImage(choroboros::assets::ids::purpleDepthKnobOn);
            pack.knobSpriteSheetOffsetOnImage = loadThemeImage(choroboros::assets::ids::purpleOffsetKnobOn);
            pack.knobSpriteSheetWidthOnImage = loadThemeImage(choroboros::assets::ids::purpleWidthKnobOn);
            pack.mixKnobSpriteSheetImage = loadThemeImage(choroboros::assets::ids::purpleMixKnob);
            break;
        case 4:
        default:
            pack.sliderThumbImage = loadThemeImage(choroboros::assets::ids::blackSliderThumb);
            pack.knobSpriteSheetRateImage = loadThemeImage(choroboros::assets::ids::blackRateKnobOff);
            pack.knobSpriteSheetDepthImage = loadThemeImage(choroboros::assets::ids::blackDepthKnobOff);
            pack.knobSpriteSheetOffsetImage = loadThemeImage(choroboros::assets::ids::blackOffsetKnobOff);
            pack.knobSpriteSheetWidthImage = loadThemeImage(choroboros::assets::ids::blackWidthKnobOff);
            pack.knobSpriteSheetRateOnImage = loadThemeImage(choroboros::assets::ids::blackRateKnobOn);
            pack.knobSpriteSheetDepthOnImage = loadThemeImage(choroboros::assets::ids::blackDepthKnobOn);
            pack.knobSpriteSheetOffsetOnImage = loadThemeImage(choroboros::assets::ids::blackOffsetKnobOn);
            pack.knobSpriteSheetWidthOnImage = loadThemeImage(choroboros::assets::ids::blackWidthKnobOn);
            pack.mixKnobSpriteSheetImage = loadThemeImage(choroboros::assets::ids::blackMixKnob);
            break;
    }

    return pack;
}

// Codacy: Parameter count warning unavoidable - this is a JUCE override method
// that must match the base class signature (LookAndFeel_V4::drawRotarySlider)
void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    // Downscaling large filmstrip frames (384/512px -> UI knob size) needs the best available
    // filter on Windows to avoid visible stair-stepping on thin highlights.
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

    // Check if this is the mix knob (by component name)
    const bool isMixKnob = slider.getName() == "Mix" || slider.getComponentID() == "Mix";

    // Use smoothed visual value if this is a SmoothedSlider
    float visualSliderPos = sliderPos;
    if (auto* smoothedSlider = dynamic_cast<SmoothedSlider*>(&slider))
    {
        const double minValue = slider.getMinimum();
        const double maxValue = slider.getMaximum();
        const double visualValue = smoothedSlider->getVisualValue();
        visualSliderPos = static_cast<float>((visualValue - minValue) / (maxValue - minValue));
        visualSliderPos = juce::jlimit(0.0f, 1.0f, visualSliderPos);
    }

    const int sweepStartDeg = juce::jlimit(0, 360, static_cast<int>(slider.getProperties().getWithDefault("knobSweepStartDeg", 0)));
    const int sweepEndDeg = juce::jlimit(0, 360, static_cast<int>(slider.getProperties().getWithDefault("knobSweepEndDeg", 360)));
    const int requestedFrameCount = juce::jmax(2, static_cast<int>(slider.getProperties().getWithDefault("knobFrameCount", 156)));
    const int sweepEndAdjustedDeg = (sweepEndDeg <= sweepStartDeg) ? (sweepStartDeg + 1) : sweepEndDeg;
    const float sweepStart = static_cast<float>(sweepStartDeg);
    const float sweepSpan = static_cast<float>(sweepEndAdjustedDeg - sweepStartDeg);
    const float sweepAngleDeg = sweepStart + visualSliderPos * sweepSpan;
    float mappedVisualSliderPos = std::fmod(sweepAngleDeg, 360.0f) / 360.0f;
    if (mappedVisualSliderPos < 0.0f)
        mappedVisualSliderPos += 1.0f;

    struct FilmstripBlend
    {
        int baseIndex = 0;
        int nextIndex = 0;
        float nextAlpha = 0.0f;
    };

    const auto selectFilmstripFrameBlend = [requestedFrameCount](float mappedPos, int availableFrames, bool reverseOrder)
    {
        const int effectiveFrames = juce::jlimit(2, availableFrames, requestedFrameCount);
        const float clampedPos = juce::jlimit(0.0f, 1.0f, mappedPos);
        const float effectiveForwardPos = clampedPos * static_cast<float>(effectiveFrames - 1);
        const float remappedForwardPos = (effectiveFrames > 1)
            ? effectiveForwardPos * static_cast<float>(availableFrames - 1) / static_cast<float>(effectiveFrames - 1)
            : 0.0f;
        const float clampedFramePos = juce::jlimit(0.0f, static_cast<float>(availableFrames - 1), remappedForwardPos);
        const float framePos = reverseOrder
            ? static_cast<float>(availableFrames - 1) - clampedFramePos
            : clampedFramePos;

        FilmstripBlend blend;
        blend.baseIndex = juce::jlimit(0, availableFrames - 1, static_cast<int>(std::floor(framePos)));
        blend.nextIndex = juce::jlimit(0, availableFrames - 1, blend.baseIndex + 1);
        blend.nextAlpha = juce::jlimit(0.0f, 1.0f, framePos - static_cast<float>(blend.baseIndex));

        if (blend.baseIndex == blend.nextIndex)
            blend.nextAlpha = 0.0f;

        return blend;
    };

    const auto drawBlendedFilmstrip = [&](const juce::Image& sheet,
                                          int numColumns,
                                          int numRows,
                                          int frameWidth,
                                          int frameHeight,
                                          int paddingX,
                                          int paddingY,
                                          int stepX,
                                          int stepY,
                                          float overallAlpha = 1.0f,
                                          bool reverseOrder = true,
                                          bool keepBaseOpaque = false,
                                          bool snapToNearestFrame = false)
    {
        if (overallAlpha <= 0.001f)
            return false;

        const int numFrames = numColumns * numRows;
        const auto blend = selectFilmstripFrameBlend(mappedVisualSliderPos, numFrames, reverseOrder);

        const auto drawFrame = [&](int frameIndex, float alpha)
        {
            if (alpha <= 0.001f)
                return false;

            const int col = frameIndex % numColumns;
            const int row = frameIndex / numColumns;
            const juce::Rectangle<int> srcRect(paddingX + col * stepX, paddingY + row * stepY,
                                               frameWidth, frameHeight);

            if (!sheet.getBounds().contains(srcRect))
                return false;

            g.saveState();
            g.setOpacity(alpha * overallAlpha);
            g.drawImage(sheet, x, y, width, height,
                        srcRect.getX(), srcRect.getY(), srcRect.getWidth(), srcRect.getHeight());
            g.restoreState();
            return true;
        };

        if (snapToNearestFrame)
        {
            const int snappedFrame = (blend.nextAlpha >= 0.5f) ? blend.nextIndex : blend.baseIndex;
            return drawFrame(snappedFrame, 1.0f);
        }

        const float baseAlpha = keepBaseOpaque ? 1.0f : (1.0f - blend.nextAlpha);
        bool drew = drawFrame(blend.baseIndex, baseAlpha);
        if (blend.nextIndex != blend.baseIndex)
            drew = drawFrame(blend.nextIndex, blend.nextAlpha) || drew;
        return drew;
    };

    const auto drawCrossfadedFilmstripPair = [&](const juce::Image& offSheet,
                                                 const juce::Image& onSheet,
                                                 int numColumns,
                                                 int numRows,
                                                 int frameWidth,
                                                 int frameHeight,
                                                 int paddingX,
                                                 int paddingY,
                                                 int stepX,
                                                 int stepY,
                                                 float onBlend,
                                                 bool reverseOrder = true)
    {
        const int numFrames = numColumns * numRows;
        const auto blend = selectFilmstripFrameBlend(mappedVisualSliderPos, numFrames, reverseOrder);
        const int frameIndex = (blend.nextAlpha >= 0.5f) ? blend.nextIndex : blend.baseIndex;
        const int col = frameIndex % numColumns;
        const int row = frameIndex / numColumns;
        const juce::Rectangle<int> srcRect(paddingX + col * stepX, paddingY + row * stepY,
                                           frameWidth, frameHeight);

        if (!offSheet.getBounds().contains(srcRect) || !onSheet.getBounds().contains(srcRect))
            return false;

        const auto offFrame = offSheet.getClippedImage(srcRect);
        const auto onFrame = onSheet.getClippedImage(srcRect);
        if (!offFrame.isValid() || !onFrame.isValid())
            return false;

        juce::Image blendedFrame(juce::Image::ARGB, frameWidth, frameHeight, true);
        juce::Image::BitmapData offData(offFrame, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData onData(onFrame, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData blendedData(blendedFrame, 0, 0, frameWidth, frameHeight, juce::Image::BitmapData::writeOnly);

        const float t = juce::jlimit(0.0f, 1.0f, onBlend);
        const float invT = 1.0f - t;

        for (int py = 0; py < frameHeight; ++py)
        {
            auto* offPixels = reinterpret_cast<juce::PixelARGB*>(offData.getLinePointer(py));
            auto* onPixels = reinterpret_cast<juce::PixelARGB*>(onData.getLinePointer(py));
            auto* outPixels = reinterpret_cast<juce::PixelARGB*>(blendedData.getLinePointer(py));

            for (int px = 0; px < frameWidth; ++px)
            {
                auto offPixel = offPixels[px];
                auto onPixel = onPixels[px];
                offPixel.unpremultiply();
                onPixel.unpremultiply();

                const float offA = static_cast<float>(offPixel.getAlpha()) / 255.0f;
                const float onA = static_cast<float>(onPixel.getAlpha()) / 255.0f;

                const float blendedA = offA * invT + onA * t;
                const float blendedRPremul = (static_cast<float>(offPixel.getRed()) * offA) * invT
                                           + (static_cast<float>(onPixel.getRed()) * onA) * t;
                const float blendedGPremul = (static_cast<float>(offPixel.getGreen()) * offA) * invT
                                           + (static_cast<float>(onPixel.getGreen()) * onA) * t;
                const float blendedBPremul = (static_cast<float>(offPixel.getBlue()) * offA) * invT
                                           + (static_cast<float>(onPixel.getBlue()) * onA) * t;

                const juce::uint8 outA = static_cast<juce::uint8>(std::round(juce::jlimit(0.0f, 255.0f, blendedA * 255.0f)));
                juce::uint8 outR = 0;
                juce::uint8 outG = 0;
                juce::uint8 outB = 0;

                if (blendedA > 1.0e-6f)
                {
                    outR = static_cast<juce::uint8>(std::round(juce::jlimit(0.0f, 255.0f, blendedRPremul / blendedA)));
                    outG = static_cast<juce::uint8>(std::round(juce::jlimit(0.0f, 255.0f, blendedGPremul / blendedA)));
                    outB = static_cast<juce::uint8>(std::round(juce::jlimit(0.0f, 255.0f, blendedBPremul / blendedA)));
                }

                outPixels[px].setARGB(outA, outR, outG, outB);
            }
        }

        g.drawImage(blendedFrame, x, y, width, height, 0, 0, frameWidth, frameHeight);
        return true;
    };

    const auto drawBlendedFilmstripWithin = [&](const juce::Image& sheet,
                                                int numColumns,
                                                int numRows,
                                                int frameWidth,
                                                int frameHeight,
                                                int paddingX,
                                                int paddingY,
                                                int stepX,
                                                int stepY,
                                                int drawHeight,
                                                float overallAlpha = 1.0f,
                                                bool reverseOrder = true,
                                                bool snapToNearestFrame = false)
    {
        if (overallAlpha <= 0.001f)
            return false;

        const int numFrames = numColumns * numRows;
        const auto blend = selectFilmstripFrameBlend(mappedVisualSliderPos, numFrames, reverseOrder);

        const auto drawFrame = [&](int frameIndex, float alpha)
        {
            if (alpha <= 0.001f)
                return false;

            const int col = frameIndex % numColumns;
            const int row = frameIndex / numColumns;
            const juce::Rectangle<int> srcRect(paddingX + col * stepX, paddingY + row * stepY,
                                               frameWidth, frameHeight);
            const auto clip = srcRect.getIntersection(sheet.getBounds());

            if (clip.isEmpty())
                return false;

            const auto frame = sheet.getClippedImage(clip);
            if (!frame.isValid())
                return false;

            g.saveState();
            g.setOpacity(alpha * overallAlpha);
            g.drawImageWithin(frame, x, y, width, drawHeight,
                              juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                              false);
            g.restoreState();
            return true;
        };

        if (snapToNearestFrame)
        {
            const int snappedFrame = (blend.nextAlpha >= 0.5f) ? blend.nextIndex : blend.baseIndex;
            return drawFrame(snappedFrame, 1.0f);
        }

        bool drew = drawFrame(blend.baseIndex, 1.0f - blend.nextAlpha);
        if (blend.nextIndex != blend.baseIndex)
            drew = drawFrame(blend.nextIndex, blend.nextAlpha) || drew;
        return drew;
    };

    // Main engine knobs now all use the optimized per-control on/off filmstrips.
    if (!isMixKnob)
    {
        const auto knobId = slider.getComponentID();
        const juce::Image* knobSheetOff = nullptr;
        const juce::Image* knobSheetOn = nullptr;
        if (knobId == "Rate" && knobSpriteSheetRateImage.isValid())
        {
            knobSheetOff = &knobSpriteSheetRateImage;
            knobSheetOn = knobSpriteSheetRateOnImage.isValid() ? &knobSpriteSheetRateOnImage : nullptr;
        }
        else if (knobId == "Depth" && knobSpriteSheetDepthImage.isValid())
        {
            knobSheetOff = &knobSpriteSheetDepthImage;
            knobSheetOn = knobSpriteSheetDepthOnImage.isValid() ? &knobSpriteSheetDepthOnImage : nullptr;
        }
        else if (knobId == "Offset" && knobSpriteSheetOffsetImage.isValid())
        {
            knobSheetOff = &knobSpriteSheetOffsetImage;
            knobSheetOn = knobSpriteSheetOffsetOnImage.isValid() ? &knobSpriteSheetOffsetOnImage : nullptr;
        }
        else if (knobId == "Width" && knobSpriteSheetWidthImage.isValid())
        {
            knobSheetOff = &knobSpriteSheetWidthImage;
            knobSheetOn = knobSpriteSheetWidthOnImage.isValid() ? &knobSpriteSheetWidthOnImage : nullptr;
        }
        else if (knobSpriteSheetRateImage.isValid())
        {
            knobSheetOff = &knobSpriteSheetRateImage;
            knobSheetOn = knobSpriteSheetRateOnImage.isValid() ? &knobSpriteSheetRateOnImage : nullptr;
        }

        if (knobSheetOff != nullptr)
        {
            const bool hasCrossfadePair = (knobSheetOn != nullptr);
            const float blendProgress = juce::jlimit(0.0f, 1.0f, hqAnimationProgress);

            float offAlpha = hqIsOn ? 0.0f : 1.0f;
            float onAlpha = hqIsOn ? 1.0f : 0.0f;

            if (hasCrossfadePair && hqAnimationActive)
            {
                if (drawCrossfadedFilmstripPair(*knobSheetOff, *knobSheetOn,
                                                10, 10, 300, 300, 12, 12, 312, 312,
                                                blendProgress, false))
                    return;
            }

            bool drew = drawBlendedFilmstrip(*knobSheetOff, 10, 10, 300, 300, 12, 12, 312, 312, offAlpha, false, false, true);
            if (hasCrossfadePair)
                drew = drawBlendedFilmstrip(*knobSheetOn, 10, 10, 300, 300, 12, 12, 312, 312, onAlpha, false, false, true) || drew;

            if (drew)
                return;
        }
    }

    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float angle = rotaryStartAngle + mappedVisualSliderPos * (rotaryEndAngle - rotaryStartAngle);

    if (isMixKnob && mixKnobSpriteSheetImage.isValid())
    {
        if (drawBlendedFilmstrip(mixKnobSpriteSheetImage, 13, 12, 512, 512, 64, 64, 576, 576, 1.0f, true, false, true))
            return;
    }

    if (knobBaseImage.isValid())
    {
        // Base remains static.
        g.drawImage(knobBaseImage, x, y, width, height, 0, 0,
                    knobBaseImage.getWidth(), knobBaseImage.getHeight());

        // Indicator is the only rotating layer.
        if (knobIndicatorImage.isValid())
        {
            g.saveState();
            g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
            g.drawImage(knobIndicatorImage, x, y, width, height, 0, 0,
                        knobIndicatorImage.getWidth(), knobIndicatorImage.getHeight());
            g.restoreState();
        }

        // Shadow overlay stays static above the rotating indicator.
        if (knobShadowOverlayImage.isValid())
        {
            g.drawImage(knobShadowOverlayImage, x, y, width, height, 0, 0,
                        knobShadowOverlayImage.getWidth(), knobShadowOverlayImage.getHeight());
        }
        return;
    }

    // Fallback to default
    LookAndFeel_V4::drawRotarySlider(g, x, y, width, height, sliderPos,
                                     rotaryStartAngle, rotaryEndAngle, slider);
}
// Codacy: Parameter count warning unavoidable - this is a JUCE override method
// that must match the base class signature (LookAndFeel_V4::drawLinearSlider)
void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

    if (style == juce::Slider::LinearHorizontal)
    {
        // Track is built into the back panel; only draw the thumb.
        
        // Calculate visual slider position (with smoothing if applicable).
        // Map normalized 0..1 directly into the constrained thumb range so
        // the thumb reaches both ends of the visible track.
        const float constraintOffset = static_cast<float>(width) * 0.115f;
        const float thumbMinPos = static_cast<float>(x) + constraintOffset;
        const float thumbMaxPos = static_cast<float>(x + width) - constraintOffset;

        float visualSliderPos;
        if (auto* smoothedSlider = dynamic_cast<SmoothedSlider*>(&slider))
        {
            const double minValue = slider.getMinimum();
            const double maxValue = slider.getMaximum();
            const double visualValue = smoothedSlider->getVisualValue();
            const float normalized = juce::jlimit(0.0f, 1.0f,
                static_cast<float>((visualValue - minValue) / (maxValue - minValue)));
            visualSliderPos = thumbMinPos + normalized * (thumbMaxPos - thumbMinPos);
        }
        else
        {
            const float clamped = juce::jlimit(0.0f, 1.0f, sliderPos);
            visualSliderPos = thumbMinPos + clamped * (thumbMaxPos - thumbMinPos);
        }

        drawSliderThumb(g, x, y, width, height, visualSliderPos);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                        minSliderPos, maxSliderPos, style, slider);
    }
}

void CustomLookAndFeel::drawSliderTrack(juce::Graphics& g, int x, int y, int width, int height)
{
    if (sliderTrackImage.isValid())
    {
        g.drawImage(sliderTrackImage, x, y, width, height, 0, 0,
                   sliderTrackImage.getWidth(), sliderTrackImage.getHeight());
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.fillRoundedRectangle(x, y + height * 0.4f, width, height * 0.2f, 2.0f);
    }
}

void CustomLookAndFeel::drawSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                                       float visualSliderPos)
{
    // visualSliderPos is already mapped into the constrained thumb range
    // by drawLinearSlider, so use it directly.
    const float constrainedSliderPos = visualSliderPos;

    // Calculate thumb size maintaining original aspect ratio (40x80 = 1:2)
    float thumbWidth, thumbHeight;
    if (sliderThumbImage.isValid())
    {
        const float imageAspectRatio = static_cast<float>(sliderThumbImage.getWidth()) / static_cast<float>(sliderThumbImage.getHeight());
        const float maxThumbHeight = height * 1.0f;  // Use full slider height for visibility
        const float maxThumbWidth = width * 0.2f;   // Allow thumb to scale with slider (was capped at 36)
        
        // Calculate dimensions maintaining aspect ratio
        thumbHeight = maxThumbHeight;
        thumbWidth = thumbHeight * imageAspectRatio;
        
        // If width exceeds max, scale down
        if (thumbWidth > maxThumbWidth)
        {
            thumbWidth = maxThumbWidth;
            thumbHeight = thumbWidth / imageAspectRatio;
        }
    }
    else
    {
        // Fallback if no image
        const float baseThumbWidth = juce::jmin(width * 0.1f, 30.0f);
        thumbWidth = baseThumbWidth * 0.8f;
        thumbHeight = height * 0.8f;
    }
    
    const float thumbX = constrainedSliderPos - thumbWidth * 0.5f;
    const float thumbY = y + (height - thumbHeight) * 0.5f;
    
    if (sliderThumbImage.isValid())
    {
        g.drawImage(sliderThumbImage, thumbX, thumbY, thumbWidth, thumbHeight,
                   0, 0, sliderThumbImage.getWidth(), sliderThumbImage.getHeight());
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);
    }
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const float borderWidth = 1.0f;
    const bool isOn = button.getToggleState();
    
    drawToggleBorder(g, bounds, isOn);
    
    const auto innerBounds = bounds.reduced(borderWidth);
    drawToggleBackground(g, innerBounds, isOn);
    drawToggleIndicator(g, innerBounds, isOn);
}

void CustomLookAndFeel::drawToggleBorder(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isOn)
{
    const float cornerRadius = 6.0f;
    juce::ColourGradient gradient;
    
    if (isOn)
    {
        gradient = juce::ColourGradient(
            juce::Colour(0xff4a6b5a), bounds.getX(), bounds.getY(),
            juce::Colour(0xff2a3b32), bounds.getX(), bounds.getBottom(), false);
    }
    else
    {
        gradient = juce::ColourGradient(
            juce::Colour(0xff606060), bounds.getX(), bounds.getY(),
            juce::Colour(0xff404040), bounds.getX(), bounds.getBottom(), false);
    }
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
}

void CustomLookAndFeel::drawToggleBackground(juce::Graphics& g, const juce::Rectangle<float>& innerBounds, bool isOn)
{
    const float cornerRadius = 5.0f; // 6.0f - 1.0f border
    
    if (isOn)
        g.setColour(juce::Colour(0xff2a4a3a).withAlpha(0.9f));
    else
        g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f));
    
    g.fillRoundedRectangle(innerBounds, cornerRadius);
}

void CustomLookAndFeel::drawToggleIndicator(juce::Graphics& g, const juce::Rectangle<float>& innerBounds, bool isOn)
{
    const float indicatorSize = innerBounds.getHeight() * 0.7f;
    const float indicatorY = innerBounds.getCentreY() - indicatorSize * 0.5f;
    const float indicatorX = isOn 
        ? (innerBounds.getRight() - indicatorSize - 4.0f)
        : (innerBounds.getX() + 4.0f);
    
    juce::ColourGradient indicatorGradient;
    if (isOn)
    {
        indicatorGradient = juce::ColourGradient(
            juce::Colour(0xff9dbd78), indicatorX, indicatorY,
            juce::Colour(0xff6b8d5a), indicatorX, indicatorY + indicatorSize, false);
    }
    else
    {
        indicatorGradient = juce::ColourGradient(
            juce::Colour(0xff808080), indicatorX, indicatorY,
            juce::Colour(0xff505050), indicatorX, indicatorY + indicatorSize, false);
    }
    
    g.setGradientFill(indicatorGradient);
    g.fillEllipse(indicatorX, indicatorY, indicatorSize, indicatorSize);
    
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.fillEllipse(indicatorX + 1.0f, indicatorY + 1.0f, indicatorSize * 0.3f, indicatorSize * 0.3f);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    auto boxArea = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    const auto accent = getThemeAccentColour();

    auto baseBackground = box.findColour(juce::ComboBox::backgroundColourId);
    if (baseBackground.getAlpha() == 0)
        baseBackground = getThemePanelColour().withAlpha(0.82f);

    auto outlineColour = box.findColour(juce::ComboBox::outlineColourId);
    if (outlineColour.getAlpha() == 0)
        outlineColour = getThemePanelOutlineColour();

    juce::ColourGradient gradient(baseBackground.brighter(isButtonDown ? 0.08f : 0.15f),
                                  boxArea.getX(), boxArea.getY(),
                                  baseBackground.darker(0.32f).interpolatedWith(accent, 0.06f),
                                  boxArea.getX(), boxArea.getBottom(),
                                  false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(boxArea, 4.0f);

    if (buttonW > 0 && buttonH > 0)
    {
        const auto arrowZone = juce::Rectangle<float>(static_cast<float>(buttonX),
                                                      static_cast<float>(buttonY),
                                                      static_cast<float>(buttonW),
                                                      static_cast<float>(buttonH));
        g.setColour(accent.withAlpha(0.18f));
        g.fillRoundedRectangle(arrowZone.reduced(1.0f), 3.0f);

        g.setColour(outlineColour.withAlpha(0.35f));
        g.drawLine(arrowZone.getX(), arrowZone.getY() + 2.0f, arrowZone.getX(), arrowZone.getBottom() - 2.0f, 1.0f);
    }

    g.setColour(outlineColour.withAlpha(box.isEnabled() ? 0.95f : 0.45f));
    g.drawRoundedRectangle(boxArea, 4.0f, 1.05f);

    juce::Path arrow;
    const float arrowSize = 6.0f;
    const float arrowX = static_cast<float>(buttonX) + static_cast<float>(buttonW) * 0.5f;
    const float arrowY = static_cast<float>(buttonY) + static_cast<float>(buttonH) * 0.5f;
    
    g.setColour(box.findColour(juce::ComboBox::arrowColourId)
                    .withMultipliedAlpha(box.isEnabled() ? 0.95f : 0.45f));
    
    if (isButtonDown)
    {
        // Arrow pointing up when open
        arrow.addTriangle(arrowX, arrowY - arrowSize / 2,
                         arrowX - arrowSize / 2, arrowY + arrowSize / 2,
                         arrowX + arrowSize / 2, arrowY + arrowSize / 2);
    }
    else
    {
        // Arrow pointing down when closed
        arrow.addTriangle(arrowX, arrowY + arrowSize / 2,
                         arrowX - arrowSize / 2, arrowY - arrowSize / 2,
                         arrowX + arrowSize / 2, arrowY - arrowSize / 2);
    }
    
    g.fillPath(arrow);
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(3, 1, juce::jmax(1, box.getWidth() - 28), juce::jmax(1, box.getHeight() - 2));
    label.setFont(getComboBoxFont(box));
    label.setJustificationType(box.getJustificationType());
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto accent = getThemeAccentColour();
    auto base = findColour(juce::PopupMenu::backgroundColourId);
    if (base.getAlpha() == 0)
        base = getThemePanelColour().withAlpha(0.98f);

    g.fillAll(base.darker(0.45f));

    const auto bounds = juce::Rectangle<float>(0.5f, 0.5f,
                                               static_cast<float>(width) - 1.0f,
                                               static_cast<float>(height) - 1.0f);
    juce::ColourGradient fill(base.brighter(0.08f),
                              bounds.getX(), bounds.getY(),
                              base.darker(0.25f).interpolatedWith(accent, 0.08f),
                              bounds.getX(), bounds.getBottom(),
                              false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds.reduced(0.6f), 6.0f);

    g.setColour(getThemePanelOutlineColour().withAlpha(0.92f));
    g.drawRoundedRectangle(bounds.reduced(0.6f), 6.0f, 1.15f);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                                          const juce::String& text, const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    if (isSeparator)
    {
        const auto separator = area.reduced(10, juce::jmax(1, area.getHeight() / 3));
        const float y = static_cast<float>(separator.getCentreY());
        g.setColour(getThemeAccentColour().withAlpha(0.42f));
        g.drawLine(static_cast<float>(separator.getX()), y,
                   static_cast<float>(separator.getRight()), y, 1.0f);
        return;
    }

    auto row = area.reduced(4, 2);
    const auto rowf = row.toFloat();
    const auto accent = getThemeAccentColour();

    if (isHighlighted)
    {
        auto hl = findColour(juce::PopupMenu::highlightedBackgroundColourId);
        if (hl.getAlpha() == 0)
            hl = accent.withAlpha(0.30f);
        g.setColour(hl);
        g.fillRoundedRectangle(rowf, 4.0f);
        g.setColour(accent.withAlpha(0.65f));
        g.drawRoundedRectangle(rowf, 4.0f, 1.0f);
    }

    juce::Colour textColour = textColourToUse != nullptr ? *textColourToUse
                                                          : findColour(juce::PopupMenu::textColourId);
    if (textColour.getAlpha() == 0)
        textColour = juce::Colours::white;

    if (isHighlighted)
    {
        const auto highlighted = findColour(juce::PopupMenu::highlightedTextColourId);
        if (highlighted.getAlpha() != 0)
            textColour = highlighted;
    }

    if (!isActive)
        textColour = textColour.withMultipliedAlpha(0.48f);

    auto iconArea = row.removeFromLeft(20);
    if (icon != nullptr)
    {
        icon->drawWithin(g, iconArea.toFloat().reduced(2.0f),
                         juce::RectanglePlacement::centred, 1.0f);
    }
    else if (isTicked)
    {
        juce::Path tick;
        tick.startNewSubPath(static_cast<float>(iconArea.getX() + 4), static_cast<float>(iconArea.getCentreY()));
        tick.lineTo(static_cast<float>(iconArea.getX() + 8), static_cast<float>(iconArea.getBottom() - 5));
        tick.lineTo(static_cast<float>(iconArea.getRight() - 4), static_cast<float>(iconArea.getY() + 5));
        g.setColour(accent.withMultipliedAlpha(isActive ? 1.0f : 0.55f));
        g.strokePath(tick, juce::PathStrokeType(2.0f));
    }

    const int shortcutWidth = shortcutKeyText.isNotEmpty() ? 70 : 0;
    auto shortcutArea = row.removeFromRight(shortcutWidth);
    auto arrowArea = row.removeFromRight(hasSubMenu ? 12 : 0);

    g.setColour(textColour);
    g.setFont(getPopupMenuFont());
    g.drawFittedText(text, row, juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour(textColour.withMultipliedAlpha(0.7f));
        g.setFont(getPopupMenuFont().withHeight(getPopupMenuFont().getHeight() * 0.85f));
        g.drawFittedText(shortcutKeyText, shortcutArea, juce::Justification::centredRight, 1);
    }

    if (hasSubMenu)
    {
        juce::Path submenuArrow;
        const float cx = static_cast<float>(arrowArea.getCentreX());
        const float cy = static_cast<float>(arrowArea.getCentreY());
        submenuArrow.startNewSubPath(cx - 2.0f, cy - 4.0f);
        submenuArrow.lineTo(cx + 2.0f, cy);
        submenuArrow.lineTo(cx - 2.0f, cy + 4.0f);
        g.setColour(textColour.withMultipliedAlpha(0.8f));
        g.strokePath(submenuArrow, juce::PathStrokeType(1.4f));
    }
}

void CustomLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                   const juce::String& sectionName)
{
    auto header = area.reduced(4, 2).toFloat();
    const auto accent = getThemeAccentColour();

    g.setColour(accent.withAlpha(0.22f));
    g.fillRoundedRectangle(header, 4.0f);
    g.setColour(accent.withAlpha(0.65f));
    g.drawRoundedRectangle(header, 4.0f, 1.0f);

    auto textColour = findColour(juce::PopupMenu::headerTextColourId);
    if (textColour.getAlpha() == 0)
        textColour = accent.brighter(0.4f);

    g.setColour(textColour);
    g.setFont(getPopupMenuFont().boldened());
    g.drawFittedText(sectionName, area.reduced(10, 0), juce::Justification::centredLeft, 1);
}

void CustomLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                                  int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
    LookAndFeel_V4::getIdealPopupMenuItemSize(text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight);
    if (isSeparator)
    {
        idealHeight = juce::jmax(6, idealHeight);
        return;
    }

    idealHeight = juce::jmax(22, juce::jmax(standardMenuItemHeight, idealHeight));
    idealWidth += 16;
}

void CustomLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto bounds = juce::Rectangle<float>(0.5f, 0.5f,
                                         static_cast<float>(width) - 1.0f,
                                         static_cast<float>(height) - 1.0f);
    auto bg = findColour(juce::TooltipWindow::backgroundColourId);
    if (bg.getAlpha() == 0)
        bg = getThemePanelColour().darker(0.28f).withAlpha(0.97f);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 5.0f);

    auto outline = findColour(juce::TooltipWindow::outlineColourId);
    if (outline.getAlpha() == 0)
        outline = getThemePanelOutlineColour();
    g.setColour(outline.withAlpha(0.9f));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    auto textColour = findColour(juce::TooltipWindow::textColourId);
    if (textColour.getAlpha() == 0)
        textColour = juce::Colours::white;
    g.setColour(textColour);
    g.setFont(getPopupMenuFont().withHeight(juce::jmax(11.0f, getPopupMenuFont().getHeight() * 0.95f)));
    g.drawFittedText(text, juce::Rectangle<int>(width, height).reduced(8, 4), juce::Justification::centredLeft, 6);
}

void CustomLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics& g,
                                                 const juce::Path& path, juce::Image& cachedImage)
{
    juce::ignoreUnused(cachedImage);
    const auto fill = getThemePanelColour().darker(0.2f).withAlpha(0.97f);
    g.setColour(fill);
    g.fillPath(path);

    g.setColour(getThemePanelOutlineColour().withAlpha(0.9f));
    g.strokePath(path, juce::PathStrokeType(1.15f));
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    const auto customHeightVar = box.getProperties().getWithDefault("customFontHeight", 0.0);
    const float customHeight = static_cast<float>(customHeightVar);
    const float height = customHeight > 0.0f ? customHeight
                                             : juce::jmax(10.0f, static_cast<float>(box.getHeight()) * 0.75f);
    if (uiTextTypeface != nullptr)
        return juce::Font { juce::FontOptions { uiTextTypeface }.withHeight(height) };
    return LookAndFeel_V4::getComboBoxFont(box);
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    if (uiTextTypeface != nullptr)
    {
        const float fallback = LookAndFeel_V4::getPopupMenuFont().getHeight();
        const float height = popupMenuFontHeight > 0.0f ? popupMenuFontHeight : fallback;
        return juce::Font { juce::FontOptions { uiTextTypeface }.withHeight(height) };
    }
    return LookAndFeel_V4::getPopupMenuFont();
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    const auto customHeightVar = button.getProperties().getWithDefault("customFontHeight", 0.0);
    const float customHeight = static_cast<float>(customHeightVar);
    const float height = customHeight > 0.0f ? customHeight
                                             : juce::jmax(10.0f, static_cast<float>(buttonHeight) * 0.68f);
    if (uiTextTypeface != nullptr)
        return juce::Font { juce::FontOptions { uiTextTypeface }.withHeight(height) };
    return LookAndFeel_V4::getTextButtonFont(button, buttonHeight);
}

juce::Font CustomLookAndFeel::getLabelFont(juce::Label& label)
{
    if (const auto* valueLabel = dynamic_cast<const LabelWithContainer*>(&label))
    {
        if (valueLabel->isValueLabelStyleEnabled())
            return label.getFont();
    }

    if (uiTextTypeface != nullptr)
    {
        const float height = juce::jmax(10.0f, label.getFont().getHeight());
        return juce::Font { juce::FontOptions { uiTextTypeface }.withHeight(height) };
    }
    return LookAndFeel_V4::getLabelFont(label);
}

void CustomLookAndFeel::setUiTextTypeface(juce::Typeface::Ptr typeface)
{
    uiTextTypeface = std::move(typeface);
}
