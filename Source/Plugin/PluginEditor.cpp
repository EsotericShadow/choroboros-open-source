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
#include "../UI/LabelWithContainer.h"
#include "BinaryData.h"
#include "FeedbackDialog.h"
#include "AboutDialog.h"
#include "../UI/PluginEditorSetup.h"
#include "../UI/DevPanel.h"
#include <cmath>
#include <vector>

namespace
{
int uiScaleInt(int value)
{
    return juce::roundToInt(static_cast<float>(value) * ChoroborosPluginEditor::kUiScale);
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

void loadPersistedLayoutDefaults(LayoutTuning& layout)
{
    const auto json = DefaultsPersistence::load();
    if (json.isEmpty())
        return;

    const auto parsed = juce::JSON::parse(json);
    if (parsed.isVoid())
        return;

    const auto* root = parsed.getDynamicObject();
    if (root == nullptr || !root->hasProperty("layout"))
        return;

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
    layout.mixKnobYOffset = getIntOrDefault(layoutVar, "mixKnobYOffset", layout.mixKnobYOffset);
    layout.mixKnobYOffsetGreen = getIntOrDefault(layoutVar, "mixKnobYOffsetGreen", layout.mixKnobYOffset);
    layout.mixKnobYOffsetBlue = getIntOrDefault(layoutVar, "mixKnobYOffsetBlue", layout.mixKnobYOffset);
    layout.mixKnobYOffsetRed = getIntOrDefault(layoutVar, "mixKnobYOffsetRed", layout.mixKnobYOffset);
    layout.mixKnobYOffsetPurple = getIntOrDefault(layoutVar, "mixKnobYOffsetPurple", layout.mixKnobYOffset);
    layout.mixKnobYOffsetBlack = getIntOrDefault(layoutVar, "mixKnobYOffsetBlack", layout.mixKnobYOffset);
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
    layout.mixValueOffsetX = getIntOrDefault(layoutVar, "mixValueOffsetX", layout.mixValueOffsetX);
    layout.mixValueOffsetXGreen = getIntOrDefault(layoutVar, "mixValueOffsetXGreen", layout.mixValueOffsetX);
    layout.mixValueOffsetXBlue = getIntOrDefault(layoutVar, "mixValueOffsetXBlue", layout.mixValueOffsetX);
    layout.mixValueOffsetXRed = getIntOrDefault(layoutVar, "mixValueOffsetXRed", layout.mixValueOffsetX);
    layout.mixValueOffsetXPurple = getIntOrDefault(layoutVar, "mixValueOffsetXPurple", layout.mixValueOffsetX);
    layout.mixValueOffsetXBlack = getIntOrDefault(layoutVar, "mixValueOffsetXBlack", layout.mixValueOffsetX);
    layout.rateValueOffsetY = getIntOrDefault(layoutVar, "rateValueOffsetY", layout.rateValueOffsetY);
    layout.depthValueOffsetY = getIntOrDefault(layoutVar, "depthValueOffsetY", layout.depthValueOffsetY);
    layout.offsetValueOffsetY = getIntOrDefault(layoutVar, "offsetValueOffsetY", layout.offsetValueOffsetY);
    layout.widthValueOffsetY = getIntOrDefault(layoutVar, "widthValueOffsetY", layout.widthValueOffsetY);
    layout.knobValueFontSize = getIntOrDefault(layoutVar, "knobValueFontSize", layout.knobValueFontSize);
    layout.colorValueFontSize = getIntOrDefault(layoutVar, "colorValueFontSize", layout.colorValueFontSize);
    layout.mixValueFontSize = getIntOrDefault(layoutVar, "mixValueFontSize", layout.mixValueFontSize);
    layout.valueTextAlphaPct = getIntOrDefault(layoutVar, "valueTextAlphaPct", layout.valueTextAlphaPct);
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
    layout.rateKnobVisualResponseMs = getIntOrDefault(layoutVar, "rateKnobVisualResponseMs", layout.rateKnobVisualResponseMs);
    layout.depthKnobVisualResponseMs = getIntOrDefault(layoutVar, "depthKnobVisualResponseMs", layout.depthKnobVisualResponseMs);
    layout.offsetKnobVisualResponseMs = getIntOrDefault(layoutVar, "offsetKnobVisualResponseMs", layout.offsetKnobVisualResponseMs);
    layout.widthKnobVisualResponseMs = getIntOrDefault(layoutVar, "widthKnobVisualResponseMs", layout.widthKnobVisualResponseMs);
    layout.mixKnobVisualResponseMs = getIntOrDefault(layoutVar, "mixKnobVisualResponseMs", layout.mixKnobVisualResponseMs);
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
}

class DevPanelWindow : public juce::DocumentWindow
{
public:
    DevPanelWindow(ChoroborosPluginEditor& editor, ChoroborosAudioProcessor& processor)
        : juce::DocumentWindow("Choroboros Dev Panel",
                               juce::Colour(0xff202020),
                               juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setAlwaysOnTop(true);
        setContentOwned(new DevPanel(editor, processor), true);
        centreAroundComponent(&editor, 900, 700);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};
} // namespace

//==============================================================================
ChoroborosPluginEditor::ChoroborosPluginEditor (ChoroborosAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    loadValueLabelTypeface();
    loadUiTextTypeface();
    customLookAndFeel.setUiTextTypeface(uiTextTypeface);
    layoutTuning = PluginEditorSetup::makeDefaultLayout();
    loadPersistedLayoutDefaults(layoutTuning);
    
    setupEngineColorSelector();
    // Note: setupEngineColorSelector now reads the saved parameter value and updates UI
    
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
    
    PluginEditorSetup::setupHQButton(*this);
    hqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::HQ_ID, hqButton);
    
    PluginEditorSetup::setupValueLabels(*this);
    PluginEditorSetup::setupLabels(*this);
    setupSliderValueChangeListeners();
    
    // Update value label colors based on saved engine color (after all labels are set up)
    auto engineColorParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::ENGINE_COLOR_ID);
    if (engineColorParam)
    {
        const int savedColorIndex = static_cast<int>(engineColorParam->load());
        updateValueLabelColors(savedColorIndex);
    }
    else
    {
        updateValueLabelColors(0);  // Default to Green if no parameter
    }
    
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
    
    // Setup feedback button (beta version)
    feedbackButton.setButtonText("Feedback");
    feedbackButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    feedbackButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    feedbackButton.onClick = [this] {
        if (audioProcessor.feedbackCollector)
        {
            FeedbackDialog::show(*audioProcessor.feedbackCollector);
        }
    };
    feedbackButton.setTooltip("Send Feedback: Share your thoughts, bug reports, or feature requests. Usage statistics included automatically.");
    addAndMakeVisible(feedbackButton);
    feedbackButton.setBounds(uiScaleInt(645), uiScaleInt(5), uiScaleInt(45), uiScaleInt(10)); // Top right, small button
    
    // Setup Help button
    helpButton.setButtonText("Help");
    helpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    helpButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    helpButton.onClick = [] {
        // Open help/documentation (for now, just open email - can be updated to PDF link later)
        juce::URL("mailto:info@kaizenstrategic.ai?subject=Choroboros%20Help").launchInDefaultBrowser();
    };
    helpButton.setTooltip("Help: Get documentation and information");
    addAndMakeVisible(helpButton);
    helpButton.setBounds(uiScaleInt(600), uiScaleInt(5), uiScaleInt(40), uiScaleInt(10));
    
    // Setup About button
    aboutButton.setButtonText("About");
    aboutButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    aboutButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    aboutButton.onClick = [] {
        AboutDialog::show();
    };
    aboutButton.setTooltip("About: View version information and company details");
    addAndMakeVisible(aboutButton);
    aboutButton.setBounds(uiScaleInt(555), uiScaleInt(5), uiScaleInt(40), uiScaleInt(10));

    // Setup Dev button
    devButton.setButtonText("DEV");
    devButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    devButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    devButton.onClick = [this]
    {
        if (!devWindow)
            devWindow = std::make_unique<DevPanelWindow>(*this, audioProcessor);

        const bool shouldShow = !devWindow->isVisible();
        devWindow->setVisible(shouldShow);
        if (shouldShow)
            devWindow->toFront(true);
    };
    addAndMakeVisible(devButton);
    devButton.setBounds(uiScaleInt(5), uiScaleInt(5), uiScaleInt(40), uiScaleInt(10));
    
    // Set fixed size
    setSize(uiScaleInt(700), uiScaleInt(363));
    applyLayout();
    setResizable(false, false);
}

void ChoroborosPluginEditor::loadValueLabelTypeface()
{
    if (BinaryData::Technology_ttfSize > 0)
    {
        valueLabelTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Technology_ttf,
            static_cast<size_t>(BinaryData::Technology_ttfSize));
    }
}

void ChoroborosPluginEditor::loadUiTextTypeface()
{
    if (BinaryData::Retroica_ttfSize > 0)
    {
        uiTextTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Retroica_ttf,
            static_cast<size_t>(BinaryData::Retroica_ttfSize));
    }
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
    setLookAndFeel(nullptr);
}

//==============================================================================
void ChoroborosPluginEditor::paint (juce::Graphics& g)
{
    // Draw background
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, 0, 0, getWidth(), getHeight(), 0, 0,
                   backgroundImage.getWidth(), backgroundImage.getHeight());
    }
    else
    {
        g.fillAll(juce::Colours::black);
    }
}

void ChoroborosPluginEditor::resized()
{
    // Fixed size, no resizing needed
}

void ChoroborosPluginEditor::applyLayout()
{
    PluginEditorSetup::applyLayout(*this, layoutTuning);
    repaint();
}

void ChoroborosPluginEditor::applyTuningToUI()
{
    const auto& tuning = audioProcessor.getTuningState();
    rateSlider.setSkewFactor(tuning.rate.uiSkew.load());
    depthSlider.setSkewFactor(tuning.depth.uiSkew.load());
    offsetSlider.setSkewFactor(tuning.offset.uiSkew.load());
    widthSlider.setSkewFactor(tuning.width.uiSkew.load());
    colorSlider.setSkewFactor(tuning.color.uiSkew.load());
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
    addAndMakeVisible(engineColorBox);
    
    engineColorBox.addItem("Green", 1);
    engineColorBox.addItem("Blue", 2);
    engineColorBox.addItem("Red", 3);
    engineColorBox.addItem("Purple", 4);
    engineColorBox.addItem("Black", 5);
    engineColorBox.setSelectedId(1);
    
    const int engineColorWidth = 80;
    const int engineColorHeight = 14;
    // Position at bottom left (scaled 50%)
    engineColorBox.setBounds(uiScaleInt(20), uiScaleInt(335), uiScaleInt(engineColorWidth), uiScaleInt(engineColorHeight));
    
    // Remove background color - make it transparent
    engineColorBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    engineColorBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    engineColorBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    
    engineColorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::ENGINE_COLOR_ID, engineColorBox);
    
    // Read the current parameter value and update UI to match (for persistence)
    // Do this AFTER attachment is created so ComboBox has the correct value
    auto engineColorParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::ENGINE_COLOR_ID);
    if (engineColorParam)
    {
        const int savedColorIndex = static_cast<int>(engineColorParam->load());
        // Use the ComboBox's selected ID to ensure consistency
        const int actualColorIndex = engineColorBox.getSelectedId() - 1;
        customLookAndFeel.setColorTheme(actualColorIndex);
        loadBackgroundImage(actualColorIndex);
        // Value labels will be updated after they're set up in constructor
    }
    
    engineColorBox.setTooltip("Engine Selection: Choose between five distinct chorus algorithms. Green=Classic, Blue=Modern, Red=Vintage, Purple=Experimental, Black=Linear.");
    
    engineColorBox.onChange = [this] {
        const int colorIndex = engineColorBox.getSelectedId() - 1;
        customLookAndFeel.setColorTheme(colorIndex);
        loadBackgroundImage(colorIndex);
        updateValueLabelColors(colorIndex);
        PluginEditorSetup::applyLayout(*this, layoutTuning);
        
        // Track engine switch for feedback
        if (audioProcessor.feedbackCollector)
        {
            auto hqParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::HQ_ID);
            bool hq = hqParam ? (hqParam->load() > 0.5f) : false;
            audioProcessor.feedbackCollector->trackEngineSwitch(colorIndex, hq);
        }
        
        // Force sliders to repaint with new thumb image
        rateSlider.repaint();
        depthSlider.repaint();
        offsetSlider.repaint();
        widthSlider.repaint();
        colorSlider.repaint();
        mixSlider.repaint();
        repaint();
    };
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
    // Color values for each engine:
    // Green (0): #9dbd78
    // Blue (1): #7fb8ff
    // Red (2): #ff8d8b
    // Purple (3): #b88dd8
    // Black (4): #d4d4d4
    juce::Colour valueTextColor;
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

    const float alpha = static_cast<float>(juce::jlimit(0, 100, layoutTuning.valueTextAlphaPct)) * 0.01f;
    valueTextColor = valueTextColor.withAlpha(alpha);
    
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

void ChoroborosPluginEditor::loadBackgroundImage(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    
    const char* bgName = nullptr;
    int bgSize = 0;
    
    if (colorIndex == 0) // Green
    {
        bgName = BinaryData::green_backpanel_png;
        bgSize = BinaryData::green_backpanel_pngSize;
    }
    else if (colorIndex == 1) // Blue
    {
        bgName = BinaryData::blue_backpanel_png;
        bgSize = BinaryData::blue_backpanel_pngSize;
    }
    else if (colorIndex == 2) // Red
    {
        bgName = BinaryData::red_backpanel_png;
        bgSize = BinaryData::red_backpanel_pngSize;
    }
    else if (colorIndex == 3) // Purple
    {
        bgName = BinaryData::purple_backpanel_png;
        bgSize = BinaryData::purple_backpanel_pngSize;
    }
    else // Black (colorIndex == 4)
    {
        bgName = BinaryData::black_backpanel_png;
        bgSize = BinaryData::black_backpanel_pngSize;
    }
    
    if (bgName && bgSize > 0)
        backgroundImage = juce::ImageCache::getFromMemory(bgName, bgSize);
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
        slider.setTooltip("LFO Phase Offset: Shifts the modulation phase from 0° to 180°. Useful for stereo width and avoiding phase cancellation.");
    else if (paramId == ChoroborosAudioProcessor::WIDTH_ID)
        slider.setTooltip("Stereo Width: Controls the stereo spread from 0% (mono) to 200% (wide). Adjusts the phase relationship between left and right channels.");
    else if (paramId == ChoroborosAudioProcessor::COLOR_ID)
        slider.setTooltip("Tone/Character: Engine-specific parameter. Green=feedback, Blue=filter, Red=saturation, Purple=warp amount, Black=ensemble spread/complexity (HQ) and modulation intensity (Normal).");
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
    const double bpm = getHostBpm();
    if (bpm <= 0.0)
        return;

    const double mappedCurrent = static_cast<double>(audioProcessor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, static_cast<float>(rateControl.getValue())));
    const double mappedFromMin = static_cast<double>(audioProcessor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, static_cast<float>(rateControl.getMinimum())));
    const double mappedFromMax = static_cast<double>(audioProcessor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, static_cast<float>(rateControl.getMaximum())));
    const double mappedMin = juce::jmin(mappedFromMin, mappedFromMax);
    const double mappedMax = juce::jmax(mappedFromMin, mappedFromMax);

    juce::PopupMenu menu;
    menu.addSectionHeader("Rate Sync @ " + juce::String(bpm, 2) + " BPM");

    int nextId = 1000;
    std::vector<std::pair<int, double>> idToHz;

    const auto addRateItem = [&](juce::PopupMenu& targetMenu, const juce::String& label, double beatsPerCycle)
    {
        if (beatsPerCycle <= 0.0)
            return;
        const double hz = bpm / (60.0 * beatsPerCycle);
        const bool inRange = (hz >= mappedMin && hz <= mappedMax);
        const bool isTicked = std::abs(hz - mappedCurrent) <= 0.01;
        targetMenu.addItem(nextId, label + " (" + juce::String(hz, 2) + " Hz)", inRange, isTicked);
        if (inRange)
            idToHz.emplace_back(nextId, hz);
        ++nextId;
    };

    const std::vector<int> denominators { 64, 32, 16, 8, 4, 3, 2, 1 };

    juce::PopupMenu straightMenu;
    for (int denom : denominators)
        addRateItem(straightMenu, "1/" + juce::String(denom), 4.0 / static_cast<double>(denom));
    menu.addSubMenu("Straight", straightMenu);

    juce::PopupMenu tripletMenu;
    for (int denom : denominators)
        addRateItem(tripletMenu, "1/" + juce::String(denom) + "T", (4.0 / static_cast<double>(denom)) * (2.0 / 3.0));
    menu.addSubMenu("Triplet", tripletMenu);

    juce::PopupMenu dottedMenu;
    for (int denom : denominators)
        addRateItem(dottedMenu, "1/" + juce::String(denom) + ".", (4.0 / static_cast<double>(denom)) * 1.5);
    menu.addSubMenu("Dotted", dottedMenu);

    juce::PopupMenu swingMenu;
    const std::vector<int> swingPercents { 54, 58, 62, 66, 70 };
    for (int baseDenom : { 8, 16 })
    {
        juce::PopupMenu baseSwingMenu;
        const double pairBeats = 8.0 / static_cast<double>(baseDenom);
        for (int swingPct : swingPercents)
        {
            const double longBeats = pairBeats * (static_cast<double>(swingPct) / 100.0);
            const double shortBeats = pairBeats - longBeats;
            addRateItem(baseSwingMenu, "1/" + juce::String(baseDenom) + " Swing " + juce::String(swingPct) + "% Long", longBeats);
            addRateItem(baseSwingMenu, "1/" + juce::String(baseDenom) + " Swing " + juce::String(swingPct) + "% Short", shortBeats);
        }
        swingMenu.addSubMenu("1/" + juce::String(baseDenom) + " Swing", baseSwingMenu);
    }
    menu.addSubMenu("Swing", swingMenu);

    juce::Component::SafePointer<ChoroborosPluginEditor> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&rateControl),
                       [safeThis, idToHz, mappedMin, mappedMax](int selectedId)
                       {
                           if (safeThis == nullptr || selectedId == 0)
                               return;

                           double targetHz = -1.0;
                           for (const auto& entry : idToHz)
                           {
                               if (entry.first == selectedId)
                               {
                                   targetHz = entry.second;
                                   break;
                               }
                           }

                           if (targetHz <= 0.0)
                               return;

                           const auto toRawRate = [&](double desiredMappedHz) -> double
                           {
                               const double clampedMapped = juce::jlimit(mappedMin, mappedMax, desiredMappedHz);
                               double lo = safeThis->rateSlider.getMinimum();
                               double hi = safeThis->rateSlider.getMaximum();
                               for (int i = 0; i < 30; ++i)
                               {
                                   const double mid = 0.5 * (lo + hi);
                                   const double mappedMid = static_cast<double>(safeThis->audioProcessor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, static_cast<float>(mid)));
                                   if (mappedMid < clampedMapped)
                                       lo = mid;
                                   else
                                       hi = mid;
                               }
                               return juce::jlimit(safeThis->rateSlider.getMinimum(), safeThis->rateSlider.getMaximum(), 0.5 * (lo + hi));
                           };

                           safeThis->rateSlider.setValue(toRawRate(targetHz), juce::sendNotificationSync);
                       });
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
        float newValue = parseValueFromText(newText, paramId);
        if (newValue >= 0.0f)  // Valid value
        {
            // Clamp to slider range
            double minVal = slider.getMinimum();
            double maxVal = slider.getMaximum();
            float clampedValue = static_cast<float>(juce::jlimit(minVal, maxVal, static_cast<double>(newValue)));
            
            // Update the parameter directly via the value tree state to ensure it applies
            // This bypasses any potential attachment blocking
            auto* param = audioProcessor.getValueTreeState().getParameter(paramId);
            if (param != nullptr)
            {
                // Convert to normalized 0-1 range
                float normalizedValue = param->convertTo0to1(clampedValue);
                // Clamp normalized value to valid range
                normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);
                param->setValueNotifyingHost(normalizedValue);
            }
            else
            {
                // Parameter not found - fallback to slider update
                slider.setValue(clampedValue, juce::sendNotificationSync);
            }
            
            // Also set slider value to update visual position
            slider.setValue(clampedValue, juce::dontSendNotification);
            
            // Format and set the label text - this will be picked up by editorAboutToBeHidden
            updateValueLabel(label, clampedValue, paramId);
            
            return true;  // Value was applied successfully
        }
        else
        {
            // Invalid value - restore previous value
            updateValueLabel(label, slider.getValue(), paramId);
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
    if (value > 0.0f && value <= 10.0f)
        return value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseDepthValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 100.0f)
        return (value > 1.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseOffsetValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("°").trim();
    if (clean.endsWithIgnoreCase("deg"))
        clean = clean.substring(0, clean.length() - 3).trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 180.0f)
        return value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseWidthValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 200.0f)
        return (value > 2.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseColorValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 100.0f)
        return (value > 1.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseMixValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 100.0f)
        return (value > 1.0f) ? (value / 100.0f) : value;
    return -1.0f;
}
