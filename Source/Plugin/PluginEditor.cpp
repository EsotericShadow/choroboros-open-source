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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Config/DefaultsPersistence.h"
#include "../Assets/AssetRepository.h"
#include "../UI/LabelWithContainer.h"
#include "../UI/DevPanelSupport.h"
#include "BinaryData.h"
#include "FeedbackDialog.h"
#include "AboutDialog.h"
#include "HelpDialog.h"
#include "ConfirmDialog.h"
#include "MessageDialog.h"
#include "TextEntryDialog.h"
#include "WindowsRenderPolicy.h"
#include "../KZN/ChoroborosKznImporter.h"
#include "../UI/PluginEditorSetup.h"
#include "../UI/DevPanel.h"
#include <array>
#include <chrono>
#include <cmath>
#include <future>
#include <limits>
#include <vector>
#if CHOROBOROS_PERFETTO_ENABLED
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif

namespace
{
juce::Image toSoftwareImage(const juce::Image& image)
{
    if (! image.isValid())
        return {};

    juce::SoftwareImageType softwareType;
    return softwareType.convert(image);
}

juce::Image loadSoftwareImageFromMemory(const void* data, int dataSize)
{
    return toSoftwareImage(juce::ImageCache::getFromMemory(data, dataSize));
}


struct SharedBackgroundCache
{
    juce::CriticalSection lock;
    std::array<BackgroundAssetPack, 5> packs {};
    std::array<bool, 5> valid { false, false, false, false, false };
};

SharedBackgroundCache& getSharedBackgroundCache()
{
    static SharedBackgroundCache cache;
    return cache;
}

int uiScaleInt(int value)
{
    return juce::roundToInt(static_cast<float>(value) * ChoroborosPluginEditor::kBaseUiScale);
}

int getIntOrDefault(const juce::var& objectVar, const juce::Identifier& key, int fallback)
{
    if (const auto* object = objectVar.getDynamicObject())
    {
        const auto value = object->getProperty(key);
        if (value.isInt() || value.isInt64())
            return static_cast<int>(value);
        if (value.isDouble())
            return static_cast<int>(std::lround(static_cast<double>(value)));
    }
    return fallback;
}

bool loadLayoutDefaultsFromJson(LayoutTuning& layout, const juce::String& json)
{
    if (json.isEmpty())
        return false;

    const auto parsed = juce::JSON::parse(json);
    if (parsed.isVoid())
        return false;

    const auto* root = parsed.getDynamicObject();
    if (root == nullptr || !root->hasProperty("layout"))
        return false;

    const juce::var layoutVar = root->getProperty("layout");
    layout.mainKnobSize = getIntOrDefault(layoutVar, "mainKnobSize", layout.mainKnobSize);
    layout.mainKnobSizeGreen = getIntOrDefault(layoutVar, "mainKnobSizeGreen", layout.mainKnobSize);
    layout.mainKnobSizeBlue = getIntOrDefault(layoutVar, "mainKnobSizeBlue", layout.mainKnobSize);
    layout.mainKnobSizeRed = getIntOrDefault(layoutVar, "mainKnobSizeRed", layout.mainKnobSize);
    layout.mainKnobSizePurple = getIntOrDefault(layoutVar, "mainKnobSizePurple", layout.mainKnobSize);
    layout.mainKnobSizeBlack = getIntOrDefault(layoutVar, "mainKnobSizeBlack", layout.mainKnobSize);
    layout.knobTopY = getIntOrDefault(layoutVar, "knobTopY", layout.knobTopY);
    layout.knobTopYGreen = getIntOrDefault(layoutVar, "knobTopYGreen", layout.knobTopY);
    layout.knobTopYBlue = getIntOrDefault(layoutVar, "knobTopYBlue", layout.knobTopY);
    layout.knobTopYRed = getIntOrDefault(layoutVar, "knobTopYRed", layout.knobTopY);
    layout.knobTopYPurple = getIntOrDefault(layoutVar, "knobTopYPurple", layout.knobTopY);
    layout.knobTopYBlack = getIntOrDefault(layoutVar, "knobTopYBlack", layout.knobTopY);
    layout.rateCenterX = getIntOrDefault(layoutVar, "rateCenterX", layout.rateCenterX);
    layout.rateCenterXGreen = getIntOrDefault(layoutVar, "rateCenterXGreen", layout.rateCenterX);
    layout.rateCenterXBlue = getIntOrDefault(layoutVar, "rateCenterXBlue", layout.rateCenterX);
    layout.rateCenterXRed = getIntOrDefault(layoutVar, "rateCenterXRed", layout.rateCenterX);
    layout.rateCenterXPurple = getIntOrDefault(layoutVar, "rateCenterXPurple", layout.rateCenterX);
    layout.rateCenterXBlack = getIntOrDefault(layoutVar, "rateCenterXBlack", layout.rateCenterX);
    layout.depthCenterX = getIntOrDefault(layoutVar, "depthCenterX", layout.depthCenterX);
    layout.depthCenterXGreen = getIntOrDefault(layoutVar, "depthCenterXGreen", layout.depthCenterX);
    layout.depthCenterXBlue = getIntOrDefault(layoutVar, "depthCenterXBlue", layout.depthCenterX);
    layout.depthCenterXRed = getIntOrDefault(layoutVar, "depthCenterXRed", layout.depthCenterX);
    layout.depthCenterXPurple = getIntOrDefault(layoutVar, "depthCenterXPurple", layout.depthCenterX);
    layout.depthCenterXBlack = getIntOrDefault(layoutVar, "depthCenterXBlack", layout.depthCenterX);
    layout.offsetCenterX = getIntOrDefault(layoutVar, "offsetCenterX", layout.offsetCenterX);
    layout.offsetCenterXGreen = getIntOrDefault(layoutVar, "offsetCenterXGreen", layout.offsetCenterX);
    layout.offsetCenterXBlue = getIntOrDefault(layoutVar, "offsetCenterXBlue", layout.offsetCenterX);
    layout.offsetCenterXRed = getIntOrDefault(layoutVar, "offsetCenterXRed", layout.offsetCenterX);
    layout.offsetCenterXPurple = getIntOrDefault(layoutVar, "offsetCenterXPurple", layout.offsetCenterX);
    layout.offsetCenterXBlack = getIntOrDefault(layoutVar, "offsetCenterXBlack", layout.offsetCenterX);
    layout.widthCenterX = getIntOrDefault(layoutVar, "widthCenterX", layout.widthCenterX);
    layout.widthCenterXGreen = getIntOrDefault(layoutVar, "widthCenterXGreen", layout.widthCenterX);
    layout.widthCenterXBlue = getIntOrDefault(layoutVar, "widthCenterXBlue", layout.widthCenterX);
    layout.widthCenterXRed = getIntOrDefault(layoutVar, "widthCenterXRed", layout.widthCenterX);
    layout.widthCenterXPurple = getIntOrDefault(layoutVar, "widthCenterXPurple", layout.widthCenterX);
    layout.widthCenterXBlack = getIntOrDefault(layoutVar, "widthCenterXBlack", layout.widthCenterX);
    layout.sliderTrackStartX = getIntOrDefault(layoutVar, "sliderTrackStartX", layout.sliderTrackStartX);
    layout.sliderTrackStartY = getIntOrDefault(layoutVar, "sliderTrackStartY", layout.sliderTrackStartY);
    layout.sliderTrackEndX = getIntOrDefault(layoutVar, "sliderTrackEndX", layout.sliderTrackEndX);
    layout.sliderTrackEndY = getIntOrDefault(layoutVar, "sliderTrackEndY", layout.sliderTrackEndY);
    layout.sliderSize = getIntOrDefault(layoutVar, "sliderSize", layout.sliderSize);
    layout.sliderTrackStartXGreen = getIntOrDefault(layoutVar, "sliderTrackStartXGreen", layout.sliderTrackStartX);
    layout.sliderTrackStartYGreen = getIntOrDefault(layoutVar, "sliderTrackStartYGreen", layout.sliderTrackStartY);
    layout.sliderTrackEndXGreen = getIntOrDefault(layoutVar, "sliderTrackEndXGreen", layout.sliderTrackEndX);
    layout.sliderTrackEndYGreen = getIntOrDefault(layoutVar, "sliderTrackEndYGreen", layout.sliderTrackEndY);
    layout.sliderSizeGreen = getIntOrDefault(layoutVar, "sliderSizeGreen", layout.sliderSize);
    layout.sliderTrackStartXBlue = getIntOrDefault(layoutVar, "sliderTrackStartXBlue", layout.sliderTrackStartX);
    layout.sliderTrackStartYBlue = getIntOrDefault(layoutVar, "sliderTrackStartYBlue", layout.sliderTrackStartY);
    layout.sliderTrackEndXBlue = getIntOrDefault(layoutVar, "sliderTrackEndXBlue", layout.sliderTrackEndX);
    layout.sliderTrackEndYBlue = getIntOrDefault(layoutVar, "sliderTrackEndYBlue", layout.sliderTrackEndY);
    layout.sliderSizeBlue = getIntOrDefault(layoutVar, "sliderSizeBlue", layout.sliderSize);
    layout.sliderTrackStartXRed = getIntOrDefault(layoutVar, "sliderTrackStartXRed", layout.sliderTrackStartX);
    layout.sliderTrackStartYRed = getIntOrDefault(layoutVar, "sliderTrackStartYRed", layout.sliderTrackStartY);
    layout.sliderTrackEndXRed = getIntOrDefault(layoutVar, "sliderTrackEndXRed", layout.sliderTrackEndX);
    layout.sliderTrackEndYRed = getIntOrDefault(layoutVar, "sliderTrackEndYRed", layout.sliderTrackEndY);
    layout.sliderSizeRed = getIntOrDefault(layoutVar, "sliderSizeRed", layout.sliderSize);
    layout.sliderTrackStartXPurple = getIntOrDefault(layoutVar, "sliderTrackStartXPurple", layout.sliderTrackStartX);
    layout.sliderTrackStartYPurple = getIntOrDefault(layoutVar, "sliderTrackStartYPurple", layout.sliderTrackStartY);
    layout.sliderTrackEndXPurple = getIntOrDefault(layoutVar, "sliderTrackEndXPurple", layout.sliderTrackEndX);
    layout.sliderTrackEndYPurple = getIntOrDefault(layoutVar, "sliderTrackEndYPurple", layout.sliderTrackEndY);
    layout.sliderSizePurple = getIntOrDefault(layoutVar, "sliderSizePurple", layout.sliderSize);
    layout.sliderTrackStartXBlack = getIntOrDefault(layoutVar, "sliderTrackStartXBlack", layout.sliderTrackStartX);
    layout.sliderTrackStartYBlack = getIntOrDefault(layoutVar, "sliderTrackStartYBlack", layout.sliderTrackStartY);
    layout.sliderTrackEndXBlack = getIntOrDefault(layoutVar, "sliderTrackEndXBlack", layout.sliderTrackEndX);
    layout.sliderTrackEndYBlack = getIntOrDefault(layoutVar, "sliderTrackEndYBlack", layout.sliderTrackEndY);
    layout.sliderSizeBlack = getIntOrDefault(layoutVar, "sliderSizeBlack", layout.sliderSize);
    // Backwards compat: migrate old sliderX/sliderY/sliderCenterX/sliderCenterY/sliderW/sliderH
    if (const auto* obj = layoutVar.getDynamicObject())
    {
        if (obj->hasProperty("sliderTrackStartX") == false)
        {
            if (obj->hasProperty("sliderCenterX"))
            {
                const int centerX = getIntOrDefault(layoutVar, "sliderCenterX", 360);
                const int centerY = getIntOrDefault(layoutVar, "sliderCenterY", 268);
                const int oldW = getIntOrDefault(layoutVar, "sliderW", 250);
                layout.sliderTrackStartX = centerX - (oldW / 2);
                layout.sliderTrackStartY = centerY;
                layout.sliderTrackEndX = centerX + (oldW / 2);
                layout.sliderTrackEndY = centerY;
            }
            else if (obj->hasProperty("sliderX"))
            {
                const int oldX = getIntOrDefault(layoutVar, "sliderX", 235);
                const int oldY = getIntOrDefault(layoutVar, "sliderY", 259);
                const int oldW = getIntOrDefault(layoutVar, "sliderW", 250);
                layout.sliderTrackStartX = oldX;
                layout.sliderTrackStartY = oldY + 9;  // track Y = center of old bounds
                layout.sliderTrackEndX = oldX + oldW;
                layout.sliderTrackEndY = oldY + 9;
            }
        }
        if (obj->hasProperty("sliderW") || obj->hasProperty("sliderH"))
        {
            const int oldW = getIntOrDefault(layoutVar, "sliderW", 250);
            const int oldH = getIntOrDefault(layoutVar, "sliderH", 18);
            layout.sliderSize = juce::jlimit(10, 500, juce::jmax(
                juce::roundToInt(100.0f * oldW / 250.0f),
                juce::roundToInt(100.0f * oldH / 18.0f)));
        }
    }
    layout.mixKnobSize = getIntOrDefault(layoutVar, "mixKnobSize", layout.mixKnobSize);
    layout.mixKnobSizeGreen = getIntOrDefault(layoutVar, "mixKnobSizeGreen", layout.mixKnobSize);
    layout.mixKnobSizeBlue = getIntOrDefault(layoutVar, "mixKnobSizeBlue", layout.mixKnobSize);
    layout.mixKnobSizeRed = getIntOrDefault(layoutVar, "mixKnobSizeRed", layout.mixKnobSize);
    layout.mixKnobSizePurple = getIntOrDefault(layoutVar, "mixKnobSizePurple", layout.mixKnobSize);
    layout.mixKnobSizeBlack = getIntOrDefault(layoutVar, "mixKnobSizeBlack", layout.mixKnobSize);
    layout.mixCenterX = getIntOrDefault(layoutVar, "mixCenterX", layout.mixCenterX);
    layout.mixCenterXGreen = getIntOrDefault(layoutVar, "mixCenterXGreen", layout.mixCenterX);
    layout.mixCenterXBlue = getIntOrDefault(layoutVar, "mixCenterXBlue", layout.mixCenterX);
    layout.mixCenterXRed = getIntOrDefault(layoutVar, "mixCenterXRed", layout.mixCenterX);
    layout.mixCenterXPurple = getIntOrDefault(layoutVar, "mixCenterXPurple", layout.mixCenterX);
    layout.mixCenterXBlack = getIntOrDefault(layoutVar, "mixCenterXBlack", layout.mixCenterX);
    layout.mixKnobY = getIntOrDefault(layoutVar, "mixKnobY", layout.mixKnobY);
    layout.mixKnobYGreen = getIntOrDefault(layoutVar, "mixKnobYGreen", layout.mixKnobY);
    layout.mixKnobYBlue = getIntOrDefault(layoutVar, "mixKnobYBlue", layout.mixKnobY);
    layout.mixKnobYRed = getIntOrDefault(layoutVar, "mixKnobYRed", layout.mixKnobY);
    layout.mixKnobYPurple = getIntOrDefault(layoutVar, "mixKnobYPurple", layout.mixKnobY);
    layout.mixKnobYBlack = getIntOrDefault(layoutVar, "mixKnobYBlack", layout.mixKnobY);
    layout.mixKnobYOffset = getIntOrDefault(layoutVar, "mixKnobYOffset", layout.mixKnobYOffset);
    layout.mixKnobYOffsetGreen = getIntOrDefault(layoutVar, "mixKnobYOffsetGreen", layout.mixKnobYOffset);
    layout.mixKnobYOffsetBlue = getIntOrDefault(layoutVar, "mixKnobYOffsetBlue", layout.mixKnobYOffset);
    layout.mixKnobYOffsetRed = getIntOrDefault(layoutVar, "mixKnobYOffsetRed", layout.mixKnobYOffset);
    layout.mixKnobYOffsetPurple = getIntOrDefault(layoutVar, "mixKnobYOffsetPurple", layout.mixKnobYOffset);
    layout.mixKnobYOffsetBlack = getIntOrDefault(layoutVar, "mixKnobYOffsetBlack", layout.mixKnobYOffset);
    layout.valueLabelWidth = getIntOrDefault(layoutVar, "valueLabelWidth", layout.valueLabelWidth);
    layout.valueLabelHeight = getIntOrDefault(layoutVar, "valueLabelHeight", layout.valueLabelHeight);
    layout.valueLabelY = getIntOrDefault(layoutVar, "valueLabelY", layout.valueLabelY);
    layout.valueLabelYGreen = getIntOrDefault(layoutVar, "valueLabelYGreen", layout.valueLabelY);
    layout.valueLabelYBlue = getIntOrDefault(layoutVar, "valueLabelYBlue", layout.valueLabelY);
    layout.valueLabelYRed = getIntOrDefault(layoutVar, "valueLabelYRed", layout.valueLabelY);
    layout.valueLabelYPurple = getIntOrDefault(layoutVar, "valueLabelYPurple", layout.valueLabelY);
    layout.valueLabelYBlack = getIntOrDefault(layoutVar, "valueLabelYBlack", layout.valueLabelY);
    layout.rateValueOffsetX = getIntOrDefault(layoutVar, "rateValueOffsetX", layout.rateValueOffsetX);
    layout.rateValueOffsetXGreen = getIntOrDefault(layoutVar, "rateValueOffsetXGreen", layout.rateValueOffsetX);
    layout.rateValueOffsetXBlue = getIntOrDefault(layoutVar, "rateValueOffsetXBlue", layout.rateValueOffsetX);
    layout.rateValueOffsetXRed = getIntOrDefault(layoutVar, "rateValueOffsetXRed", layout.rateValueOffsetX);
    layout.rateValueOffsetXPurple = getIntOrDefault(layoutVar, "rateValueOffsetXPurple", layout.rateValueOffsetX);
    layout.rateValueOffsetXBlack = getIntOrDefault(layoutVar, "rateValueOffsetXBlack", layout.rateValueOffsetX);
    layout.depthValueOffsetX = getIntOrDefault(layoutVar, "depthValueOffsetX", layout.depthValueOffsetX);
    layout.depthValueOffsetXGreen = getIntOrDefault(layoutVar, "depthValueOffsetXGreen", layout.depthValueOffsetX);
    layout.depthValueOffsetXBlue = getIntOrDefault(layoutVar, "depthValueOffsetXBlue", layout.depthValueOffsetX);
    layout.depthValueOffsetXRed = getIntOrDefault(layoutVar, "depthValueOffsetXRed", layout.depthValueOffsetX);
    layout.depthValueOffsetXPurple = getIntOrDefault(layoutVar, "depthValueOffsetXPurple", layout.depthValueOffsetX);
    layout.depthValueOffsetXBlack = getIntOrDefault(layoutVar, "depthValueOffsetXBlack", layout.depthValueOffsetX);
    layout.offsetValueOffsetX = getIntOrDefault(layoutVar, "offsetValueOffsetX", layout.offsetValueOffsetX);
    layout.offsetValueOffsetXGreen = getIntOrDefault(layoutVar, "offsetValueOffsetXGreen", layout.offsetValueOffsetX);
    layout.offsetValueOffsetXBlue = getIntOrDefault(layoutVar, "offsetValueOffsetXBlue", layout.offsetValueOffsetX);
    layout.offsetValueOffsetXRed = getIntOrDefault(layoutVar, "offsetValueOffsetXRed", layout.offsetValueOffsetX);
    layout.offsetValueOffsetXPurple = getIntOrDefault(layoutVar, "offsetValueOffsetXPurple", layout.offsetValueOffsetX);
    layout.offsetValueOffsetXBlack = getIntOrDefault(layoutVar, "offsetValueOffsetXBlack", layout.offsetValueOffsetX);
    layout.widthValueOffsetX = getIntOrDefault(layoutVar, "widthValueOffsetX", layout.widthValueOffsetX);
    layout.widthValueOffsetXGreen = getIntOrDefault(layoutVar, "widthValueOffsetXGreen", layout.widthValueOffsetX);
    layout.widthValueOffsetXBlue = getIntOrDefault(layoutVar, "widthValueOffsetXBlue", layout.widthValueOffsetX);
    layout.widthValueOffsetXRed = getIntOrDefault(layoutVar, "widthValueOffsetXRed", layout.widthValueOffsetX);
    layout.widthValueOffsetXPurple = getIntOrDefault(layoutVar, "widthValueOffsetXPurple", layout.widthValueOffsetX);
    layout.widthValueOffsetXBlack = getIntOrDefault(layoutVar, "widthValueOffsetXBlack", layout.widthValueOffsetX);
    layout.colorValueY = getIntOrDefault(layoutVar, "colorValueY", layout.colorValueY);
    layout.colorValueYGreen = getIntOrDefault(layoutVar, "colorValueYGreen", layout.colorValueY);
    layout.colorValueYBlue = getIntOrDefault(layoutVar, "colorValueYBlue", layout.colorValueY);
    layout.colorValueYRed = getIntOrDefault(layoutVar, "colorValueYRed", layout.colorValueY);
    layout.colorValueYPurple = getIntOrDefault(layoutVar, "colorValueYPurple", layout.colorValueY);
    layout.colorValueYBlack = getIntOrDefault(layoutVar, "colorValueYBlack", layout.colorValueY);
    layout.colorValueCenterX = getIntOrDefault(layoutVar, "colorValueCenterX", layout.colorValueCenterX);
    layout.colorValueWidth = getIntOrDefault(layoutVar, "colorValueWidth", layout.colorValueWidth);
    layout.colorValueHeight = getIntOrDefault(layoutVar, "colorValueHeight", layout.colorValueHeight);
    layout.colorValueXOffset = getIntOrDefault(layoutVar, "colorValueXOffset", layout.colorValueXOffset);
    layout.colorValueXOffsetGreen = getIntOrDefault(layoutVar, "colorValueXOffsetGreen", layout.colorValueXOffset);
    layout.colorValueXOffsetBlue = getIntOrDefault(layoutVar, "colorValueXOffsetBlue", layout.colorValueXOffset);
    layout.colorValueXOffsetRed = getIntOrDefault(layoutVar, "colorValueXOffsetRed", layout.colorValueXOffset);
    layout.colorValueXOffsetPurple = getIntOrDefault(layoutVar, "colorValueXOffsetPurple", layout.colorValueXOffset);
    layout.colorValueXOffsetBlack = getIntOrDefault(layoutVar, "colorValueXOffsetBlack", layout.colorValueXOffset);
    layout.mixValueY = getIntOrDefault(layoutVar, "mixValueY", layout.mixValueY);
    layout.mixValueYGreen = getIntOrDefault(layoutVar, "mixValueYGreen", layout.mixValueY);
    layout.mixValueYBlue = getIntOrDefault(layoutVar, "mixValueYBlue", layout.mixValueY);
    layout.mixValueYRed = getIntOrDefault(layoutVar, "mixValueYRed", layout.mixValueY);
    layout.mixValueYPurple = getIntOrDefault(layoutVar, "mixValueYPurple", layout.mixValueY);
    layout.mixValueYBlack = getIntOrDefault(layoutVar, "mixValueYBlack", layout.mixValueY);
    layout.mixValueWidth = getIntOrDefault(layoutVar, "mixValueWidth", layout.mixValueWidth);
    layout.mixValueHeight = getIntOrDefault(layoutVar, "mixValueHeight", layout.mixValueHeight);
    layout.mixValueOffsetX = getIntOrDefault(layoutVar, "mixValueOffsetX", layout.mixValueOffsetX);
    layout.mixValueOffsetXGreen = getIntOrDefault(layoutVar, "mixValueOffsetXGreen", layout.mixValueOffsetX);
    layout.mixValueOffsetXBlue = getIntOrDefault(layoutVar, "mixValueOffsetXBlue", layout.mixValueOffsetX);
    layout.mixValueOffsetXRed = getIntOrDefault(layoutVar, "mixValueOffsetXRed", layout.mixValueOffsetX);
    layout.mixValueOffsetXPurple = getIntOrDefault(layoutVar, "mixValueOffsetXPurple", layout.mixValueOffsetX);
    layout.mixValueOffsetXBlack = getIntOrDefault(layoutVar, "mixValueOffsetXBlack", layout.mixValueOffsetX);
    layout.rateValueOffsetY = getIntOrDefault(layoutVar, "rateValueOffsetY", layout.rateValueOffsetY);
    layout.rateValueOffsetYGreen = getIntOrDefault(layoutVar, "rateValueOffsetYGreen", layout.rateValueOffsetY);
    layout.rateValueOffsetYBlue = getIntOrDefault(layoutVar, "rateValueOffsetYBlue", layout.rateValueOffsetY);
    layout.rateValueOffsetYRed = getIntOrDefault(layoutVar, "rateValueOffsetYRed", layout.rateValueOffsetY);
    layout.rateValueOffsetYPurple = getIntOrDefault(layoutVar, "rateValueOffsetYPurple", layout.rateValueOffsetY);
    layout.rateValueOffsetYBlack = getIntOrDefault(layoutVar, "rateValueOffsetYBlack", layout.rateValueOffsetY);
    layout.depthValueOffsetY = getIntOrDefault(layoutVar, "depthValueOffsetY", layout.depthValueOffsetY);
    layout.depthValueOffsetYGreen = getIntOrDefault(layoutVar, "depthValueOffsetYGreen", layout.depthValueOffsetY);
    layout.depthValueOffsetYBlue = getIntOrDefault(layoutVar, "depthValueOffsetYBlue", layout.depthValueOffsetY);
    layout.depthValueOffsetYRed = getIntOrDefault(layoutVar, "depthValueOffsetYRed", layout.depthValueOffsetY);
    layout.depthValueOffsetYPurple = getIntOrDefault(layoutVar, "depthValueOffsetYPurple", layout.depthValueOffsetY);
    layout.depthValueOffsetYBlack = getIntOrDefault(layoutVar, "depthValueOffsetYBlack", layout.depthValueOffsetY);
    layout.offsetValueOffsetY = getIntOrDefault(layoutVar, "offsetValueOffsetY", layout.offsetValueOffsetY);
    layout.offsetValueOffsetYGreen = getIntOrDefault(layoutVar, "offsetValueOffsetYGreen", layout.offsetValueOffsetY);
    layout.offsetValueOffsetYBlue = getIntOrDefault(layoutVar, "offsetValueOffsetYBlue", layout.offsetValueOffsetY);
    layout.offsetValueOffsetYRed = getIntOrDefault(layoutVar, "offsetValueOffsetYRed", layout.offsetValueOffsetY);
    layout.offsetValueOffsetYPurple = getIntOrDefault(layoutVar, "offsetValueOffsetYPurple", layout.offsetValueOffsetY);
    layout.offsetValueOffsetYBlack = getIntOrDefault(layoutVar, "offsetValueOffsetYBlack", layout.offsetValueOffsetY);
    layout.widthValueOffsetY = getIntOrDefault(layoutVar, "widthValueOffsetY", layout.widthValueOffsetY);
    layout.widthValueOffsetYGreen = getIntOrDefault(layoutVar, "widthValueOffsetYGreen", layout.widthValueOffsetY);
    layout.widthValueOffsetYBlue = getIntOrDefault(layoutVar, "widthValueOffsetYBlue", layout.widthValueOffsetY);
    layout.widthValueOffsetYRed = getIntOrDefault(layoutVar, "widthValueOffsetYRed", layout.widthValueOffsetY);
    layout.widthValueOffsetYPurple = getIntOrDefault(layoutVar, "widthValueOffsetYPurple", layout.widthValueOffsetY);
    layout.widthValueOffsetYBlack = getIntOrDefault(layoutVar, "widthValueOffsetYBlack", layout.widthValueOffsetY);
    layout.knobValueFontSize = getIntOrDefault(layoutVar, "knobValueFontSize", layout.knobValueFontSize);
    layout.colorValueFontSize = getIntOrDefault(layoutVar, "colorValueFontSize", layout.colorValueFontSize);
    layout.mixValueFontSize = getIntOrDefault(layoutVar, "mixValueFontSize", layout.mixValueFontSize);
    layout.valueTextAlphaPct = getIntOrDefault(layoutVar, "valueTextAlphaPct", layout.valueTextAlphaPct);
    layout.valueTextColourMode = getIntOrDefault(layoutVar, "valueTextColourMode", layout.valueTextColourMode);
    layout.valueTextColour = getIntOrDefault(layoutVar, "valueTextColour", layout.valueTextColour);
    layout.topButtonsWidth = getIntOrDefault(layoutVar, "topButtonsWidth", layout.topButtonsWidth);
    layout.topButtonsHeight = getIntOrDefault(layoutVar, "topButtonsHeight", layout.topButtonsHeight);
    layout.topButtonsGap = getIntOrDefault(layoutVar, "topButtonsGap", layout.topButtonsGap);
    layout.topButtonsRightMargin = getIntOrDefault(layoutVar, "topButtonsRightMargin", layout.topButtonsRightMargin);
    layout.topButtonsTopY = getIntOrDefault(layoutVar, "topButtonsTopY", layout.topButtonsTopY);
    layout.topButtonsFontSize = getIntOrDefault(layoutVar, "topButtonsFontSize", layout.topButtonsFontSize);
    layout.topButtonsTextColour = getIntOrDefault(layoutVar, "topButtonsTextColour", layout.topButtonsTextColour);
    layout.topButtonsBackgroundColour = getIntOrDefault(layoutVar, "topButtonsBackgroundColour", layout.topButtonsBackgroundColour);
    layout.topButtonsOnBackgroundColour = getIntOrDefault(layoutVar, "topButtonsOnBackgroundColour", layout.topButtonsOnBackgroundColour);
    layout.engineSelectorX = getIntOrDefault(layoutVar, "engineSelectorX", layout.engineSelectorX);
    layout.engineSelectorY = getIntOrDefault(layoutVar, "engineSelectorY", layout.engineSelectorY);
    layout.engineSelectorW = getIntOrDefault(layoutVar, "engineSelectorW", layout.engineSelectorW);
    layout.engineSelectorH = getIntOrDefault(layoutVar, "engineSelectorH", layout.engineSelectorH);
    layout.engineSelectorFontSize = getIntOrDefault(layoutVar, "engineSelectorFontSize", layout.engineSelectorFontSize);
    layout.engineSelectorTextColour = getIntOrDefault(layoutVar, "engineSelectorTextColour", layout.engineSelectorTextColour);
    layout.engineSelectorBackgroundColour = getIntOrDefault(layoutVar, "engineSelectorBackgroundColour", layout.engineSelectorBackgroundColour);
    layout.engineSelectorOutlineColour = getIntOrDefault(layoutVar, "engineSelectorOutlineColour", layout.engineSelectorOutlineColour);
    layout.engineSelectorArrowColour = getIntOrDefault(layoutVar, "engineSelectorArrowColour", layout.engineSelectorArrowColour);
    layout.engineSelectorPopupBackgroundColour = getIntOrDefault(layoutVar, "engineSelectorPopupBackgroundColour", layout.engineSelectorPopupBackgroundColour);
    layout.engineSelectorPopupTextColour = getIntOrDefault(layoutVar, "engineSelectorPopupTextColour", layout.engineSelectorPopupTextColour);
    layout.engineSelectorPopupHighlightedBackgroundColour = getIntOrDefault(layoutVar, "engineSelectorPopupHighlightedBackgroundColour", layout.engineSelectorPopupHighlightedBackgroundColour);
    layout.engineSelectorPopupHighlightedTextColour = getIntOrDefault(layoutVar, "engineSelectorPopupHighlightedTextColour", layout.engineSelectorPopupHighlightedTextColour);
    layout.hqSwitchSize = getIntOrDefault(layoutVar, "hqSwitchSize", layout.hqSwitchSize);
    layout.hqSwitchOffsetX = getIntOrDefault(layoutVar, "hqSwitchOffsetX", layout.hqSwitchOffsetX);
    layout.hqSwitchOffsetY = getIntOrDefault(layoutVar, "hqSwitchOffsetY", layout.hqSwitchOffsetY);
    layout.hqSwitchOffsetXGreen = getIntOrDefault(layoutVar, "hqSwitchOffsetXGreen", layout.hqSwitchOffsetX);
    layout.hqSwitchOffsetXBlue = getIntOrDefault(layoutVar, "hqSwitchOffsetXBlue", layout.hqSwitchOffsetX);
    layout.hqSwitchOffsetXRed = getIntOrDefault(layoutVar, "hqSwitchOffsetXRed", layout.hqSwitchOffsetX);
    layout.hqSwitchOffsetXPurple = getIntOrDefault(layoutVar, "hqSwitchOffsetXPurple", layout.hqSwitchOffsetX);
    layout.hqSwitchOffsetXBlack = getIntOrDefault(layoutVar, "hqSwitchOffsetXBlack", layout.hqSwitchOffsetX);
    layout.hqSwitchOffsetYGreen = getIntOrDefault(layoutVar, "hqSwitchOffsetYGreen", layout.hqSwitchOffsetY);
    layout.hqSwitchOffsetYBlue = getIntOrDefault(layoutVar, "hqSwitchOffsetYBlue", layout.hqSwitchOffsetY);
    layout.hqSwitchOffsetYRed = getIntOrDefault(layoutVar, "hqSwitchOffsetYRed", layout.hqSwitchOffsetY);
    layout.hqSwitchOffsetYPurple = getIntOrDefault(layoutVar, "hqSwitchOffsetYPurple", layout.hqSwitchOffsetY);
    layout.hqSwitchOffsetYBlack = getIntOrDefault(layoutVar, "hqSwitchOffsetYBlack", layout.hqSwitchOffsetY);
    layout.rateKnobVisualResponseMs = getIntOrDefault(layoutVar, "rateKnobVisualResponseMs", layout.rateKnobVisualResponseMs);
    layout.depthKnobVisualResponseMs = getIntOrDefault(layoutVar, "depthKnobVisualResponseMs", layout.depthKnobVisualResponseMs);
    layout.offsetKnobVisualResponseMs = getIntOrDefault(layoutVar, "offsetKnobVisualResponseMs", layout.offsetKnobVisualResponseMs);
    layout.widthKnobVisualResponseMs = getIntOrDefault(layoutVar, "widthKnobVisualResponseMs", layout.widthKnobVisualResponseMs);
    layout.mixKnobVisualResponseMs = getIntOrDefault(layoutVar, "mixKnobVisualResponseMs", layout.mixKnobVisualResponseMs);
    layout.knobDragSensitivityPct = getIntOrDefault(layoutVar, "knobDragSensitivityPct", layout.knobDragSensitivityPct);
    layout.scrollWheelSensitivityPct = getIntOrDefault(layoutVar, "scrollWheelSensitivityPct", layout.scrollWheelSensitivityPct);
    layout.knobRollOffSpeedPct = getIntOrDefault(layoutVar, "knobRollOffSpeedPct", layout.knobRollOffSpeedPct);
    layout.knobSweepStartDeg = getIntOrDefault(layoutVar, "knobSweepStartDeg", layout.knobSweepStartDeg);
    layout.knobSweepEndDeg = getIntOrDefault(layoutVar, "knobSweepEndDeg", layout.knobSweepEndDeg);
    layout.knobFrameCount = getIntOrDefault(layoutVar, "knobFrameCount", layout.knobFrameCount);
    const int legacyFlipEnabled = getIntOrDefault(layoutVar, "valueFlipEnabled", layout.mainValueFlipEnabled);
    const int legacyFlipDurationMs = getIntOrDefault(layoutVar, "valueFlipDurationMs", layout.mainValueFlipDurationMs);
    const int legacyFlipTravelPxTimes10 = getIntOrDefault(layoutVar, "valueFlipTravelPxTimes10", layout.mainValueFlipTravelUpPxTimes100 / 10);
    const int legacyFlipShearPct = getIntOrDefault(layoutVar, "valueFlipShearPct", layout.mainValueFlipShearPct);
    const int legacyFlipMinScalePct = getIntOrDefault(layoutVar, "valueFlipMinScalePct", layout.mainValueFlipMinScalePct);

    layout.mainValueFlipEnabled = getIntOrDefault(layoutVar, "mainValueFlipEnabled", legacyFlipEnabled);
    layout.mainValueFlipDurationMs = getIntOrDefault(layoutVar, "mainValueFlipDurationMs", legacyFlipDurationMs);
    const int legacyMainTravelUpTimes100 = getIntOrDefault(layoutVar, "mainValueFlipTravelUpPxTimes10", legacyFlipTravelPxTimes10) * 10;
    const int legacyMainTravelDownTimes100 = getIntOrDefault(layoutVar, "mainValueFlipTravelDownPxTimes10", legacyFlipTravelPxTimes10) * 10;
    layout.mainValueFlipTravelUpPxTimes100 = getIntOrDefault(layoutVar, "mainValueFlipTravelUpPxTimes100", legacyMainTravelUpTimes100);
    layout.mainValueFlipTravelDownPxTimes100 = getIntOrDefault(layoutVar, "mainValueFlipTravelDownPxTimes100", legacyMainTravelDownTimes100);
    layout.mainValueFlipTravelOutPct = getIntOrDefault(layoutVar, "mainValueFlipTravelOutPct", layout.mainValueFlipTravelOutPct);
    layout.mainValueFlipTravelInPct = getIntOrDefault(layoutVar, "mainValueFlipTravelInPct", layout.mainValueFlipTravelInPct);
    layout.mainValueFlipShearPct = getIntOrDefault(layoutVar, "mainValueFlipShearPct", legacyFlipShearPct);
    layout.mainValueFlipMinScalePct = getIntOrDefault(layoutVar, "mainValueFlipMinScalePct", legacyFlipMinScalePct);

    layout.colorValueFlipEnabled = getIntOrDefault(layoutVar, "colorValueFlipEnabled", legacyFlipEnabled);
    layout.colorValueFlipDurationMs = getIntOrDefault(layoutVar, "colorValueFlipDurationMs", legacyFlipDurationMs);
    const int legacyColorTravelUpTimes100 = getIntOrDefault(layoutVar, "colorValueFlipTravelUpPxTimes10", legacyFlipTravelPxTimes10) * 10;
    const int legacyColorTravelDownTimes100 = getIntOrDefault(layoutVar, "colorValueFlipTravelDownPxTimes10", legacyFlipTravelPxTimes10) * 10;
    layout.colorValueFlipTravelUpPxTimes100 = getIntOrDefault(layoutVar, "colorValueFlipTravelUpPxTimes100", legacyColorTravelUpTimes100);
    layout.colorValueFlipTravelDownPxTimes100 = getIntOrDefault(layoutVar, "colorValueFlipTravelDownPxTimes100", legacyColorTravelDownTimes100);
    layout.colorValueFlipTravelOutPct = getIntOrDefault(layoutVar, "colorValueFlipTravelOutPct", layout.colorValueFlipTravelOutPct);
    layout.colorValueFlipTravelInPct = getIntOrDefault(layoutVar, "colorValueFlipTravelInPct", layout.colorValueFlipTravelInPct);
    layout.colorValueFlipShearPct = getIntOrDefault(layoutVar, "colorValueFlipShearPct", legacyFlipShearPct);
    layout.colorValueFlipMinScalePct = getIntOrDefault(layoutVar, "colorValueFlipMinScalePct", legacyFlipMinScalePct);

    layout.mixValueFlipEnabled = getIntOrDefault(layoutVar, "mixValueFlipEnabled", legacyFlipEnabled);
    layout.mixValueFlipDurationMs = getIntOrDefault(layoutVar, "mixValueFlipDurationMs", legacyFlipDurationMs);
    const int legacyMixTravelUpTimes100 = getIntOrDefault(layoutVar, "mixValueFlipTravelUpPxTimes10", legacyFlipTravelPxTimes10) * 10;
    const int legacyMixTravelDownTimes100 = getIntOrDefault(layoutVar, "mixValueFlipTravelDownPxTimes10", legacyFlipTravelPxTimes10) * 10;
    layout.mixValueFlipTravelUpPxTimes100 = getIntOrDefault(layoutVar, "mixValueFlipTravelUpPxTimes100", legacyMixTravelUpTimes100);
    layout.mixValueFlipTravelDownPxTimes100 = getIntOrDefault(layoutVar, "mixValueFlipTravelDownPxTimes100", legacyMixTravelDownTimes100);
    layout.mixValueFlipTravelOutPct = getIntOrDefault(layoutVar, "mixValueFlipTravelOutPct", layout.mixValueFlipTravelOutPct);
    layout.mixValueFlipTravelInPct = getIntOrDefault(layoutVar, "mixValueFlipTravelInPct", layout.mixValueFlipTravelInPct);
    layout.mixValueFlipShearPct = getIntOrDefault(layoutVar, "mixValueFlipShearPct", legacyFlipShearPct);
    layout.mixValueFlipMinScalePct = getIntOrDefault(layoutVar, "mixValueFlipMinScalePct", legacyFlipMinScalePct);
    layout.valueFxEnabled = getIntOrDefault(layoutVar, "valueFxEnabled", layout.valueFxEnabled);
    layout.valueGlowAlphaPct = getIntOrDefault(layoutVar, "valueGlowAlphaPct", layout.valueGlowAlphaPct);
    layout.valueGlowSpreadPxTimes100 = getIntOrDefault(layoutVar, "valueGlowSpreadPxTimes100", layout.valueGlowSpreadPxTimes100);
    layout.valueFxPerCharOffsetXPxTimes100 = getIntOrDefault(layoutVar, "valueFxPerCharOffsetXPxTimes100", layout.valueFxPerCharOffsetXPxTimes100);
    layout.valueFxPerCharOffsetYPxTimes100 = getIntOrDefault(layoutVar, "valueFxPerCharOffsetYPxTimes100", layout.valueFxPerCharOffsetYPxTimes100);
    layout.valueTopReflectAlphaPct = getIntOrDefault(layoutVar, "valueTopReflectAlphaPct", layout.valueTopReflectAlphaPct);
    layout.valueTopReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, "valueTopReflectOffsetXPxTimes100", layout.valueTopReflectOffsetXPxTimes100);
    layout.valueTopReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, "valueTopReflectOffsetYPxTimes100", layout.valueTopReflectOffsetYPxTimes100);
    layout.valueTopReflectShearPct = getIntOrDefault(layoutVar, "valueTopReflectShearPct", layout.valueTopReflectShearPct);
    layout.valueTopReflectRotateDeg = getIntOrDefault(layoutVar, "valueTopReflectRotateDeg", layout.valueTopReflectRotateDeg);
    layout.valueBottomReflectAlphaPct = getIntOrDefault(layoutVar, "valueBottomReflectAlphaPct", layout.valueBottomReflectAlphaPct);
    layout.valueBottomReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, "valueBottomReflectOffsetXPxTimes100", layout.valueBottomReflectOffsetXPxTimes100);
    layout.valueBottomReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, "valueBottomReflectOffsetYPxTimes100", layout.valueBottomReflectOffsetYPxTimes100);
    layout.valueBottomReflectShearPct = getIntOrDefault(layoutVar, "valueBottomReflectShearPct", layout.valueBottomReflectShearPct);
    layout.valueBottomReflectRotateDeg = getIntOrDefault(layoutVar, "valueBottomReflectRotateDeg", layout.valueBottomReflectRotateDeg);
    layout.valueReflectBlurPxTimes100 = getIntOrDefault(layoutVar, "valueReflectBlurPxTimes100", layout.valueReflectBlurPxTimes100);
    layout.valueReflectSquashPct = getIntOrDefault(layoutVar, "valueReflectSquashPct", layout.valueReflectSquashPct);
    layout.valueReflectMotionPct = getIntOrDefault(layoutVar, "valueReflectMotionPct", layout.valueReflectMotionPct);
    layout.colorValueFxEnabled = getIntOrDefault(layoutVar, "colorValueFxEnabled", layout.valueFxEnabled);
    layout.colorValueGlowAlphaPct = getIntOrDefault(layoutVar, "colorValueGlowAlphaPct", layout.valueGlowAlphaPct);
    layout.colorValueGlowSpreadPxTimes100 = getIntOrDefault(layoutVar, "colorValueGlowSpreadPxTimes100", layout.valueGlowSpreadPxTimes100);
    layout.colorValueFxPerCharOffsetXPxTimes100 = getIntOrDefault(layoutVar, "colorValueFxPerCharOffsetXPxTimes100", layout.valueFxPerCharOffsetXPxTimes100);
    layout.colorValueFxPerCharOffsetYPxTimes100 = getIntOrDefault(layoutVar, "colorValueFxPerCharOffsetYPxTimes100", layout.valueFxPerCharOffsetYPxTimes100);
    layout.colorValueTopReflectAlphaPct = getIntOrDefault(layoutVar, "colorValueTopReflectAlphaPct", layout.valueTopReflectAlphaPct);
    layout.colorValueTopReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, "colorValueTopReflectOffsetXPxTimes100", layout.valueTopReflectOffsetXPxTimes100);
    layout.colorValueTopReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, "colorValueTopReflectOffsetYPxTimes100", layout.valueTopReflectOffsetYPxTimes100);
    layout.colorValueTopReflectShearPct = getIntOrDefault(layoutVar, "colorValueTopReflectShearPct", layout.valueTopReflectShearPct);
    layout.colorValueTopReflectRotateDeg = getIntOrDefault(layoutVar, "colorValueTopReflectRotateDeg", layout.valueTopReflectRotateDeg);
    layout.colorValueBottomReflectAlphaPct = getIntOrDefault(layoutVar, "colorValueBottomReflectAlphaPct", layout.valueBottomReflectAlphaPct);
    layout.colorValueBottomReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, "colorValueBottomReflectOffsetXPxTimes100", layout.valueBottomReflectOffsetXPxTimes100);
    layout.colorValueBottomReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, "colorValueBottomReflectOffsetYPxTimes100", layout.valueBottomReflectOffsetYPxTimes100);
    layout.colorValueBottomReflectShearPct = getIntOrDefault(layoutVar, "colorValueBottomReflectShearPct", layout.valueBottomReflectShearPct);
    layout.colorValueBottomReflectRotateDeg = getIntOrDefault(layoutVar, "colorValueBottomReflectRotateDeg", layout.valueBottomReflectRotateDeg);
    layout.colorValueReflectBlurPxTimes100 = getIntOrDefault(layoutVar, "colorValueReflectBlurPxTimes100", layout.valueReflectBlurPxTimes100);
    layout.colorValueReflectSquashPct = getIntOrDefault(layoutVar, "colorValueReflectSquashPct", layout.valueReflectSquashPct);
    layout.colorValueReflectMotionPct = getIntOrDefault(layoutVar, "colorValueReflectMotionPct", layout.valueReflectMotionPct);
    layout.mixValueFxEnabled = getIntOrDefault(layoutVar, "mixValueFxEnabled", layout.valueFxEnabled);
    layout.mixValueGlowAlphaPct = getIntOrDefault(layoutVar, "mixValueGlowAlphaPct", layout.valueGlowAlphaPct);
    layout.mixValueGlowSpreadPxTimes100 = getIntOrDefault(layoutVar, "mixValueGlowSpreadPxTimes100", layout.valueGlowSpreadPxTimes100);
    layout.mixValueFxPerCharOffsetXPxTimes100 = getIntOrDefault(layoutVar, "mixValueFxPerCharOffsetXPxTimes100", layout.valueFxPerCharOffsetXPxTimes100);
    layout.mixValueFxPerCharOffsetYPxTimes100 = getIntOrDefault(layoutVar, "mixValueFxPerCharOffsetYPxTimes100", layout.valueFxPerCharOffsetYPxTimes100);
    layout.mixValueTopReflectAlphaPct = getIntOrDefault(layoutVar, "mixValueTopReflectAlphaPct", layout.valueTopReflectAlphaPct);
    layout.mixValueTopReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, "mixValueTopReflectOffsetXPxTimes100", layout.valueTopReflectOffsetXPxTimes100);
    layout.mixValueTopReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, "mixValueTopReflectOffsetYPxTimes100", layout.valueTopReflectOffsetYPxTimes100);
    layout.mixValueTopReflectShearPct = getIntOrDefault(layoutVar, "mixValueTopReflectShearPct", layout.valueTopReflectShearPct);
    layout.mixValueTopReflectRotateDeg = getIntOrDefault(layoutVar, "mixValueTopReflectRotateDeg", layout.valueTopReflectRotateDeg);
    layout.mixValueBottomReflectAlphaPct = getIntOrDefault(layoutVar, "mixValueBottomReflectAlphaPct", layout.valueBottomReflectAlphaPct);
    layout.mixValueBottomReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, "mixValueBottomReflectOffsetXPxTimes100", layout.valueBottomReflectOffsetXPxTimes100);
    layout.mixValueBottomReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, "mixValueBottomReflectOffsetYPxTimes100", layout.valueBottomReflectOffsetYPxTimes100);
    layout.mixValueBottomReflectShearPct = getIntOrDefault(layoutVar, "mixValueBottomReflectShearPct", layout.valueBottomReflectShearPct);
    layout.mixValueBottomReflectRotateDeg = getIntOrDefault(layoutVar, "mixValueBottomReflectRotateDeg", layout.valueBottomReflectRotateDeg);
    layout.mixValueReflectBlurPxTimes100 = getIntOrDefault(layoutVar, "mixValueReflectBlurPxTimes100", layout.valueReflectBlurPxTimes100);
    layout.mixValueReflectSquashPct = getIntOrDefault(layoutVar, "mixValueReflectSquashPct", layout.valueReflectSquashPct);
    layout.mixValueReflectMotionPct = getIntOrDefault(layoutVar, "mixValueReflectMotionPct", layout.valueReflectMotionPct);

    const std::array<juce::String, LayoutTuning::engineCount> engineSuffixes { { "Green", "Blue", "Red", "Purple", "Black" } };
    const std::array<juce::String, LayoutTuning::mainValueFieldCount> fieldPrefixes { { "Rate", "Depth", "Offset", "Width" } };
    for (int engineIndex = 0; engineIndex < LayoutTuning::engineCount; ++engineIndex)
    {
        for (int fieldIndex = 0; fieldIndex < LayoutTuning::mainValueFieldCount; ++fieldIndex)
        {
            auto& anim = layout.mainValueAnimationsByEngine[static_cast<std::size_t>(engineIndex)][static_cast<std::size_t>(fieldIndex)];
            const auto key = [&fieldPrefixes, &engineSuffixes, fieldIndex, engineIndex](const juce::String& suffix)
            {
                return juce::Identifier("mainValue" + fieldPrefixes[static_cast<std::size_t>(fieldIndex)] + suffix
                                        + engineSuffixes[static_cast<std::size_t>(engineIndex)]);
            };

            anim.fx.enabled = getIntOrDefault(layoutVar, key("FxEnabled"), layout.valueFxEnabled);
            anim.fx.glowAlphaPct = getIntOrDefault(layoutVar, key("GlowAlphaPct"), layout.valueGlowAlphaPct);
            anim.fx.glowSpreadPxTimes100 = getIntOrDefault(layoutVar, key("GlowSpreadPxTimes100"), layout.valueGlowSpreadPxTimes100);
            anim.fx.perCharOffsetXPxTimes100 = getIntOrDefault(layoutVar, key("PerCharOffsetXPxTimes100"), layout.valueFxPerCharOffsetXPxTimes100);
            anim.fx.perCharOffsetYPxTimes100 = getIntOrDefault(layoutVar, key("PerCharOffsetYPxTimes100"), layout.valueFxPerCharOffsetYPxTimes100);
            anim.fx.topReflectAlphaPct = getIntOrDefault(layoutVar, key("TopReflectAlphaPct"), layout.valueTopReflectAlphaPct);
            anim.fx.topReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, key("TopReflectOffsetXPxTimes100"), layout.valueTopReflectOffsetXPxTimes100);
            anim.fx.topReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, key("TopReflectOffsetYPxTimes100"), layout.valueTopReflectOffsetYPxTimes100);
            anim.fx.topReflectShearPct = getIntOrDefault(layoutVar, key("TopReflectShearPct"), layout.valueTopReflectShearPct);
            anim.fx.topReflectRotateDeg = getIntOrDefault(layoutVar, key("TopReflectRotateDeg"), layout.valueTopReflectRotateDeg);
            anim.fx.bottomReflectAlphaPct = getIntOrDefault(layoutVar, key("BottomReflectAlphaPct"), layout.valueBottomReflectAlphaPct);
            anim.fx.bottomReflectOffsetXPxTimes100 = getIntOrDefault(layoutVar, key("BottomReflectOffsetXPxTimes100"), layout.valueBottomReflectOffsetXPxTimes100);
            anim.fx.bottomReflectOffsetYPxTimes100 = getIntOrDefault(layoutVar, key("BottomReflectOffsetYPxTimes100"), layout.valueBottomReflectOffsetYPxTimes100);
            anim.fx.bottomReflectShearPct = getIntOrDefault(layoutVar, key("BottomReflectShearPct"), layout.valueBottomReflectShearPct);
            anim.fx.bottomReflectRotateDeg = getIntOrDefault(layoutVar, key("BottomReflectRotateDeg"), layout.valueBottomReflectRotateDeg);
            anim.fx.reflectBlurPxTimes100 = getIntOrDefault(layoutVar, key("ReflectBlurPxTimes100"), layout.valueReflectBlurPxTimes100);
            anim.fx.reflectSquashPct = getIntOrDefault(layoutVar, key("ReflectSquashPct"), layout.valueReflectSquashPct);
            anim.fx.reflectMotionPct = getIntOrDefault(layoutVar, key("ReflectMotionPct"), layout.valueReflectMotionPct);

            anim.flip.enabled = getIntOrDefault(layoutVar, key("FlipEnabled"), layout.mainValueFlipEnabled);
            anim.flip.durationMs = getIntOrDefault(layoutVar, key("FlipDurationMs"), layout.mainValueFlipDurationMs);
            anim.flip.travelUpPxTimes100 = getIntOrDefault(layoutVar, key("FlipTravelUpPxTimes100"), layout.mainValueFlipTravelUpPxTimes100);
            anim.flip.travelDownPxTimes100 = getIntOrDefault(layoutVar, key("FlipTravelDownPxTimes100"), layout.mainValueFlipTravelDownPxTimes100);
            anim.flip.travelOutPct = getIntOrDefault(layoutVar, key("FlipTravelOutPct"), layout.mainValueFlipTravelOutPct);
            anim.flip.travelInPct = getIntOrDefault(layoutVar, key("FlipTravelInPct"), layout.mainValueFlipTravelInPct);
            anim.flip.shearPct = getIntOrDefault(layoutVar, key("FlipShearPct"), layout.mainValueFlipShearPct);
            anim.flip.minScalePct = getIntOrDefault(layoutVar, key("FlipMinScalePct"), layout.mainValueFlipMinScalePct);
        }
    }

    return true;
}

void loadPersistedLayoutDefaults(LayoutTuning& layout)
{
    juce::String loadError;
    auto json = DefaultsPersistence::loadUser(&loadError);
    if (json.isEmpty())
        json = DefaultsPersistence::loadFactory(&loadError);

    loadLayoutDefaultsFromJson(layout, json);
}

void loadFactoryLayoutDefaults(LayoutTuning& layout)
{
    juce::String loadError;
    const auto json = DefaultsPersistence::loadFactory(&loadError);
    loadLayoutDefaultsFromJson(layout, json);
}

BackgroundAssetPack decodeBackgroundAssetPack(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    auto& assetRepository = choroboros::assets::AssetRepository::instance();
    BackgroundAssetPack pack;

    switch (colorIndex)
    {
        case 0:
            pack.off = assetRepository.loadImage(choroboros::assets::ids::greenBackgroundOff, true).image;
            pack.lit = assetRepository.loadImage(choroboros::assets::ids::greenBackgroundOn, true).image;
            break;
        case 1:
            pack.off = assetRepository.loadImage(choroboros::assets::ids::blueBackgroundOff, true).image;
            pack.lit = assetRepository.loadImage(choroboros::assets::ids::blueBackgroundOn, true).image;
            break;
        case 2:
            pack.off = assetRepository.loadImage(choroboros::assets::ids::redBackgroundOff, true).image;
            pack.lit = assetRepository.loadImage(choroboros::assets::ids::redBackgroundOn, true).image;
            break;
        case 3:
            pack.off = assetRepository.loadImage(choroboros::assets::ids::purpleBackgroundOff, true).image;
            pack.lit = assetRepository.loadImage(choroboros::assets::ids::purpleBackgroundOn, true).image;
            break;
        case 4:
        default:
            pack.off = assetRepository.loadImage(choroboros::assets::ids::blackBackgroundOff, true).image;
            pack.lit = assetRepository.loadImage(choroboros::assets::ids::blackBackgroundOn, true).image;
            break;
    }

    return pack;
}

BackgroundAssetPack getOrDecodeBackgroundAssetPack(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    const auto index = static_cast<size_t>(colorIndex);
    auto& cache = getSharedBackgroundCache();

    {
        const juce::ScopedLock lock(cache.lock);
        if (cache.valid[index])
            return cache.packs[index];
    }

    auto decoded = decodeBackgroundAssetPack(colorIndex);
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

class DevPanelWindow : public juce::DocumentWindow
{
public:
    DevPanelWindow(ChoroborosPluginEditor& editor, ChoroborosAudioProcessor& processor)
        : juce::DocumentWindow("Choroboros Beta Dev Panel",
                               juce::Colour(0xff202020),
                               juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setResizeLimits(1028, 525, 8192, 8192);
        setAlwaysOnTop(true);
        setContentOwned(new DevPanel(editor, processor), true);
        centreAroundComponent(&editor, 1100, 700);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};
} // namespace

class ChoroborosPluginEditor::HQLitOverlay : public juce::Component
{
public:
    explicit HQLitOverlay(ChoroborosPluginEditor& owner) : editor(owner)
    {
        setInterceptsMouseClicks(false, false);
    }

    void resized() override
    {
        invalidateCache();
    }

    void invalidateCache()
    {
        cachedScaledImage = {};
        cachedWidth = 0;
        cachedHeight = 0;
        cachedSourceWidth = 0;
        cachedSourceHeight = 0;
    }

    void paint(juce::Graphics& g) override
    {
        if (!editor.backgroundImageLit.isValid())
            return;

        const float litOpacity = editor.hqButton.getAnimationProgress();
        if (litOpacity <= 0.0f)
            return;

        refreshCachedImageIfNeeded();
        if (!cachedScaledImage.isValid())
            return;

        g.setOpacity(litOpacity);
        g.drawImageAt(cachedScaledImage, 0, 0);
    }

private:
    void refreshCachedImageIfNeeded()
    {
        const auto& source = editor.backgroundImageLit;
        const int width = getWidth();
        const int height = getHeight();

        if (!source.isValid() || width <= 0 || height <= 0)
            return;

        if (cachedScaledImage.isValid()
            && cachedWidth == width && cachedHeight == height
            && cachedSourceWidth == source.getWidth()
            && cachedSourceHeight == source.getHeight())
            return;

        cachedScaledImage = juce::Image(juce::Image::ARGB, width, height, true);
        juce::Graphics cachedGraphics(cachedScaledImage);
        cachedGraphics.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        cachedGraphics.drawImage(source, 0, 0, width, height, 0, 0, source.getWidth(), source.getHeight());

        cachedWidth = width;
        cachedHeight = height;
        cachedSourceWidth = source.getWidth();
        cachedSourceHeight = source.getHeight();
    }

    ChoroborosPluginEditor& editor;
    juce::Image cachedScaledImage;
    int cachedWidth = 0;
    int cachedHeight = 0;
    int cachedSourceWidth = 0;
    int cachedSourceHeight = 0;
};

//==============================================================================
ChoroborosPluginEditor::ChoroborosPluginEditor (ChoroborosAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    editorCtorStartMs = juce::Time::getMillisecondCounterHiRes();
    setLookAndFeel(&customLookAndFeel);
    hqLitOverlay_ = std::make_unique<HQLitOverlay>(*this);
    addAndMakeVisible(*hqLitOverlay_);
    hqLitOverlay_->toBack();

    int initialEngineIndex = 0;
    if (auto* engineColorParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
        initialEngineIndex = juce::jlimit(0, 4, static_cast<int>(engineColorParam->load()));

    activeThemeDecodeColorIndex = initialEngineIndex;
    activeThemeDecodeFuture = std::async(std::launch::async, [initialEngineIndex]()
    {
        return CustomLookAndFeel::getOrDecodeThemeAssetPack(initialEngineIndex);
    });

    const double fontSetupStartMs = juce::Time::getMillisecondCounterHiRes();
    loadValueLabelTypeface();
    loadUiTextTypeface();
    customLookAndFeel.setUiTextTypeface(uiTextTypeface);
    audioProcessor.logLoadTraceEvent("editor_font_setup_ms",
                                     juce::Time::getMillisecondCounterHiRes() - fontSetupStartMs);

    const double layoutDefaultsStartMs = juce::Time::getMillisecondCounterHiRes();
    layoutTuning = PluginEditorSetup::makeDefaultLayout();
    loadPersistedLayoutDefaults(layoutTuning);
    audioProcessor.logLoadTraceEvent("editor_layout_defaults_ms",
                                     juce::Time::getMillisecondCounterHiRes() - layoutDefaultsStartMs);
    
    // Create branded top header bar with preset browser
    if (audioProcessor.presetManager)
    {
        topHeaderBar_ = std::make_unique<TopHeaderBar> (*audioProcessor.presetManager, getUiScale());
        addAndMakeVisible (*topHeaderBar_);
    }

    const double themeSetupStartMs = juce::Time::getMillisecondCounterHiRes();
    setupEngineColorSelector();
    if (CustomLookAndFeel::isThemeAssetPackCached(initialEngineIndex))
    {
        customLookAndFeel.setColorTheme(initialEngineIndex);
        activeThemeInstalled = true;
    }
    // Note: setupEngineColorSelector now reads the saved parameter value and updates UI
    audioProcessor.logLoadTraceEvent("editor_theme_setup_ms",
                                     juce::Time::getMillisecondCounterHiRes() - themeSetupStartMs);
    
    const double controlsSetupStartMs = juce::Time::getMillisecondCounterHiRes();
    // Setup sliders with exact bounds
    setupSlider(rateSlider, rateLabel, rateValueLabel, "RATE", ChoroborosAudioProcessor::RATE_ID);
    setupSlider(depthSlider, depthLabel, depthValueLabel, "DEPTH", ChoroborosAudioProcessor::DEPTH_ID);
    setupSlider(offsetSlider, offsetLabel, offsetValueLabel, "OFFSET", ChoroborosAudioProcessor::OFFSET_ID);
    setupSlider(widthSlider, widthLabel, widthValueLabel, "WIDTH", ChoroborosAudioProcessor::WIDTH_ID);
    setupSlider(colorSlider, colorLabel, colorValueLabel, "COLOR", ChoroborosAudioProcessor::COLOR_ID);
    setupSlider(mixSlider, mixLabel, mixValueLabel, "MIX", ChoroborosAudioProcessor::MIX_ID);
    
    PluginEditorSetup::setupSliders(*this);
    applyTuningToUI();
    setupSliderAttachments();
    audioProcessor.logLoadTraceEvent("editor_controls_setup_ms",
                                     juce::Time::getMillisecondCounterHiRes() - controlsSetupStartMs);
    
    PluginEditorSetup::setupHQButton(*this);
    hqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::HQ_ID, hqButton);
    customLookAndFeel.setHqAnimationState(hqButton.getAnimationProgress(),
                                         hqButton.isAnimating(),
                                         hqButton.isOn());
    
    PluginEditorSetup::setupValueLabels(*this);
    PluginEditorSetup::setupLabels(*this);
    setupSliderValueChangeListeners();
    
    applyCurrentEngineVisual();
    
    // Set up double-click editing for value labels
    setupValueLabelEditing(rateValueLabel, rateSlider, ChoroborosAudioProcessor::RATE_ID);
    setupValueLabelEditing(depthValueLabel, depthSlider, ChoroborosAudioProcessor::DEPTH_ID);
    setupValueLabelEditing(offsetValueLabel, offsetSlider, ChoroborosAudioProcessor::OFFSET_ID);
    setupValueLabelEditing(widthValueLabel, widthSlider, ChoroborosAudioProcessor::WIDTH_ID);
    setupValueLabelEditing(colorValueLabel, colorSlider, ChoroborosAudioProcessor::COLOR_ID);
    setupValueLabelEditing(mixValueLabel, mixSlider, ChoroborosAudioProcessor::MIX_ID);
    
    // Initial value updates
    updateValueLabel(rateValueLabel, rateSlider.getValue(), ChoroborosAudioProcessor::RATE_ID);
    updateValueLabel(depthValueLabel, depthSlider.getValue(), ChoroborosAudioProcessor::DEPTH_ID);
    updateValueLabel(offsetValueLabel, offsetSlider.getValue(), ChoroborosAudioProcessor::OFFSET_ID);
    updateValueLabel(widthValueLabel, widthSlider.getValue(), ChoroborosAudioProcessor::WIDTH_ID);
    updateValueLabel(colorValueLabel, colorSlider.getValue(), ChoroborosAudioProcessor::COLOR_ID);
    updateValueLabel(mixValueLabel, mixSlider.getValue(), ChoroborosAudioProcessor::MIX_ID);
    
    // Setup tooltip window
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
    
    // ---- Top-bar sliding icon-button drawer -----------------------------------
    {
        auto devIcon = loadSoftwareImageFromMemory (
            BinaryData::dev_png, BinaryData::dev_pngSize);
        auto aboutIcon = loadSoftwareImageFromMemory (
            BinaryData::about_png, BinaryData::about_pngSize);
        auto helpIcon = loadSoftwareImageFromMemory (
            BinaryData::help_png, BinaryData::help_pngSize);
        auto feedbackIcon = loadSoftwareImageFromMemory (
            BinaryData::bug_feedback_button_png, BinaryData::bug_feedback_button_pngSize);

        topBarDrawer.setupIcons (devIcon, aboutIcon, helpIcon, feedbackIcon);
        topBarDrawer.setupLayout (getUiScale());

        // Set initial drawer accent colour to match the current engine
        if (auto* engineColorParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
        {
            topBarDrawer.setAccentColour (devpanel::engineSkinColourForIndex (
                juce::jlimit (0, 4, static_cast<int> (engineColorParam->load()))));
            if (topHeaderBar_)
                topHeaderBar_->setAccentColour (devpanel::engineSkinColourForIndex (
                    juce::jlimit (0, 4, static_cast<int> (engineColorParam->load()))));
        }

        // Wire up button callbacks (tooltips are now handled by the drawer
        // itself via hover-expansion, not native JUCE tooltips)
        topBarDrawer.devButton.onClick = [this]
        {
            if (isShuttingDown())
                return;

            ensureDevPanelWindowCreated (true);
            if (devWindow == nullptr)
                return;

            const bool shouldShow = !devWindow->isVisible();
            devWindow->setVisible (shouldShow);
            if (shouldShow)
            {
                choroboros::windows::applyPreferredRenderer(*devWindow,
                                                            "editor_dev_panel_show",
                                                            &audioProcessor);
                devWindow->toFront (true);
            }
        };

        topBarDrawer.aboutButton.onClick = [this] { showAboutWindow(); };

        topBarDrawer.helpButton.onClick = [this] { showHelpWindow(); };

        topBarDrawer.feedbackButton.onClick = [this] {
            showFeedbackWindow();
        };

        addAndMakeVisible (topBarDrawer);
    }

    postUiTask([](ChoroborosPluginEditor& editor)
    {
        editor.applyWindowsRenderPolicyToMainPeer();
    });
    
    // Listen for engine color changes (preset load or manual) to update value label colors
    audioProcessor.getValueTreeState().addParameterListener(ChoroborosAudioProcessor::ENGINE_COLOR_ID, this);
    
    // Set fixed size (original content height + header bar)
    setSize(uiScaleInt(700), uiScaleInt(363) + getHeaderBarHeight());
    applyLayout();
    setResizable(false, false);

#if CHOROBOROS_INSPECTOR_ENABLED
    setWantsKeyboardFocus(true);
#endif

    audioProcessor.logLoadTraceEvent("editor_ctor_total_ms",
                                     juce::Time::getMillisecondCounterHiRes() - editorCtorStartMs);

    // Check for pending crash report from a previous unclean shutdown.
    // Deferred so the editor is fully visible before the dialog appears.
    if (SessionLog::hasPendingCrashReport())
    {
        postUiTaskAfterDelay(1500, [](ChoroborosPluginEditor& editor)
        {
            auto crashReport = SessionLog::readPendingCrashReport();
            if (crashReport.isNotEmpty())
                editor.showCrashReportWindow(crashReport);
            // Crash report file is NOT deleted here -- FeedbackDialog clears
            // it after the user sends, saves, or dismisses.
        });
    }
}

// Shared typeface caches -- ref-counted via SharedResourcePointer so they are
// destroyed when the last editor instance dies, not during DLL_PROCESS_DETACH.
struct ValueLabelTypefaceCache
{
    juce::Typeface::Ptr typeface = (BinaryData::Technology_ttfSize > 0)
        ? juce::Typeface::createSystemTypefaceFor(BinaryData::Technology_ttf,
                                                   static_cast<size_t>(BinaryData::Technology_ttfSize))
        : nullptr;
};

struct UiTextTypefaceCache
{
    juce::Typeface::Ptr typeface = (BinaryData::Retroica_ttfSize > 0)
        ? juce::Typeface::createSystemTypefaceFor(BinaryData::Retroica_ttf,
                                                   static_cast<size_t>(BinaryData::Retroica_ttfSize))
        : nullptr;
};

void ChoroborosPluginEditor::setTooltipsEnabled(bool enabled)
{
    if (tooltipWindow != nullptr)
        tooltipWindow->setEnabled(enabled);
}

void ChoroborosPluginEditor::showStatusDialog(juce::AlertWindow::AlertIconType iconType,
                                              const juce::String& title,
                                              const juce::String& message,
                                              const juce::String& telemetryContext,
                                              juce::Component* anchorComponent)
{
    showStatusWindow(iconType, title, message, telemetryContext, anchorComponent);
}

void ChoroborosPluginEditor::showConfirmationDialog(const juce::String& title,
                                                    const juce::String& message,
                                                    const juce::String& confirmText,
                                                    const juce::String& cancelText,
                                                    bool warningTone,
                                                    std::function<void(bool)> onDecision,
                                                    const juce::String& telemetryContext,
                                                    juce::Component* anchorComponent)
{
    closeManagedDialogWindow(confirmationWindow);
    showManagedDialogWindow(confirmationWindow,
                            std::make_unique<ConfirmDialog>(title,
                                                            message,
                                                            confirmText,
                                                            cancelText,
                                                            warningTone,
                                                            std::move(onDecision)),
                            title,
                            true,
                            420, 260, 860, 720,
                            telemetryContext,
                            anchorComponent);
}

void ChoroborosPluginEditor::showTextEntryDialog(const juce::String& title,
                                                 const juce::String& prompt,
                                                 const juce::String& initialText,
                                                 const juce::String& confirmText,
                                                 const juce::String& cancelText,
                                                 bool warningTone,
                                                 std::function<void(bool, const juce::String&)> onDecision,
                                                 const juce::String& telemetryContext,
                                                 juce::Component* anchorComponent)
{
    closeManagedDialogWindow(textEntryWindow);
    showManagedDialogWindow(textEntryWindow,
                            std::make_unique<TextEntryDialog>(title,
                                                              prompt,
                                                              initialText,
                                                              confirmText,
                                                              cancelText,
                                                              warningTone,
                                                              std::move(onDecision)),
                            title,
                            false,
                            420, 200, 640, 360,
                            telemetryContext,
                            anchorComponent);
}

void ChoroborosPluginEditor::loadValueLabelTypeface()
{
    if (BinaryData::Technology_ttfSize <= 0)
        return;

    juce::SharedResourcePointer<ValueLabelTypefaceCache> cache;
    valueLabelTypeface = cache->typeface;
}

void ChoroborosPluginEditor::loadUiTextTypeface()
{
    if (BinaryData::Retroica_ttfSize <= 0)
        return;

    juce::SharedResourcePointer<UiTextTypefaceCache> cache;
    uiTextTypeface = cache->typeface;
}

juce::Font ChoroborosPluginEditor::makeValueLabelFont(float heightPx, bool bold) const
{
    juce::FontOptions options { heightPx };
    if (bold)
        options = juce::FontOptions { heightPx, juce::Font::bold };

    juce::Font font { options };
    if (valueLabelTypeface != nullptr)
        return juce::Font { juce::FontOptions { valueLabelTypeface }.withHeight(heightPx) };

    if (bold)
        font.setBold(true);
    return font;
}

juce::Font ChoroborosPluginEditor::makeUiTextFont(float heightPx, bool bold) const
{
    juce::FontOptions options { heightPx };
    if (bold)
        options = juce::FontOptions { heightPx, juce::Font::bold };

    juce::Font font { options };
    if (uiTextTypeface != nullptr)
        return juce::Font { juce::FontOptions { uiTextTypeface }.withHeight(heightPx) };

    if (bold)
        font.setBold(true);
    return font;
}

ChoroborosPluginEditor::~ChoroborosPluginEditor()
{
    shutdownFence.store(true);

    // 1. Remove parameter listener FIRST -- prevents audio thread from calling
    //    parameterChanged() on a half-destroyed editor (Cubase/Reaper freeze).
    audioProcessor.getValueTreeState().removeParameterListener(ChoroborosAudioProcessor::ENGINE_COLOR_ID, this);

    // 2. Null out all slider callbacks that capture raw `this` -- these can
    //    fire during component teardown as attachments are destroyed.
    rateSlider.onValueChange = nullptr;
    depthSlider.onValueChange = nullptr;
    offsetSlider.onValueChange = nullptr;
    widthSlider.onValueChange = nullptr;
    colorSlider.onValueChange = nullptr;
    mixSlider.onValueChange = nullptr;

    // 3. Destroy attachments before sliders -- prevents dangling parameter
    //    callbacks during member destruction order.
    rateAttachment.reset();
    depthAttachment.reset();
    offsetAttachment.reset();
    widthAttachment.reset();
    colorAttachment.reset();
    mixAttachment.reset();
    hqAttachment.reset();
    engineColorAttachment.reset();

    // 4. Stop background theme thread.
    stopDeferredThemePrewarm();

#if CHOROBOROS_INSPECTOR_ENABLED
    inspector.reset();
#endif

    // 5. Explicitly destroy child windows BEFORE component teardown.
    //    On Windows, visible DocumentWindows destroyed during DLL_PROCESS_DETACH
    //    can trigger cascading HWND messages that deadlock or crash (Cubase).
    //    Hide first, remove native peer, THEN delete the C++ object.
    closeManagedWindows();
    if (devWindow != nullptr)
        closeManagedDocumentWindow(devWindow);

    // 6. Detach look-and-feel last.
    setLookAndFeel(nullptr);
}

#if CHOROBOROS_INSPECTOR_ENABLED
bool ChoroborosPluginEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('i', juce::ModifierKeys::commandModifier, 0))
    {
        if (inspector == nullptr)
            inspector = std::make_unique<melatonin::Inspector> (*this, false);
        inspector->setVisible (! inspector->isVisible());
        return true;
    }
    return Component::keyPressed (key);
}
#endif

void ChoroborosPluginEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();
    applyWindowsRenderPolicyToMainPeer();
    applyWindowsRenderPolicyToManagedWindows();
}

void ChoroborosPluginEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == ChoroborosAudioProcessor::ENGINE_COLOR_ID)
    {
        const int newId = juce::roundToInt(newValue) + 1;
        postUiTask([newId](ChoroborosPluginEditor& editor)
        {
            if (!editor.audioProcessor.hasActiveCustomEngine()
                && editor.engineColorBox.getSelectedId() != newId)
            {
                editor.engineColorBox.setSelectedId(newId, juce::sendNotificationSync);
            }

            // Engine switches can arrive from host state restore, preset loads,
            // or the engine selector itself. Reapplying the current engine visual
            // here ensures the per-engine layout follows the settled parameter
            // value rather than a stale pre-notify engine index.
            editor.applyCurrentEngineVisual();

            if (editor.audioProcessor.presetManager
                && ! editor.audioProcessor.presetManager->isLoadInProgress())
                editor.audioProcessor.presetManager->invalidatePreset();
        });
    }
}

//==============================================================================
void ChoroborosPluginEditor::paint (juce::Graphics& g)
{
#if CHOROBOROS_PERFETTO_ENABLED
    TRACE_COMPONENT();
#endif

    if (!activeThemeInstalled && activeThemeDecodeFuture.valid())
    {
        // Non-blocking check: is the async theme decode finished?
        if (activeThemeDecodeFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            // Not ready yet. Draw black fallback and check again next frame.
            g.fillAll(juce::Colours::black);
            // Schedule repaint so we poll again on the next message loop iteration.
            postUiTask([](ChoroborosPluginEditor& editor)
            {
                editor.repaint();
            });
            return;
        }

        const double waitStartMs = juce::Time::getMillisecondCounterHiRes();
        auto themePack = activeThemeDecodeFuture.get();  // guaranteed immediate, future is ready
        customLookAndFeel.installThemeAssetPack(activeThemeDecodeColorIndex, std::move(themePack));
        activeThemeInstalled = true;

        audioProcessor.logLoadTraceEvent("editor_theme_wait_before_first_paint_ms",
                                         juce::Time::getMillisecondCounterHiRes() - waitStartMs,
                                         "engine=" + juce::String(activeThemeDecodeColorIndex));
    }

    // Install any themes decoded by the prewarm worker thread.
    {
        std::vector<PrewarmedTheme> ready;
        {
            std::lock_guard<std::mutex> lock(prewarmQueueMutex);
            ready.swap(prewarmQueue);
        }
        for (auto& t : ready)
        {
            customLookAndFeel.installThemeAssetPack(t.colorIndex, std::move(t.pack));
            if (t.colorIndex == t.activeColorIndex)
            {
                backgroundImage = t.backgroundPack.off;
                backgroundImageLit = t.backgroundPack.lit;
                invalidateHQLitOverlayCache();
                audioProcessor.logLoadTraceEvent(
                    "editor_active_theme_ready_ms",
                    juce::Time::getMillisecondCounterHiRes() - editorCtorStartMs,
                    "engine=" + juce::String(t.activeColorIndex));
            }
        }
        if (!ready.empty())
            repaint();
    }

    if (!firstPaintTimingLogged)
    {
        firstPaintTimingLogged = true;
        audioProcessor.logLoadTraceEvent("editor_first_paint_ms",
                                         juce::Time::getMillisecondCounterHiRes() - editorCtorStartMs);

        if (!themePrewarmStarted)
        {
            themePrewarmStarted = true;
            const int activeEngineIndex = juce::jlimit(0, 4, engineColorBox.getSelectedId() - 1);
            startDeferredThemePrewarm(activeEngineIndex);
        }

        scheduleDeferredDevPanelPrewarm();
    }

    // Fill the whole window (including header area) with black
    g.fillAll (juce::Colours::black);

    // Draw background below the header bar
    const int yOff = getHeaderBarHeight();
    const int contentH = getHeight() - yOff;

    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, 0, yOff, getWidth(), contentH, 0, 0,
                   backgroundImage.getWidth(), backgroundImage.getHeight());
    }

    // (Top-bar container is now painted by the TopBarDrawer component)
}

void ChoroborosPluginEditor::resized()
{
    layoutTopChrome();
}

void ChoroborosPluginEditor::visibilityChanged()
{
    // Logic Pro (AU) sometimes hides the editor window without destroying it.
    // The editor object stays alive, background threads keep running, and
    // renderOpenGL (if used) keeps firing.  When we become invisible, stop
    // the expensive background theme prewarm thread.  When we become visible
    // again, restart it so the next engine switch is snappy.
    if (!isVisible())
    {
        stopDeferredThemePrewarm();
        return;
    }

    applyWindowsRenderPolicyToMainPeer();
    applyWindowsRenderPolicyToManagedWindows();
}

void ChoroborosPluginEditor::setScaleFactor(float newScale)
{
    if (!std::isfinite(newScale))
        return;

    const float clampedScale = juce::jmax(0.5f, newScale);
    if (std::abs(clampedScale - dpiScale) < 0.001f)
        return;

    rebuildScaleSensitiveUI(clampedScale);
}

void ChoroborosPluginEditor::repaintHQLitOverlay()
{
    if (hqLitOverlay_ != nullptr)
        hqLitOverlay_->repaint();
}

void ChoroborosPluginEditor::invalidateHQLitOverlayCache()
{
    if (hqLitOverlay_ != nullptr)
        hqLitOverlay_->invalidateCache();
}

void ChoroborosPluginEditor::refreshEditorSurfaceAfterEngineVisualChange()
{
    rateSlider.repaint();
    depthSlider.repaint();
    offsetSlider.repaint();
    widthSlider.repaint();
    colorSlider.repaint();
    mixSlider.repaint();
    repaintHQLitOverlay();

    // Engine visual swaps can move and resize child components. Force the
    // parent editor surface underneath them to redraw too, otherwise some
    // hosts preserve stale pixels from the previous engine skin/layout.
    const int headerBarHeight = getHeaderBarHeight();
    repaint(0, headerBarHeight, getWidth(), juce::jmax(0, getHeight() - headerBarHeight));
}

void ChoroborosPluginEditor::invalidateScaleSensitiveCaches()
{
    invalidateHQLitOverlayCache();
    hqButton.invalidateScaledFrameCache();
}

void ChoroborosPluginEditor::layoutTopChrome()
{
    const auto s = [this](int value)
    {
        return juce::roundToInt(static_cast<float>(value) * getUiScale());
    };

    const int headerBarHeight = getHeaderBarHeight();
    if (topHeaderBar_ != nullptr)
        topHeaderBar_->setBounds(0, 0, getWidth(), headerBarHeight);

    if (hqLitOverlay_ != nullptr)
    {
        hqLitOverlay_->setBounds(0, headerBarHeight, getWidth(), juce::jmax(0, getHeight() - headerBarHeight));
        hqLitOverlay_->toBack();
    }

    const int drawerWidth = topBarDrawer.getExpandedWidth();
    const int drawerHeight = topBarDrawer.getTotalHeight();
    const int marginRight = s(4);
    const int marginTop = juce::jmax(0, (headerBarHeight - topBarDrawer.getDrawerHeight()) / 2);
    topBarDrawer.setBounds(juce::jmax(0, getWidth() - drawerWidth - marginRight),
                           marginTop,
                           drawerWidth,
                           drawerHeight);
}

void ChoroborosPluginEditor::applyLayoutInternal(bool shouldRepaint)
{
    PluginEditorSetup::applyLayout(*this, layoutTuning);
    layoutTopChrome();

    if (shouldRepaint)
        repaint();
}

void ChoroborosPluginEditor::rebuildScaleSensitiveUI(float newScale)
{
    dpiScale = newScale;
    juce::AudioProcessorEditor::setScaleFactor(newScale);

    if (topHeaderBar_ != nullptr)
        topHeaderBar_->setUiScale(getUiScale());

    topBarDrawer.setUiScale(getUiScale());
    applyLayoutInternal(false);
    invalidateScaleSensitiveCaches();
    repaint();
}

void ChoroborosPluginEditor::applyLayout()
{
    applyLayoutInternal(true);
}

void ChoroborosPluginEditor::resetLayoutToFactoryDefaults()
{
    layoutTuning = PluginEditorSetup::makeDefaultLayout();
    loadFactoryLayoutDefaults(layoutTuning);
    applyLayout();
    refreshValueLabels();
}

void ChoroborosPluginEditor::applyTuningToUI()
{
    const auto& tuning = audioProcessor.getTuningState();
    rateSlider.setSkewFactor(tuning.rate.uiSkew.load());
    depthSlider.setSkewFactor(tuning.depth.uiSkew.load());
    offsetSlider.setSkewFactor(tuning.offset.uiSkew.load());
    widthSlider.setSkewFactor(tuning.width.uiSkew.load());
    colorSlider.setSkewFactor(tuning.color.uiSkew.load());
    colorSlider.setValue(colorSlider.getValue(), juce::dontSendNotification);
    mixSlider.setSkewFactor(tuning.mix.uiSkew.load());
}

void ChoroborosPluginEditor::refreshValueLabels()
{
    updateValueLabel(rateValueLabel, rateSlider.getValue(), ChoroborosAudioProcessor::RATE_ID);
    updateValueLabel(depthValueLabel, depthSlider.getValue(), ChoroborosAudioProcessor::DEPTH_ID);
    updateValueLabel(offsetValueLabel, offsetSlider.getValue(), ChoroborosAudioProcessor::OFFSET_ID);
    updateValueLabel(widthValueLabel, widthSlider.getValue(), ChoroborosAudioProcessor::WIDTH_ID);
    updateValueLabel(colorValueLabel, colorSlider.getValue(), ChoroborosAudioProcessor::COLOR_ID);
    updateValueLabel(mixValueLabel, mixSlider.getValue(), ChoroborosAudioProcessor::MIX_ID);
    repaint();
}

void ChoroborosPluginEditor::setupEngineColorSelector()
{
    rebuildEngineSelectorItems();
    engineColorBox.setSelectedId(1, juce::dontSendNotification);

    if (topHeaderBar_)
        topHeaderBar_->setEngineSelector (&engineColorBox);

    engineColorBox.setTooltip("Engine Selection: Choose between five distinct chorus algorithms. Green=Classic, Blue=Modern, Red=Vintage, Purple=Experimental, Black=Linear.");

    engineColorBox.onChange = [this]
    {
        if (engineSwitchInProgress)
            return;

        const int selectedId = engineColorBox.getSelectedId();
        if (selectedId <= 0 || !audioProcessor.customEngineManager)
            return;

        const auto& engines = audioProcessor.customEngineManager->getEngines();
        const int engineIdx = selectedId - 1;
        if (engineIdx < 0 || engineIdx >= static_cast<int>(engines.size()))
            return;

        engineSwitchInProgress = true;
        const auto& engine = engines[static_cast<size_t>(engineIdx)];

        if (engine.isFactory)
        {
            if (audioProcessor.hasActiveCustomEngine())
                audioProcessor.deactivateCustomEngine();

            if (auto* param = audioProcessor.getValueTreeState().getParameter(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(engine.factoryIndex)));

            if (auto* collector = audioProcessor.getFeedbackCollector())
            {
                auto hqParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::HQ_ID);
                const bool hq = hqParam ? (hqParam->load() > 0.5f) : false;
                collector->trackEngineSwitch(engine.factoryIndex, hq);
            }
        }
        else
        {
            audioProcessor.activateCustomEngine(engine.id);
        }

        applyEngineVisual(engine.visual);
        applyTuningToUI();
        refreshEditorSurfaceAfterEngineVisualChange();
        engineSwitchInProgress = false;
    };

    applyCurrentEngineVisual();
}

void ChoroborosPluginEditor::rebuildEngineSelectorItems()
{
    engineColorBox.clear(juce::dontSendNotification);

    if (audioProcessor.customEngineManager)
    {
        const auto& engines = audioProcessor.customEngineManager->getEngines();
        for (int i = 0; i < static_cast<int>(engines.size()); ++i)
            engineColorBox.addItem(engines[static_cast<size_t>(i)].name, i + 1);
        return;
    }

    engineColorBox.addItem("Green", 1);
    engineColorBox.addItem("Blue", 2);
    engineColorBox.addItem("Red", 3);
    engineColorBox.addItem("Purple", 4);
    engineColorBox.addItem("Black", 5);
}

void ChoroborosPluginEditor::startDeferredThemePrewarm(int activeColorIndex)
{
    stopDeferredThemePrewarm();

    // Each thread invocation gets its own stop flag so a prior join + new start
    // does not race on the shared atomic.
    themePrewarmStopFlag = std::make_shared<std::atomic<bool>>(false);
    auto stopFlag = themePrewarmStopFlag;  // shared_ptr copy -- thread owns a ref

    themePrewarmThread = std::thread([this, stopFlag, activeColorIndex]()
    {
        std::array<int, 5> prewarmOrder { activeColorIndex, 0, 1, 2, 3 };
        int orderCursor = 1;
        for (int i = 0; i < 5 && orderCursor < 5; ++i)
        {
            if (i == activeColorIndex) continue;
            prewarmOrder[static_cast<size_t>(orderCursor++)] = i;
        }

        for (int i = 0; i < orderCursor; ++i)
        {
            if (stopFlag->load()) return;

            const int colorIndex = prewarmOrder[static_cast<size_t>(i)];
            auto backgroundPack = getOrDecodeBackgroundAssetPack(colorIndex);
            auto pack = CustomLookAndFeel::getOrDecodeThemeAssetPack(colorIndex);

            if (stopFlag->load()) return;

            {
                std::lock_guard<std::mutex> lock(prewarmQueueMutex);
                prewarmQueue.push_back({ colorIndex, activeColorIndex,
                                         std::move(pack), std::move(backgroundPack) });
            }
        }
    });
}

void ChoroborosPluginEditor::stopDeferredThemePrewarm()
{
    themePrewarmStopFlag->store(true);

    // Safe to join now: the worker thread no longer calls callAsync or touches
    // the MessageManager, so joining from the message thread cannot deadlock.
    if (themePrewarmThread.joinable())
        themePrewarmThread.join();
}

void ChoroborosPluginEditor::postUiTask(std::function<void(ChoroborosPluginEditor&)> task)
{
    if (shutdownFence.load())
        return;

    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);
    juce::MessageManager::callAsync([safeThis, task = std::move(task)]() mutable
    {
        if (safeThis == nullptr || safeThis->shutdownFence.load())
            return;

        task(*safeThis);
    });
}

void ChoroborosPluginEditor::postUiTaskAfterDelay(int delayMs,
                                                  std::function<void(ChoroborosPluginEditor&)> task)
{
    if (delayMs <= 0)
    {
        postUiTask(std::move(task));
        return;
    }

    if (shutdownFence.load())
        return;

    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);
    juce::Timer::callAfterDelay(delayMs, [safeThis, task = std::move(task)]() mutable
    {
        if (safeThis == nullptr || safeThis->shutdownFence.load())
            return;

        task(*safeThis);
    });
}

void ChoroborosPluginEditor::applyWindowsRenderPolicyToMainPeer()
{
    choroboros::windows::applyPreferredRenderer(*this, "editor_main_peer", &audioProcessor);
}

void ChoroborosPluginEditor::applyWindowsRenderPolicyToManagedWindows()
{
    if (devWindow != nullptr && devWindow->isVisible())
        choroboros::windows::applyPreferredRenderer(*devWindow, "editor_dev_panel", &audioProcessor);

    if (aboutWindow != nullptr && aboutWindow->isVisible())
        choroboros::windows::applyPreferredRenderer(*aboutWindow, "editor_about_dialog", &audioProcessor);

    if (helpWindow != nullptr && helpWindow->isVisible())
        choroboros::windows::applyPreferredRenderer(*helpWindow, "editor_help_dialog", &audioProcessor);

    if (feedbackWindow != nullptr && feedbackWindow->isVisible())
        choroboros::windows::applyPreferredRenderer(*feedbackWindow, "editor_feedback_dialog", &audioProcessor);

    if (messageWindow != nullptr && messageWindow->isVisible())
        choroboros::windows::applyPreferredRenderer(*messageWindow, "editor_status_dialog", &audioProcessor);

    if (confirmationWindow != nullptr && confirmationWindow->isVisible())
        choroboros::windows::applyPreferredRenderer(*confirmationWindow, "editor_confirmation_dialog", &audioProcessor);

    if (textEntryWindow != nullptr && textEntryWindow->isVisible())
        choroboros::windows::applyPreferredRenderer(*textEntryWindow, "editor_text_entry_dialog", &audioProcessor);
}

void ChoroborosPluginEditor::closeManagedDialogWindow(std::unique_ptr<juce::DialogWindow>& window)
{
    if (window == nullptr)
        return;

    window->exitModalState(0);
    window->setVisible(false);
    window->removeFromDesktop();
    window.reset();
}

void ChoroborosPluginEditor::closeManagedDocumentWindow(std::unique_ptr<juce::DocumentWindow>& window)
{
    if (window == nullptr)
        return;

    window->setVisible(false);
    window->removeFromDesktop();
    window.reset();
}

void ChoroborosPluginEditor::closeManagedWindows()
{
    closeManagedDialogWindow(textEntryWindow);
    closeManagedDialogWindow(confirmationWindow);
    closeManagedDialogWindow(messageWindow);
    closeManagedDialogWindow(feedbackWindow);
    closeManagedDialogWindow(helpWindow);
    closeManagedDialogWindow(aboutWindow);
}

void ChoroborosPluginEditor::showManagedDialogWindow(std::unique_ptr<juce::DialogWindow>& slot,
                                                     std::unique_ptr<juce::Component> content,
                                                     const juce::String& title,
                                                     bool resizable,
                                                     int minWidth,
                                                     int minHeight,
                                                     int maxWidth,
                                                     int maxHeight,
                                                     const juce::String& telemetryContext,
                                                     juce::Component* centreAround)
{
    if (shutdownFence.load())
        return;

    if (slot == nullptr)
    {
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(content.release());
        options.dialogTitle = title;
        options.dialogBackgroundColour = devpanel::hackerBg();
        options.componentToCentreAround = centreAround != nullptr ? centreAround : this;
        options.resizable = resizable;
        options.useNativeTitleBar = true;

        slot.reset(options.create());
        if (slot == nullptr)
            return;

        if (resizable && minWidth > 0 && minHeight > 0 && maxWidth >= minWidth && maxHeight >= minHeight)
            slot->setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);
    }

    if (centreAround != nullptr)
        slot->centreAroundComponent(centreAround, slot->getWidth(), slot->getHeight());

    slot->setVisible(true);
    choroboros::windows::applyPreferredRenderer(*slot, telemetryContext, &audioProcessor);
    slot->toFront(true);
}

void ChoroborosPluginEditor::showAboutWindow()
{
    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);
    showManagedDialogWindow(aboutWindow,
                            aboutWindow == nullptr
                                ? std::unique_ptr<juce::Component>(std::make_unique<AboutDialog>(
                                    [safeThis](juce::AlertWindow::AlertIconType iconType,
                                               const juce::String& title,
                                               const juce::String& message)
                                    {
                                        if (safeThis == nullptr || safeThis->isShuttingDown())
                                            return;

                                        safeThis->showStatusWindow(iconType, title, message, "editor_about_license");
                                    }).release())
                                : std::unique_ptr<juce::Component>(),
                            "About Choroboros",
                            false,
                            0, 0, 0, 0,
                            "editor_about_dialog",
                            this);
}

void ChoroborosPluginEditor::showHelpWindow()
{
    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);
    showManagedDialogWindow(helpWindow,
                            helpWindow == nullptr
                                ? std::make_unique<HelpDialog>(
                                    [safeThis](juce::AlertWindow::AlertIconType iconType,
                                               const juce::String& title,
                                               const juce::String& message)
                                    {
                                        if (safeThis == nullptr || safeThis->isShuttingDown())
                                            return;

                                        safeThis->showStatusWindow(iconType, title, message, "editor_help_link_failure");
                                    })
                                : std::unique_ptr<juce::Component>(),
                            "Help & Support",
                            false,
                            0, 0, 0, 0,
                            "editor_help_dialog",
                            this);
}

void ChoroborosPluginEditor::showFeedbackWindow()
{
    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);

    closeManagedDialogWindow(feedbackWindow);
    showManagedDialogWindow(feedbackWindow,
                            std::make_unique<FeedbackDialog>(
                                audioProcessor.getFeedbackCollector(),
                                [safeThis](juce::AlertWindow::AlertIconType iconType,
                                           const juce::String& title,
                                           const juce::String& message)
                                {
                                    if (safeThis == nullptr || safeThis->isShuttingDown())
                                        return;

                                    safeThis->showStatusWindow(iconType, title, message, "editor_feedback_link_failure");
                                }),
                            "Feedback",
                            true,
                            500, 440, 800, 800,
                            "editor_feedback_dialog",
                            this);
}

void ChoroborosPluginEditor::showCrashReportWindow(const juce::String& crashReport)
{
    if (crashReport.isEmpty())
        return;

    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);
    closeManagedDialogWindow(feedbackWindow);
    showManagedDialogWindow(feedbackWindow,
                            std::make_unique<FeedbackDialog>(
                                crashReport,
                                audioProcessor.getFeedbackCollector(),
                                [safeThis](juce::AlertWindow::AlertIconType iconType,
                                           const juce::String& title,
                                           const juce::String& message)
                                {
                                    if (safeThis == nullptr || safeThis->isShuttingDown())
                                        return;

                                    safeThis->showStatusWindow(iconType, title, message, "editor_crash_report_link_failure");
                                }),
                            "Crash Report",
                            true,
                            500, 440, 800, 800,
                            "editor_crash_report_dialog",
                            this);
}

void ChoroborosPluginEditor::showStatusWindow(juce::AlertWindow::AlertIconType iconType,
                                              const juce::String& title,
                                              const juce::String& message,
                                              const juce::String& telemetryContext,
                                              juce::Component* anchorComponent)
{
    closeManagedDialogWindow(messageWindow);
    showManagedDialogWindow(messageWindow,
                            std::make_unique<MessageDialog>(title,
                                                            message,
                                                            iconType == juce::AlertWindow::WarningIcon),
                            title,
                            true,
                            420, 260, 860, 720,
                            telemetryContext,
                            anchorComponent != nullptr ? anchorComponent : this);
}

void ChoroborosPluginEditor::ensureDevPanelWindowCreated(bool triggeredByUser)
{
    if (shutdownFence.load())
        return;

    if (devWindow != nullptr)
        return;

    const double startMs = juce::Time::getMillisecondCounterHiRes();
    devWindow = std::make_unique<DevPanelWindow>(*this, audioProcessor);

    const juce::String eventName = triggeredByUser
        ? "editor_devpanel_create_on_demand_ms"
        : "editor_devpanel_prewarm_ms";
    audioProcessor.logLoadTraceEvent(eventName,
                                     juce::Time::getMillisecondCounterHiRes() - startMs);

    devPanelPrewarmComplete = true;
    choroboros::windows::applyPreferredRenderer(*devWindow,
                                                "editor_dev_panel_create",
                                                &audioProcessor);
}

void ChoroborosPluginEditor::scheduleDeferredDevPanelPrewarm()
{
    if (shutdownFence.load() || devPanelPrewarmScheduled || devPanelPrewarmComplete)
        return;

    devPanelPrewarmScheduled = true;
    postUiTaskAfterDelay(1500, [](ChoroborosPluginEditor& editor)
    {
        editor.ensureDevPanelWindowCreated(false);
    });
}

void ChoroborosPluginEditor::setupSliderAttachments()
{
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::RATE_ID, rateSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::DEPTH_ID, depthSlider);
    offsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::OFFSET_ID, offsetSlider);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::WIDTH_ID, widthSlider);
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::COLOR_ID, colorSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::MIX_ID, mixSlider);
}

void ChoroborosPluginEditor::setupSliderValueChangeListeners()
{
    rateSlider.onValueChange = [this] { updateValueLabel(rateValueLabel, rateSlider.getValue(), ChoroborosAudioProcessor::RATE_ID); };
    depthSlider.onValueChange = [this] { updateValueLabel(depthValueLabel, depthSlider.getValue(), ChoroborosAudioProcessor::DEPTH_ID); };
    offsetSlider.onValueChange = [this] { updateValueLabel(offsetValueLabel, offsetSlider.getValue(), ChoroborosAudioProcessor::OFFSET_ID); };
    widthSlider.onValueChange = [this] { updateValueLabel(widthValueLabel, widthSlider.getValue(), ChoroborosAudioProcessor::WIDTH_ID); };
    colorSlider.onValueChange = [this] { updateValueLabel(colorValueLabel, colorSlider.getValue(), ChoroborosAudioProcessor::COLOR_ID); };
    mixSlider.onValueChange = [this] { updateValueLabel(mixValueLabel, mixSlider.getValue(), ChoroborosAudioProcessor::MIX_ID); };
}

void ChoroborosPluginEditor::updateValueLabelColors(int colorIndex)
{
    juce::Colour valueTextColor;
    if (layoutTuning.valueTextColourMode != 0)
    {
        valueTextColor = juce::Colour(static_cast<juce::uint32>(layoutTuning.valueTextColour));
    }
    else
    {
        // Color values for each engine:
        // Green (0): #9dbd78
        // Blue (1): #7fb8ff
        // Red (2): #ff8d8b
        // Purple (3): #b88dd8
        // Black (4): #d4d4d4
        if (colorIndex == 0) // Green
            valueTextColor = juce::Colour(0xff9dbd78);
        else if (colorIndex == 1) // Blue
            valueTextColor = juce::Colour(0xff7fb8ff);
        else if (colorIndex == 2) // Red
            valueTextColor = juce::Colour(0xffff8d8b);
        else if (colorIndex == 3) // Purple
            valueTextColor = juce::Colour(0xffb88dd8);
        else // Black (colorIndex == 4)
            valueTextColor = juce::Colour(0xffd4d4d4);
    }

    const float alphaScale = static_cast<float>(juce::jlimit(0, 100, layoutTuning.valueTextAlphaPct)) * 0.01f;
    valueTextColor = valueTextColor.withMultipliedAlpha(alphaScale);
    
    // Update all value label text colors
    rateValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    depthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    offsetValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    widthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    colorValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    mixValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    
    // Also update editor text colors (for when editing)
    rateValueLabel.setEditorTextColor(valueTextColor);
    depthValueLabel.setEditorTextColor(valueTextColor);
    offsetValueLabel.setEditorTextColor(valueTextColor);
    widthValueLabel.setEditorTextColor(valueTextColor);
    colorValueLabel.setEditorTextColor(valueTextColor);
    mixValueLabel.setEditorTextColor(valueTextColor);
    
    // Repaint all value labels to show new color
    rateValueLabel.repaint();
    depthValueLabel.repaint();
    offsetValueLabel.repaint();
    widthValueLabel.repaint();
    colorValueLabel.repaint();
    mixValueLabel.repaint();
}

void ChoroborosPluginEditor::applyEngineVisual(const choroboros::CustomEngineVisual& visual)
{
    int fallbackColorIndex = juce::jlimit(0, 4, audioProcessor.getCurrentEngineColorIndex());
    if (visual.backgroundSet >= 0 && visual.backgroundSet < 5)
        fallbackColorIndex = visual.backgroundSet;

    customLookAndFeel.setColorTheme(fallbackColorIndex);
    loadBackgroundImage(fallbackColorIndex);

    juce::Colour valueTextColor = visual.valueTextColour;
    const float alphaScale = static_cast<float>(juce::jlimit(0, 100, layoutTuning.valueTextAlphaPct)) * 0.01f;
    valueTextColor = valueTextColor.withMultipliedAlpha(alphaScale);

    rateValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    depthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    offsetValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    widthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    colorValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    mixValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    rateValueLabel.setEditorTextColor(valueTextColor);
    depthValueLabel.setEditorTextColor(valueTextColor);
    offsetValueLabel.setEditorTextColor(valueTextColor);
    widthValueLabel.setEditorTextColor(valueTextColor);
    colorValueLabel.setEditorTextColor(valueTextColor);
    mixValueLabel.setEditorTextColor(valueTextColor);

    topBarDrawer.setAccentColour(visual.accentColour);
    if (topHeaderBar_)
        topHeaderBar_->setAccentColour(visual.accentColour);

    PluginEditorSetup::applyLayout(*this, layoutTuning);
}

void ChoroborosPluginEditor::loadBackgroundImage(int colorIndex)
{
    const auto pack = getOrDecodeBackgroundAssetPack(colorIndex);
    backgroundImage = pack.off;
    backgroundImageLit = pack.lit;
    invalidateHQLitOverlayCache();
}

void ChoroborosPluginEditor::applyCurrentEngineVisual()
{
    auto* mgr = audioProcessor.customEngineManager.get();
    if (mgr == nullptr)
    {
        const int colorIndex = audioProcessor.getCurrentEngineColorIndex();
        engineColorBox.setSelectedId(colorIndex + 1, juce::dontSendNotification);
        updateValueLabelColors(colorIndex);
        customLookAndFeel.setColorTheme(colorIndex);
        loadBackgroundImage(colorIndex);
        PluginEditorSetup::applyLayout(*this, layoutTuning);
        return;
    }

    const auto& engines = mgr->getEngines();
    const choroboros::CustomEngine* activeEngine = nullptr;
    int comboId = 1;

    if (audioProcessor.hasActiveCustomEngine())
    {
        const auto activeId = audioProcessor.getActiveCustomEngineId();
        for (int i = 0; i < static_cast<int>(engines.size()); ++i)
        {
            if (engines[static_cast<size_t>(i)].id == activeId)
            {
                activeEngine = &engines[static_cast<size_t>(i)];
                comboId = i + 1;
                break;
            }
        }
    }

    if (activeEngine == nullptr)
    {
        const int colorIndex = audioProcessor.getCurrentEngineColorIndex();
        activeEngine = mgr->getFactoryEngine(colorIndex);
        comboId = colorIndex + 1;
    }

    if (activeEngine == nullptr)
        return;

    engineColorBox.setSelectedId(comboId, juce::dontSendNotification);
    applyEngineVisual(activeEngine->visual);
    refreshEditorSurfaceAfterEngineVisualChange();
}

bool ChoroborosPluginEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
        if (file.endsWithIgnoreCase(".kzn"))
            return true;
    return false;
}

void ChoroborosPluginEditor::filesDropped(const juce::StringArray& files, int, int)
{
    juce::ignoreUnused(files);

    juce::File droppedFile;
    for (const auto& path : files)
    {
        if (path.endsWithIgnoreCase(".kzn"))
        {
            droppedFile = juce::File(path);
            break;
        }
    }

    if (!droppedFile.existsAsFile())
        return;

    const auto result = choroboros::importKzn(audioProcessor, droppedFile);
    if (result.success)
    {
        if (result.type == "engine")
        {
            rebuildEngineSelectorItems();
            applyCurrentEngineVisual();
        }

        juce::String message = "Imported " + result.type + ": " + result.presetName;
        if (result.warningMessage.isNotEmpty())
            message << "\n\nWarning:\n" << result.warningMessage;

        showStatusWindow(juce::AlertWindow::InfoIcon,
                         "KZN Import Successful",
                         message,
                         "editor_kzn_import_success");
        return;
    }

    showStatusWindow(juce::AlertWindow::WarningIcon,
                     "KZN Import Failed",
                     result.errorMessage,
                     "editor_kzn_import_failure");
}

int ChoroborosPluginEditor::calculateLabelWidth(const juce::String& text, const juce::Font& font) const
{
    // Calculate text width and add 16px total (8px padding on each side)
    // Use GlyphArrangement for accurate text width (recommended approach)
    float textWidth = juce::GlyphArrangement::getStringWidth(font, text);
    return static_cast<int>(std::ceil(textWidth)) + 16;
}

void ChoroborosPluginEditor::setupSlider(juce::Slider& slider, LabelWithContainer& label, LabelWithContainer& valueLabel,
                                         const juce::String& name, const juce::String& paramId)
{
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
    addAndMakeVisible(valueLabel);
    
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    const juce::Font font = makeUiTextFont(14.0f * getUiScale(), true);
    label.setFont(font);
    
    // Set tooltips based on parameter
    if (paramId == ChoroborosAudioProcessor::RATE_ID)
        slider.setTooltip("LFO Speed: Controls the modulation rate from 0.01 Hz (slow, lush) to 20 Hz (fast, vibrato). Lower values create classic chorus, higher values add movement.");
    else if (paramId == ChoroborosAudioProcessor::DEPTH_ID)
        slider.setTooltip("Modulation Depth: Controls how much the delay time is modulated. 0% = no effect, 100% = maximum modulation. Engine-specific scaling applied.");
    else if (paramId == ChoroborosAudioProcessor::OFFSET_ID)
        slider.setTooltip("LFO Phase Offset: Shifts the modulation phase from 0 to 180 degrees. Useful for stereo width and avoiding phase cancellation.");
    else if (paramId == ChoroborosAudioProcessor::WIDTH_ID)
        slider.setTooltip("Stereo Width: Controls the stereo spread from 0% (mono) to 200% (wide). Adjusts the phase relationship between left and right channels.");
    else if (paramId == ChoroborosAudioProcessor::COLOR_ID)
        slider.setTooltip("Tone/Character: Engine-specific parameter. Green=bloom (wet body/softness), Blue=focus (wet clarity/presence), Red NQ=post-chorus drive, Red HQ=tape tone (brighter as Color rises) + record drive, Purple=warp/orbit shape, Black=modulation intensity/ensemble spread.");
    else if (paramId == ChoroborosAudioProcessor::MIX_ID)
        slider.setTooltip("Dry/Wet Mix: Blends the original signal (0%) with the processed signal (100%). 50% = equal blend.");

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(paramId))
    {
        if (auto* rangedParameter = dynamic_cast<juce::RangedAudioParameter*>(parameter))
        {
            const double defaultValue = static_cast<double>(rangedParameter->convertFrom0to1(rangedParameter->getDefaultValue()));
            slider.setDoubleClickReturnValue(true, defaultValue);
        }
    }

    if (paramId == ChoroborosAudioProcessor::RATE_ID)
    {
        if (auto* smoothedSlider = dynamic_cast<SmoothedSlider*>(&slider))
        {
            smoothedSlider->onMouseUpCallback = [this, &slider](const juce::MouseEvent& e)
            {
                if (e.mods.isPopupMenu())
                    showRateSyncMenu(slider);
            };
        }
    }
    
    // Position labels above knobs/sliders (will be set per control in constructor)
}

double ChoroborosPluginEditor::getHostBpm() const
{
    if (auto* playHead = audioProcessor.getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
                return *bpm;
        }
    }
    return 120.0;
}

void ChoroborosPluginEditor::showRateSyncMenu(juce::Slider& rateControl)
{
    rateSyncOverlay_.reset();

    const double bpm = getHostBpm();
    if (bpm <= 0.0)
        return;

    const double mappedCurrent = static_cast<double>(audioProcessor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, static_cast<float>(rateControl.getValue())));
    const double mappedFromMin = static_cast<double>(audioProcessor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, static_cast<float>(rateControl.getMinimum())));
    const double mappedFromMax = static_cast<double>(audioProcessor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, static_cast<float>(rateControl.getMaximum())));
    const double mappedMin = juce::jmin(mappedFromMin, mappedFromMax);
    const double mappedMax = juce::jmax(mappedFromMin, mappedFromMax);
    constexpr double maxQuantizedRateHz = 20.0;
    const double quantizedMappedMax = juce::jmin(mappedMax, maxQuantizedRateHz);

    rateSyncOverlay_ = std::make_unique<RateSyncOverlay>();
    rateSyncOverlay_->configure(bpm, mappedCurrent, mappedMin, quantizedMappedMax);
    rateSyncOverlay_->setAnchorPoint(rateControl.getBoundsInParent().getCentre());

    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);

    rateSyncOverlay_->onRateSelected = [safeThis, mappedMin, quantizedMappedMax](double targetHz)
    {
        if (safeThis == nullptr || targetHz <= 0.0)
            return;

        const double clampedMapped = juce::jlimit(mappedMin, quantizedMappedMax, targetHz);
        double lo = safeThis->rateSlider.getMinimum();
        double hi = safeThis->rateSlider.getMaximum();
        for (int i = 0; i < 30; ++i)
        {
            const double mid = 0.5 * (lo + hi);
            const double mappedMid = static_cast<double>(safeThis->audioProcessor.mapParameterValue(
                ChoroborosAudioProcessor::RATE_ID, static_cast<float>(mid)));
            if (mappedMid < clampedMapped)
                lo = mid;
            else
                hi = mid;
        }
        const double raw = juce::jlimit(safeThis->rateSlider.getMinimum(),
                                        safeThis->rateSlider.getMaximum(), 0.5 * (lo + hi));
        safeThis->rateSlider.setValue(raw, juce::sendNotificationSync);
    };

    rateSyncOverlay_->onDismiss = [safeThis]()
    {
        if (safeThis != nullptr)
            safeThis->rateSyncOverlay_.reset();
    };

    addAndMakeVisible(*rateSyncOverlay_);
    rateSyncOverlay_->setBounds(getLocalBounds());
    rateSyncOverlay_->grabKeyboardFocus();
}

void ChoroborosPluginEditor::updateValueLabel(LabelWithContainer& label, float value, const juce::String& paramId)
{
    const float mappedValue = audioProcessor.mapParameterValue(paramId, value);
    juce::String text;
    
    if (paramId == ChoroborosAudioProcessor::RATE_ID)
    {
        // Rate: < 1.0 Hz: 2 decimals, >= 1.0 Hz: 1 decimal
        if (mappedValue < 1.0f)
            text = juce::String(mappedValue, 2) + " Hz";
        else
            text = juce::String(mappedValue, 1) + " Hz";
    }
    else if (paramId == ChoroborosAudioProcessor::DEPTH_ID)
    {
        text = juce::String(static_cast<int>(mappedValue * 100.0f)) + "%";
    }
    else if (paramId == ChoroborosAudioProcessor::OFFSET_ID)
    {
        text = juce::String(static_cast<int>(mappedValue)) + "°";
    }
    else if (paramId == ChoroborosAudioProcessor::WIDTH_ID)
    {
        text = juce::String(static_cast<int>(mappedValue * 100.0f)) + "%";
    }
    else if (paramId == ChoroborosAudioProcessor::COLOR_ID)
    {
        text = juce::String(static_cast<int>(mappedValue * 100.0f)) + "%";
    }
    else if (paramId == ChoroborosAudioProcessor::MIX_ID)
    {
        text = juce::String(static_cast<int>(mappedValue * 100.0f)) + "%";
    }
    
    label.setAnimatedValueText(text);
}

void ChoroborosPluginEditor::setupValueLabelEditing(LabelWithContainer& label, juce::Slider& slider, const juce::String& paramId)
{
    label.onValueEdited = [this, &slider, &label, paramId](const juce::String& newText) -> bool
    {
        const float parsedMappedValue = parseValueFromText(newText, paramId);
        if (parsedMappedValue >= 0.0f)  // Valid value
        {
            // Parse/edit values are in mapped display space; clamp in mapped space first.
            const float mappedFromMin = audioProcessor.mapParameterValue(paramId, static_cast<float>(slider.getMinimum()));
            const float mappedFromMax = audioProcessor.mapParameterValue(paramId, static_cast<float>(slider.getMaximum()));
            const float mappedMin = juce::jmin(mappedFromMin, mappedFromMax);
            const float mappedMax = juce::jmax(mappedFromMin, mappedFromMax);
            const float clampedMappedValue = juce::jlimit(mappedMin, mappedMax, parsedMappedValue);

            // Convert mapped display value back to raw parameter value.
            const float rawValue = audioProcessor.unmapParameterValue(paramId, clampedMappedValue);
            const float clampedRawValue = static_cast<float>(juce::jlimit(slider.getMinimum(), slider.getMaximum(),
                                                                           static_cast<double>(rawValue)));

            auto* param = audioProcessor.getValueTreeState().getParameter(paramId);
            if (param != nullptr)
            {
                // Convert to normalized 0-1 range
                float normalizedValue = param->convertTo0to1(clampedRawValue);
                // Clamp normalized value to valid range
                normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);
                param->setValueNotifyingHost(normalizedValue);
            }
            else
            {
                // Parameter not found - fallback to slider update
                slider.setValue(clampedRawValue, juce::sendNotificationSync);
            }
            
            // Also set slider value to update visual position
            slider.setValue(clampedRawValue, juce::dontSendNotification);
            
            // Format and set the label text - this will be picked up by editorAboutToBeHidden
            updateValueLabel(label, clampedRawValue, paramId);
            label.repaint();
            repaint();
            return true;  // Value was applied successfully
        }
        else
        {
            // Invalid value - restore previous value
            updateValueLabel(label, slider.getValue(), paramId);
            label.repaint();
            repaint();
            return false;  // Value was not applied
        }
    };
}

float ChoroborosPluginEditor::parseValueFromText(const juce::String& text, const juce::String& paramId)
{
    const juce::String trimmed = text.trim();
    
    if (paramId == ChoroborosAudioProcessor::RATE_ID)
        return parseRateValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::DEPTH_ID)
        return parseDepthValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::OFFSET_ID)
        return parseOffsetValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::WIDTH_ID)
        return parseWidthValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::COLOR_ID)
        return parseColorValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::MIX_ID)
        return parseMixValue(trimmed);
    
    return -1.0f; // Invalid
}

float ChoroborosPluginEditor::parseRateValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("Hz").trim();
    const float value = clean.getFloatValue();
    if (value > 0.0f && std::isfinite(value))
        return value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseDepthValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && std::isfinite(value))
        return (value >= 1.0f && value <= 100.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseOffsetValue(const juce::String& trimmed)
{
    juce::String clean = trimmed;
    if (clean.endsWithIgnoreCase("deg"))
        clean = clean.substring(0, clean.length() - 3).trim();
    else if (clean.endsWith("°"))
        clean = clean.dropLastCharacters(1).trim();
    const float value = clean.getFloatValue();
    if (std::isfinite(value))
        return value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseWidthValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && std::isfinite(value))
    {
        // Display shows 0–200%, so always interpret typed values as percentages.
        // Typing "100" → 1.0, "50" → 0.5, "200" → 2.0, "1" → 0.01.
        return juce::jlimit(0.0f, 2.0f, value / 100.0f);
    }
    return -1.0f;
}

float ChoroborosPluginEditor::parseColorValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && std::isfinite(value))
        return (value >= 1.0f && value <= 100.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseMixValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && std::isfinite(value))
        return (value >= 1.0f && value <= 100.0f) ? (value / 100.0f) : value;
    return -1.0f;
}
