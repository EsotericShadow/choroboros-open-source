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
#include "BinaryData.h"
#include <cmath>

// Returns a copy of the image with every non-transparent pixel set to full opacity (fixes PNGs exported at <100% opacity).
static juce::Image makeImageFullyOpaque(const juce::Image& image)
{
    juce::Image copy = image.createCopy();
    if (copy.getFormat() != juce::Image::ARGB)
        return copy;
    juce::Image::BitmapData bd(copy, 0, 0, copy.getWidth(), copy.getHeight(), juce::Image::BitmapData::readWrite);
    for (int y = 0; y < bd.height; ++y)
    {
        auto* p = reinterpret_cast<juce::PixelARGB*>(bd.getLinePointer(y));
        for (int x = 0; x < bd.width; ++x)
        {
            const juce::uint8 a = p[x].getAlpha();
            if (a > 0 && a < 255)
            {
                p[x].unpremultiply();
                p[x].setAlpha(255);
                p[x].premultiply();
            }
        }
    }
    return copy;
}

CustomLookAndFeel::CustomLookAndFeel()
{
    loadImages(0); // Default to Green
}

void CustomLookAndFeel::setColorTheme(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    if (currentColorIndex != colorIndex)
    {
        currentColorIndex = colorIndex;
        loadImages(colorIndex);
    }
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
    const char* knobBaseName = nullptr;
    int knobBaseSize = 0;
    const char* indicatorName = nullptr;
    int indicatorSize = 0;
    const char* shadowName = nullptr;
    int shadowSize = 0;
    const char* trackName = nullptr;
    int trackSize = 0;
    const char* thumbName = nullptr;
    int thumbSize = 0;
    const char* mixKnobName = nullptr;
    int mixKnobSize = 0;
    const char* knobSheetRateName = nullptr;  int knobSheetRateSize = 0;
    const char* knobSheetDepthName = nullptr; int knobSheetDepthSize = 0;
    const char* knobSheetOffsetName = nullptr; int knobSheetOffsetSize = 0;
    const char* knobSheetWidthName = nullptr; int knobSheetWidthSize = 0;
    const char* mixKnobSpriteSheetName = nullptr;
    int mixKnobSpriteSheetSize = 0;
    
    getImageDataForColor(colorIndex,
                         knobBaseName, knobBaseSize,
                         indicatorName, indicatorSize,
                         shadowName, shadowSize,
                         trackName, trackSize,
                         thumbName, thumbSize,
                         mixKnobName, mixKnobSize,
                         knobSheetRateName, knobSheetRateSize,
                         knobSheetDepthName, knobSheetDepthSize,
                         knobSheetOffsetName, knobSheetOffsetSize,
                         knobSheetWidthName, knobSheetWidthSize,
                         mixKnobSpriteSheetName, mixKnobSpriteSheetSize);
    
    if (knobBaseName && knobBaseSize > 0)
        knobBaseImage = juce::ImageCache::getFromMemory(knobBaseName, knobBaseSize);

    if (indicatorName && indicatorSize > 0)
        knobIndicatorImage = juce::ImageCache::getFromMemory(indicatorName, indicatorSize);

    if (shadowName && shadowSize > 0)
        knobShadowOverlayImage = juce::ImageCache::getFromMemory(shadowName, shadowSize);
    
    if (trackName && trackSize > 0)
        sliderTrackImage = juce::ImageCache::getFromMemory(trackName, trackSize);
    
    if (thumbName && thumbSize > 0)
        sliderThumbImage = juce::ImageCache::getFromMemory(thumbName, thumbSize);
    
    if (mixKnobName && mixKnobSize > 0)
        mixKnobImage = juce::ImageCache::getFromMemory(mixKnobName, mixKnobSize);

    auto loadKnobSheet = [](const char* name, int size) -> juce::Image {
        if (!name || size <= 0)
            return {};
        auto image = juce::ImageFileFormat::loadFrom(name, static_cast<size_t>(size));
        if (!image.isValid())
            image = juce::ImageCache::getFromMemory(name, size);
        if (image.isValid() && image.getFormat() == juce::Image::ARGB)
        {
            const int sampleX = juce::jmin(192, image.getWidth() - 1);
            const int sampleY = juce::jmin(192, image.getHeight() - 1);
            if (sampleX >= 0 && sampleY >= 0)
            {
                const juce::uint8 alpha = image.getPixelAt(sampleX, sampleY).getAlpha();
                if (alpha > 0 && alpha < 255)
                    image = makeImageFullyOpaque(image);
            }
        }
        return image;
    };

    knobSpriteSheetRateImage = loadKnobSheet(knobSheetRateName, knobSheetRateSize);
    knobSpriteSheetDepthImage = loadKnobSheet(knobSheetDepthName, knobSheetDepthSize);
    knobSpriteSheetOffsetImage = loadKnobSheet(knobSheetOffsetName, knobSheetOffsetSize);
    knobSpriteSheetWidthImage = loadKnobSheet(knobSheetWidthName, knobSheetWidthSize);
    mixKnobSpriteSheetImage = loadKnobSheet(mixKnobSpriteSheetName, mixKnobSpriteSheetSize);
}

void CustomLookAndFeel::getImageDataForColor(int colorIndex, const char*& knobBaseName, int& knobBaseSize,
                                             const char*& indicatorName, int& indicatorSize,
                                             const char*& shadowName, int& shadowSize,
                                             const char*& trackName, int& trackSize,
                                             const char*& thumbName, int& thumbSize,
                                             const char*& mixKnobName, int& mixKnobSize,
                                             const char*& knobSheetRateName, int& knobSheetRateSize,
                                             const char*& knobSheetDepthName, int& knobSheetDepthSize,
                                             const char*& knobSheetOffsetName, int& knobSheetOffsetSize,
                                             const char*& knobSheetWidthName, int& knobSheetWidthSize,
                                             const char*& mixKnobSpriteSheetName, int& mixKnobSpriteSheetSize)
{
    knobSheetRateName = nullptr;   knobSheetRateSize = 0;
    knobSheetDepthName = nullptr;  knobSheetDepthSize = 0;
    knobSheetOffsetName = nullptr; knobSheetOffsetSize = 0;
    knobSheetWidthName = nullptr;  knobSheetWidthSize = 0;
    mixKnobSpriteSheetName = nullptr;
    mixKnobSpriteSheetSize = 0;

    if (colorIndex == 0) // Green (uses spritesheets only, no base/indicator/shadow)
    {
        knobBaseName = nullptr;
        knobBaseSize = 0;
        indicatorName = nullptr;
        indicatorSize = 0;
        shadowName = nullptr;
        shadowSize = 0;
        trackName = nullptr;
        trackSize = 0;
        thumbName = BinaryData::green_slider_thumb_png;
        thumbSize = BinaryData::green_slider_thumb_pngSize;
        mixKnobName = nullptr;
        mixKnobSize = 0;
        knobSheetRateName = BinaryData::rate_knob_spritesheet_png;
        knobSheetRateSize = BinaryData::rate_knob_spritesheet_pngSize;
        knobSheetDepthName = BinaryData::depth_knob_spritesheet_png;
        knobSheetDepthSize = BinaryData::depth_knob_spritesheet_pngSize;
        knobSheetOffsetName = BinaryData::offset_knob_spritesheet_png;
        knobSheetOffsetSize = BinaryData::offset_knob_spritesheet_pngSize;
        knobSheetWidthName = BinaryData::width_knob_spritesheet_png;
        knobSheetWidthSize = BinaryData::width_knob_spritesheet_pngSize;
        mixKnobSpriteSheetName = BinaryData::mix_knob_spritesheet_png;
        mixKnobSpriteSheetSize = BinaryData::mix_knob_spritesheet_pngSize;
    }
    else if (colorIndex == 1) // Blue (spritesheets only)
    {
        knobBaseName = nullptr;
        knobBaseSize = 0;
        indicatorName = nullptr;
        indicatorSize = 0;
        shadowName = nullptr;
        shadowSize = 0;
        trackName = nullptr;
        trackSize = 0;
        thumbName = BinaryData::blue_slider_thumb_png;
        thumbSize = BinaryData::blue_slider_thumb_pngSize;
        mixKnobName = nullptr;
        mixKnobSize = 0;
        knobSheetRateName = BinaryData::Blue_knob_spritesheet_png;
        knobSheetRateSize = BinaryData::Blue_knob_spritesheet_pngSize;
        mixKnobSpriteSheetName = BinaryData::blue_mix_knob_spritesheet_png;
        mixKnobSpriteSheetSize = BinaryData::blue_mix_knob_spritesheet_pngSize;
    }
    else if (colorIndex == 2) // Red (spritesheets only, per-control like Green)
    {
        knobBaseName = nullptr;
        knobBaseSize = 0;
        indicatorName = nullptr;
        indicatorSize = 0;
        shadowName = nullptr;
        shadowSize = 0;
        trackName = nullptr;
        trackSize = 0;
        thumbName = BinaryData::red_slider_thumb_png;
        thumbSize = BinaryData::red_slider_thumb_pngSize;
        mixKnobName = nullptr;
        mixKnobSize = 0;
        knobSheetRateName = BinaryData::red_rate_knob_spritesheet_png;
        knobSheetRateSize = BinaryData::red_rate_knob_spritesheet_pngSize;
        knobSheetDepthName = BinaryData::red_depth_knob_spritesheet_png;
        knobSheetDepthSize = BinaryData::red_depth_knob_spritesheet_pngSize;
        knobSheetOffsetName = BinaryData::red_offset_knob_spritesheet_png;
        knobSheetOffsetSize = BinaryData::red_offset_knob_spritesheet_pngSize;
        knobSheetWidthName = BinaryData::red_width_knob_spritesheet_png;
        knobSheetWidthSize = BinaryData::red_width_knob_spritesheet_pngSize;
        mixKnobSpriteSheetName = BinaryData::red_mix_knob_spritesheet_png;
        mixKnobSpriteSheetSize = BinaryData::red_mix_knob_spritesheet_pngSize;
    }
    else if (colorIndex == 3) // Purple (spritesheets only)
    {
        knobBaseName = nullptr;
        knobBaseSize = 0;
        indicatorName = nullptr;
        indicatorSize = 0;
        shadowName = nullptr;
        shadowSize = 0;
        trackName = nullptr;
        trackSize = 0;
        thumbName = BinaryData::purple_slider_thumb_png;
        thumbSize = BinaryData::purple_slider_thumb_pngSize;
        mixKnobName = nullptr;
        mixKnobSize = 0;
        knobSheetRateName = BinaryData::purple_knob_spritesheet_png;
        knobSheetRateSize = BinaryData::purple_knob_spritesheet_pngSize;
        mixKnobSpriteSheetName = BinaryData::purple_mix_knob_spritesheet_png;
        mixKnobSpriteSheetSize = BinaryData::purple_mix_knob_spritesheet_pngSize;
    }
    else // Black (colorIndex == 4, spritesheets only)
    {
        knobBaseName = nullptr;
        knobBaseSize = 0;
        indicatorName = nullptr;
        indicatorSize = 0;
        shadowName = nullptr;
        shadowSize = 0;
        trackName = nullptr;
        trackSize = 0;
        thumbName = BinaryData::black__slider_thumb_png;
        thumbSize = BinaryData::black__slider_thumb_pngSize;
        mixKnobName = nullptr;
        mixKnobSize = 0;
        knobSheetRateName = BinaryData::black_Knob_spritesheet_png;
        knobSheetRateSize = BinaryData::black_Knob_spritesheet_pngSize;
        mixKnobSpriteSheetName = BinaryData::black_mix_knob_spritesheet_png;
        mixKnobSpriteSheetSize = BinaryData::black_mix_knob_spritesheet_pngSize;
    }
}

// Codacy: Parameter count warning unavoidable - this is a JUCE override method
// that must match the base class signature (LookAndFeel_V4::drawRotarySlider)
void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
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

    const auto selectFilmstripFrameIndex = [requestedFrameCount](float mappedPos, int availableFrames)
    {
        const int effectiveFrames = juce::jlimit(2, availableFrames, requestedFrameCount);
        const float clampedPos = juce::jlimit(0.0f, 1.0f, mappedPos);
        const int effectiveForwardIndex = juce::jlimit(0, effectiveFrames - 1,
            juce::roundToInt(clampedPos * static_cast<float>(effectiveFrames - 1)));
        const int remappedForwardIndex = (effectiveFrames > 1)
            ? juce::roundToInt(static_cast<float>(effectiveForwardIndex) * static_cast<float>(availableFrames - 1) / static_cast<float>(effectiveFrames - 1))
            : 0;
        return (availableFrames - 1) - juce::jlimit(0, availableFrames - 1, remappedForwardIndex);
    };

    // Green theme filmstrip knobs (individual sheets per control)
    if (currentColorIndex == 0 && !isMixKnob)
    {
        const auto knobId = slider.getComponentID();
        const juce::Image* knobSheet = nullptr;
        if (knobId == "Rate" && knobSpriteSheetRateImage.isValid())
            knobSheet = &knobSpriteSheetRateImage;
        else if (knobId == "Depth" && knobSpriteSheetDepthImage.isValid())
            knobSheet = &knobSpriteSheetDepthImage;
        else if (knobId == "Offset" && knobSpriteSheetOffsetImage.isValid())
            knobSheet = &knobSpriteSheetOffsetImage;
        else if (knobId == "Width" && knobSpriteSheetWidthImage.isValid())
            knobSheet = &knobSpriteSheetWidthImage;
        else if (knobSpriteSheetRateImage.isValid())
            knobSheet = &knobSpriteSheetRateImage;

        if (knobSheet)
        {
            // 12 rows x 13 columns, 512x512 px per frame, 12px padding
            const int numColumns = 13;
            const int numRows = 12;
            const int numFrames = numColumns * numRows; // 156
            const int frameWidth = 512;
            const int frameHeight = 512;
            const int padding = 12;
            const int stepX = frameWidth + padding;
            const int stepY = frameHeight + padding;

            const int frameIndex = selectFilmstripFrameIndex(mappedVisualSliderPos, numFrames);
            const int col = frameIndex % numColumns;
            const int row = frameIndex / numColumns;

            // Sheets include an outer padding border, so frame origin starts at +padding.
            const int sx = padding + col * stepX;
            const int sy = padding + row * stepY;

            // Extract exactly one frame so we never draw the full spritesheet
            const juce::Rectangle<int> srcRect(sx, sy, frameWidth, frameHeight);
            const juce::Rectangle<int> imgBounds(0, 0, knobSheet->getWidth(), knobSheet->getHeight());
            const juce::Rectangle<int> clip = srcRect.getIntersection(imgBounds);
            if (clip.isEmpty())
                return;
            const juce::Image frame = knobSheet->getClippedImage(clip);
            if (!frame.isValid())
                return;

            g.saveState();
            g.setOpacity(1.0f);
            g.drawImage(frame,
                        x, y, width, height,
                        0, 0, frame.getWidth(), frame.getHeight());
            g.restoreState();
            return;
        }
    }

    // Blue theme filmstrip knob (shared sheet for all main knobs)
    if (currentColorIndex == 1 && !isMixKnob && knobSpriteSheetRateImage.isValid())
    {
        const juce::Image& knobSheet = knobSpriteSheetRateImage;
        const int numColumns = 13;
        const int numRows = 12;
        const int numFrames = numColumns * numRows; // 156
        const int frameWidth = 512;
        const int frameHeight = 512;
        const int padding = 12;
        const int stepX = frameWidth + padding;
        const int stepY = frameHeight + padding;

        const int frameIndex = selectFilmstripFrameIndex(mappedVisualSliderPos, numFrames);
        const int col = frameIndex % numColumns;
        const int row = frameIndex / numColumns;

        const int sx = padding + col * stepX;
        const int sy = padding + row * stepY;

        const juce::Rectangle<int> srcRect(sx, sy, frameWidth, frameHeight);
        const juce::Rectangle<int> imgBounds(0, 0, knobSheet.getWidth(), knobSheet.getHeight());
        const juce::Rectangle<int> clip = srcRect.getIntersection(imgBounds);
        if (!clip.isEmpty())
        {
            const juce::Image frame = knobSheet.getClippedImage(clip);
            if (frame.isValid())
            {
                g.saveState();
                g.setOpacity(1.0f);
                g.drawImage(frame, x, y, width, height, 0, 0, frame.getWidth(), frame.getHeight());
                g.restoreState();
                return;
            }
        }
    }

    // Red theme filmstrip knobs (individual sheets per control, like Green)
    if (currentColorIndex == 2 && !isMixKnob)
    {
        const auto knobId = slider.getComponentID();
        const juce::Image* knobSheet = nullptr;
        if (knobId == "Rate" && knobSpriteSheetRateImage.isValid())
            knobSheet = &knobSpriteSheetRateImage;
        else if (knobId == "Depth" && knobSpriteSheetDepthImage.isValid())
            knobSheet = &knobSpriteSheetDepthImage;
        else if (knobId == "Offset" && knobSpriteSheetOffsetImage.isValid())
            knobSheet = &knobSpriteSheetOffsetImage;
        else if (knobId == "Width" && knobSpriteSheetWidthImage.isValid())
            knobSheet = &knobSpriteSheetWidthImage;
        else if (knobSpriteSheetRateImage.isValid())
            knobSheet = &knobSpriteSheetRateImage;

        if (knobSheet)
        {
            // Red sheets: 13 columns x 12 rows = 156 frames, 384x384 px per frame, 12px padding
            const int numColumns = 13;
            const int numRows = 12;
            const int numFrames = numColumns * numRows; // 156
            const int frameWidth = 384;
            const int frameHeight = 384;
            const int padding = 12;
            const int stepX = frameWidth + padding;
            const int stepY = frameHeight + padding;

            const int frameIndex = selectFilmstripFrameIndex(mappedVisualSliderPos, numFrames);
            const int col = frameIndex % numColumns;
            const int row = frameIndex / numColumns;

            const int sx = padding + col * stepX;
            const int sy = padding + row * stepY;

            const juce::Rectangle<int> srcRect(sx, sy, frameWidth, frameHeight);
            const juce::Rectangle<int> imgBounds(0, 0, knobSheet->getWidth(), knobSheet->getHeight());
            const juce::Rectangle<int> clip = srcRect.getIntersection(imgBounds);
            if (!clip.isEmpty())
            {
                const juce::Image frame = knobSheet->getClippedImage(clip);
                if (frame.isValid())
                {
                    g.saveState();
                    g.setOpacity(1.0f);
                    g.drawImage(frame, x, y, width, height, 0, 0, frame.getWidth(), frame.getHeight());
                    g.restoreState();
                    return;
                }
            }
        }
    }

    // Purple theme filmstrip knob (shared sheet for all main knobs)
    if (currentColorIndex == 3 && !isMixKnob && knobSpriteSheetRateImage.isValid())
    {
        const juce::Image& knobSheet = knobSpriteSheetRateImage;
        const int numColumns = 13;
        const int numRows = 12;
        const int numFrames = numColumns * numRows; // 156
        const int frameWidth = 512;
        const int frameHeight = 512;
        // Derive spacing from the sheet dimensions so purple works even if exported
        // without the same padding layout used by other themes.
        const int padding = juce::jmax(0, (knobSheet.getWidth() - (numColumns * frameWidth)) / (numColumns + 1));
        const int stepX = frameWidth + padding;
        const int stepY = frameHeight + padding;

        const int frameIndex = selectFilmstripFrameIndex(mappedVisualSliderPos, numFrames);
        const int col = frameIndex % numColumns;
        const int row = frameIndex / numColumns;

        const int sx = padding + col * stepX;
        const int sy = padding + row * stepY;

        const juce::Rectangle<int> srcRect(sx, sy, frameWidth, frameHeight);
        const juce::Rectangle<int> imgBounds(0, 0, knobSheet.getWidth(), knobSheet.getHeight());
        const juce::Rectangle<int> clip = srcRect.getIntersection(imgBounds);
        if (!clip.isEmpty())
        {
            const juce::Image frame = knobSheet.getClippedImage(clip);
            if (frame.isValid())
            {
                g.saveState();
                g.setOpacity(1.0f);
                // Purple knob shadow extends below frame; add bottom overflow so it isn't clipped
                constexpr int shadowOverflowBottom = 12;
                const int drawHeight = height + shadowOverflowBottom;
                g.drawImageWithin(frame, x, y, width, drawHeight,
                                 juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                                  false);
                g.restoreState();
                return;
            }
        }
    }

    // Black theme filmstrip knob (shared sheet for all main knobs)
    if (currentColorIndex == 4 && !isMixKnob && knobSpriteSheetRateImage.isValid())
    {
        const juce::Image& knobSheet = knobSpriteSheetRateImage;
        const int numColumns = 13;
        const int numRows = 12;
        const int numFrames = numColumns * numRows; // 156
        const int frameWidth = 512;
        const int frameHeight = 512;
        const int padding = 12;
        const int stepX = frameWidth + padding;
        const int stepY = frameHeight + padding;

        const int frameIndex = selectFilmstripFrameIndex(mappedVisualSliderPos, numFrames);
        const int col = frameIndex % numColumns;
        const int row = frameIndex / numColumns;

        const int sx = padding + col * stepX;
        const int sy = padding + row * stepY;

        const juce::Rectangle<int> srcRect(sx, sy, frameWidth, frameHeight);
        const juce::Rectangle<int> imgBounds(0, 0, knobSheet.getWidth(), knobSheet.getHeight());
        const juce::Rectangle<int> clip = srcRect.getIntersection(imgBounds);
        if (!clip.isEmpty())
        {
            const juce::Image frame = knobSheet.getClippedImage(clip);
            if (frame.isValid())
            {
                g.saveState();
                g.setOpacity(1.0f);
                g.drawImage(frame, x, y, width, height, 0, 0, frame.getWidth(), frame.getHeight());
                g.restoreState();
                return;
            }
        }
    }

    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float angle = rotaryStartAngle + mappedVisualSliderPos * (rotaryEndAngle - rotaryStartAngle);

    if (isMixKnob && mixKnobSpriteSheetImage.isValid())
    {
        const int numColumns = 13;
        const int numFrames = 156;
        const int frameWidth = 512;
        const int frameHeight = 512;
        const int padding = 64;
        const int stepX = frameWidth + padding;
        const int stepY = frameHeight + padding;

        const int frameIndex = selectFilmstripFrameIndex(mappedVisualSliderPos, numFrames);
        const int col = frameIndex % numColumns;
        const int row = frameIndex / numColumns;

        // Sheets include an outer padding border, so frame origin starts at +padding.
        const int sx = padding + col * stepX;
        const int sy = padding + row * stepY;

        const juce::Rectangle<int> srcRect(sx, sy, frameWidth, frameHeight);
        const juce::Rectangle<int> imgBounds(0, 0, mixKnobSpriteSheetImage.getWidth(), mixKnobSpriteSheetImage.getHeight());
        const juce::Rectangle<int> clip = srcRect.getIntersection(imgBounds);
        if (clip.isEmpty())
            return;
        const juce::Image frame = mixKnobSpriteSheetImage.getClippedImage(clip);
        if (!frame.isValid())
            return;

        g.saveState();
        g.setOpacity(1.0f);
        g.drawImage(frame, x, y, width, height, 0, 0, frame.getWidth(), frame.getHeight());
        g.restoreState();
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
    if (style == juce::Slider::LinearHorizontal)
    {
        // Track is built into the back panel; only draw the thumb.
        
        // Calculate visual slider position (with smoothing if applicable)
        // JUCE passes sliderPos as normalized 0..1 for LinearHorizontal
        float visualSliderPos;
        if (auto* smoothedSlider = dynamic_cast<SmoothedSlider*>(&slider))
        {
            const double minValue = slider.getMinimum();
            const double maxValue = slider.getMaximum();
            const double visualValue = smoothedSlider->getVisualValue();
            const float normalized = static_cast<float>((visualValue - minValue) / (maxValue - minValue));
            const float clampedNormalized = juce::jlimit(0.0f, 1.0f, normalized);
            visualSliderPos = x + clampedNormalized * static_cast<float>(width);
        }
        else
        {
            // sliderPos is 0..1 normalized
            const float clamped = juce::jlimit(0.0f, 1.0f, sliderPos);
            visualSliderPos = x + clamped * static_cast<float>(width);
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
    const float constraintOffset = width * 0.115f;
    
    const float juceMinPos = x;
    const float juceMaxPos = x + width;
    const float actualMinPos = x + constraintOffset;
    const float actualMaxPos = x + width - constraintOffset;
    
    float constrainedSliderPos;
    if (juceMaxPos > juceMinPos)
    {
        const float normalized = (visualSliderPos - juceMinPos) / (juceMaxPos - juceMinPos);
        constrainedSliderPos = actualMinPos + normalized * (actualMaxPos - actualMinPos);
    }
    else
    {
        constrainedSliderPos = actualMinPos;
    }
    
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
