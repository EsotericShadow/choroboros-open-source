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

#include "PluginEditorSetup.h"
#include "../Plugin/PluginEditor.h"

LayoutTuning PluginEditorSetup::makeDefaultLayout()
{
    return LayoutTuning{};
}

void PluginEditorSetup::applyLayout(ChoroborosPluginEditor& editor, const LayoutTuning& layout)
{
    const auto s = [&editor](int value) { return juce::roundToInt(static_cast<float>(value) * editor.getUiScale()); };
    int colorIndex = 0;
    if (auto* engineColorParam = editor.audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
        colorIndex = juce::jlimit(0, 4, static_cast<int>(engineColorParam->load()));

    const auto pickByColor = [colorIndex](int green, int blue, int red, int purple, int black, int fallback)
    {
        if (colorIndex == 0) return green;
        if (colorIndex == 1) return blue;
        if (colorIndex == 2) return red;
        if (colorIndex == 3) return purple;
        if (colorIndex == 4) return black;
        return fallback;
    };

    const int mainKnobSize = s(pickByColor(layout.mainKnobSizeGreen,
                                           layout.mainKnobSizeBlue,
                                           layout.mainKnobSizeRed,
                                           layout.mainKnobSizePurple,
                                           layout.mainKnobSizeBlack,
                                           layout.mainKnobSize));
    const int knobTopY = s(pickByColor(layout.knobTopYGreen, layout.knobTopYBlue, layout.knobTopYRed,
                                       layout.knobTopYPurple, layout.knobTopYBlack, layout.knobTopY));
    const int rateCenterX = s(pickByColor(layout.rateCenterXGreen, layout.rateCenterXBlue, layout.rateCenterXRed,
                                          layout.rateCenterXPurple, layout.rateCenterXBlack, layout.rateCenterX));
    const int depthCenterX = s(pickByColor(layout.depthCenterXGreen, layout.depthCenterXBlue, layout.depthCenterXRed,
                                           layout.depthCenterXPurple, layout.depthCenterXBlack, layout.depthCenterX));
    const int offsetCenterX = s(pickByColor(layout.offsetCenterXGreen, layout.offsetCenterXBlue, layout.offsetCenterXRed,
                                            layout.offsetCenterXPurple, layout.offsetCenterXBlack, layout.offsetCenterX));
    const int widthCenterX = s(pickByColor(layout.widthCenterXGreen, layout.widthCenterXBlue, layout.widthCenterXRed,
                                           layout.widthCenterXPurple, layout.widthCenterXBlack, layout.widthCenterX));
    const int trackStartX = s(pickByColor(layout.sliderTrackStartXGreen, layout.sliderTrackStartXBlue, layout.sliderTrackStartXRed,
                                          layout.sliderTrackStartXPurple, layout.sliderTrackStartXBlack, layout.sliderTrackStartX));
    const int trackStartY = s(pickByColor(layout.sliderTrackStartYGreen, layout.sliderTrackStartYBlue, layout.sliderTrackStartYRed,
                                          layout.sliderTrackStartYPurple, layout.sliderTrackStartYBlack, layout.sliderTrackStartY));
    const int trackEndX = s(pickByColor(layout.sliderTrackEndXGreen, layout.sliderTrackEndXBlue, layout.sliderTrackEndXRed,
                                        layout.sliderTrackEndXPurple, layout.sliderTrackEndXBlack, layout.sliderTrackEndX));
    const int trackEndY = s(pickByColor(layout.sliderTrackEndYGreen, layout.sliderTrackEndYBlue, layout.sliderTrackEndYRed,
                                        layout.sliderTrackEndYPurple, layout.sliderTrackEndYBlack, layout.sliderTrackEndY));
    const float sizeScale = pickByColor(layout.sliderSizeGreen, layout.sliderSizeBlue, layout.sliderSizeRed,
                                        layout.sliderSizePurple, layout.sliderSizeBlack, layout.sliderSize) / 100.0f;
    const int sliderH = juce::roundToInt(s(18) * sizeScale);
    const int sliderX = juce::jmin(trackStartX, trackEndX);
    const int sliderW = juce::jmax(1, std::abs(trackEndX - trackStartX));
    const int trackCenterY = (trackStartY + trackEndY) / 2;
    const int sliderY = trackCenterY - (sliderH / 2);
    const int mixKnobSize = s(pickByColor(layout.mixKnobSizeGreen,
                                          layout.mixKnobSizeBlue,
                                          layout.mixKnobSizeRed,
                                          layout.mixKnobSizePurple,
                                          layout.mixKnobSizeBlack,
                                          layout.mixKnobSize));
    const int mixCenterX = s(pickByColor(layout.mixCenterXGreen, layout.mixCenterXBlue, layout.mixCenterXRed,
                                         layout.mixCenterXPurple, layout.mixCenterXBlack, layout.mixCenterX));
    const int valueLabelWidth = s(layout.valueLabelWidth);
    const int valueLabelHeight = s(layout.valueLabelHeight);
    const int valueLabelY = s(pickByColor(layout.valueLabelYGreen, layout.valueLabelYBlue, layout.valueLabelYRed,
                                          layout.valueLabelYPurple, layout.valueLabelYBlack, layout.valueLabelY));
    const int rateValueOffsetX = s(pickByColor(layout.rateValueOffsetXGreen, layout.rateValueOffsetXBlue, layout.rateValueOffsetXRed,
                                               layout.rateValueOffsetXPurple, layout.rateValueOffsetXBlack, layout.rateValueOffsetX));
    const int depthValueOffsetX = s(pickByColor(layout.depthValueOffsetXGreen, layout.depthValueOffsetXBlue, layout.depthValueOffsetXRed,
                                                layout.depthValueOffsetXPurple, layout.depthValueOffsetXBlack, layout.depthValueOffsetX));
    const int offsetValueOffsetX = s(pickByColor(layout.offsetValueOffsetXGreen, layout.offsetValueOffsetXBlue, layout.offsetValueOffsetXRed,
                                                 layout.offsetValueOffsetXPurple, layout.offsetValueOffsetXBlack, layout.offsetValueOffsetX));
    const int widthValueOffsetX = s(pickByColor(layout.widthValueOffsetXGreen, layout.widthValueOffsetXBlue, layout.widthValueOffsetXRed,
                                                layout.widthValueOffsetXPurple, layout.widthValueOffsetXBlack, layout.widthValueOffsetX));
    const int rateValueOffsetY = s(layout.rateValueOffsetY);
    const int depthValueOffsetY = s(layout.depthValueOffsetY);
    const int offsetValueOffsetY = s(layout.offsetValueOffsetY);
    const int widthValueOffsetY = s(layout.widthValueOffsetY);
    const int colorValueWidth = s(layout.colorValueWidth);
    const int colorValueHeight = s(layout.colorValueHeight);
    const int colorValueY = s(pickByColor(layout.colorValueYGreen, layout.colorValueYBlue, layout.colorValueYRed,
                                          layout.colorValueYPurple, layout.colorValueYBlack, layout.colorValueY));
    const int colorValueXOffset = s(pickByColor(layout.colorValueXOffsetGreen, layout.colorValueXOffsetBlue, layout.colorValueXOffsetRed,
                                                layout.colorValueXOffsetPurple, layout.colorValueXOffsetBlack, layout.colorValueXOffset));
    const int mixValueWidth = s(layout.mixValueWidth);
    const int mixValueHeight = s(layout.mixValueHeight);
    const int mixValueY = s(pickByColor(layout.mixValueYGreen, layout.mixValueYBlue, layout.mixValueYRed,
                                        layout.mixValueYPurple, layout.mixValueYBlack, layout.mixValueY));
    const int mixValueOffsetX = s(pickByColor(layout.mixValueOffsetXGreen, layout.mixValueOffsetXBlue, layout.mixValueOffsetXRed,
                                              layout.mixValueOffsetXPurple, layout.mixValueOffsetXBlack, layout.mixValueOffsetX));
    const auto makeColour = [](int argb) { return juce::Colour(static_cast<juce::uint32>(argb)); };

    editor.rateSlider.setBounds(rateCenterX - (mainKnobSize / 2), knobTopY, mainKnobSize, mainKnobSize);
    editor.depthSlider.setBounds(depthCenterX - (mainKnobSize / 2), knobTopY, mainKnobSize, mainKnobSize);
    editor.offsetSlider.setBounds(offsetCenterX - (mainKnobSize / 2), knobTopY, mainKnobSize, mainKnobSize);
    editor.widthSlider.setBounds(widthCenterX - (mainKnobSize / 2), knobTopY, mainKnobSize, mainKnobSize);

    // Purple knob shadow extends below bounds; allow drawing overflow so it isn't clipped
    const bool allowKnobOverflow = (colorIndex == 3);
    editor.rateSlider.setPaintingIsUnclipped(allowKnobOverflow);
    editor.depthSlider.setPaintingIsUnclipped(allowKnobOverflow);
    editor.offsetSlider.setPaintingIsUnclipped(allowKnobOverflow);
    editor.widthSlider.setPaintingIsUnclipped(allowKnobOverflow);

    editor.colorSlider.setBounds(sliderX, sliderY, sliderW, sliderH);

    const int mixKnobX = mixCenterX - (mixKnobSize / 2);
    const int mixKnobY = s(pickByColor(layout.mixKnobYGreen, layout.mixKnobYBlue, layout.mixKnobYRed,
                                       layout.mixKnobYPurple, layout.mixKnobYBlack, layout.mixKnobY));
    editor.mixSlider.setBounds(mixKnobX, mixKnobY, mixKnobSize, mixKnobSize);

    const juce::Font labelFont = editor.makeUiTextFont(12.25f * editor.getUiScale(), true);
    int rateLabelWidth = editor.calculateLabelWidth("RATE", labelFont);
    int depthLabelWidth = editor.calculateLabelWidth("DEPTH", labelFont);
    int offsetLabelWidth = editor.calculateLabelWidth("OFFSET", labelFont);
    int widthLabelWidth = editor.calculateLabelWidth("WIDTH", labelFont);
    int colorLabelWidth = editor.calculateLabelWidth("COLOR", labelFont);
    int mixLabelWidth = editor.calculateLabelWidth("MIX", labelFont);

    const int knobLabelY = knobTopY - s(15);
    const int labelHeight = s(20);
    editor.rateLabel.setBounds(rateCenterX - (rateLabelWidth / 2), knobLabelY, rateLabelWidth, labelHeight);
    editor.depthLabel.setBounds(depthCenterX - (depthLabelWidth / 2), knobLabelY, depthLabelWidth, labelHeight);
    editor.offsetLabel.setBounds(offsetCenterX - (offsetLabelWidth / 2), knobLabelY, offsetLabelWidth, labelHeight);
    editor.widthLabel.setBounds(widthCenterX - (widthLabelWidth / 2), knobLabelY, widthLabelWidth, labelHeight);

    editor.rateValueLabel.setBounds(rateCenterX - (valueLabelWidth / 2) + rateValueOffsetX,
                                    valueLabelY + rateValueOffsetY, valueLabelWidth, valueLabelHeight);
    editor.depthValueLabel.setBounds(depthCenterX - (valueLabelWidth / 2) + depthValueOffsetX,
                                     valueLabelY + depthValueOffsetY, valueLabelWidth, valueLabelHeight);
    editor.offsetValueLabel.setBounds(offsetCenterX - (valueLabelWidth / 2) + offsetValueOffsetX,
                                      valueLabelY + offsetValueOffsetY, valueLabelWidth, valueLabelHeight);
    editor.widthValueLabel.setBounds(widthCenterX - (valueLabelWidth / 2) + widthValueOffsetX,
                                     valueLabelY + widthValueOffsetY, valueLabelWidth, valueLabelHeight);

    const int colorValueCenterX = s(layout.colorValueCenterX);
    const int colorValueX = colorValueCenterX - (colorValueWidth / 2) + colorValueXOffset;
    editor.colorValueLabel.setBounds(colorValueX, colorValueY, colorValueWidth, colorValueHeight);
    editor.colorLabel.setBounds(colorValueCenterX - (colorLabelWidth / 2),
                                colorValueY + colorValueHeight + s(4),
                                colorLabelWidth, labelHeight);

    editor.mixValueLabel.setBounds(mixCenterX - (mixValueWidth / 2) + mixValueOffsetX,
                                   mixValueY, mixValueWidth, mixValueHeight);
    editor.mixLabel.setBounds(mixCenterX - (mixLabelWidth / 2),
                              mixValueY + mixValueHeight + s(4),
                              mixLabelWidth, labelHeight);

    const juce::Font mainValueFont = editor.makeValueLabelFont(static_cast<float>(layout.knobValueFontSize) * editor.getUiScale(), true);
    editor.rateValueLabel.setFont(mainValueFont);
    editor.depthValueLabel.setFont(mainValueFont);
    editor.offsetValueLabel.setFont(mainValueFont);
    editor.widthValueLabel.setFont(mainValueFont);

    const juce::Font colorValueFont = editor.makeValueLabelFont(static_cast<float>(layout.colorValueFontSize) * editor.getUiScale(), true);
    editor.colorValueLabel.setFont(colorValueFont);
    const juce::Font mixValueFont = editor.makeValueLabelFont(static_cast<float>(layout.mixValueFontSize) * editor.getUiScale(), true);
    editor.mixValueLabel.setFont(mixValueFont);
    editor.updateValueLabelColors(colorIndex);

    const bool mainFlipEnabled = layout.mainValueFlipEnabled != 0;
    const int mainFlipDurationMs = layout.mainValueFlipDurationMs;
    const float mainFlipTravelUpPx = static_cast<float>(layout.mainValueFlipTravelUpPxTimes100) * 0.01f;
    const float mainFlipTravelDownPx = static_cast<float>(layout.mainValueFlipTravelDownPxTimes100) * 0.01f;
    const float mainFlipTravelOutScale = static_cast<float>(layout.mainValueFlipTravelOutPct) * 0.01f;
    const float mainFlipTravelInScale = static_cast<float>(layout.mainValueFlipTravelInPct) * 0.01f;
    const float mainFlipShearAmount = static_cast<float>(layout.mainValueFlipShearPct) * 0.01f;
    const float mainFlipScaleAmount = static_cast<float>(layout.mainValueFlipMinScalePct) * 0.01f;
    const float mainFlipMinScale = 1.0f - juce::jlimit(0.0f, 1.0f, mainFlipScaleAmount);
    editor.rateValueLabel.setFlipAnimationParams(mainFlipEnabled, mainFlipDurationMs, mainFlipTravelUpPx, mainFlipTravelDownPx, mainFlipTravelOutScale, mainFlipTravelInScale, mainFlipShearAmount, mainFlipMinScale);
    editor.depthValueLabel.setFlipAnimationParams(mainFlipEnabled, mainFlipDurationMs, mainFlipTravelUpPx, mainFlipTravelDownPx, mainFlipTravelOutScale, mainFlipTravelInScale, mainFlipShearAmount, mainFlipMinScale);
    editor.offsetValueLabel.setFlipAnimationParams(mainFlipEnabled, mainFlipDurationMs, mainFlipTravelUpPx, mainFlipTravelDownPx, mainFlipTravelOutScale, mainFlipTravelInScale, mainFlipShearAmount, mainFlipMinScale);
    editor.widthValueLabel.setFlipAnimationParams(mainFlipEnabled, mainFlipDurationMs, mainFlipTravelUpPx, mainFlipTravelDownPx, mainFlipTravelOutScale, mainFlipTravelInScale, mainFlipShearAmount, mainFlipMinScale);

    const bool colorFlipEnabled = layout.colorValueFlipEnabled != 0;
    const int colorFlipDurationMs = layout.colorValueFlipDurationMs;
    const float colorFlipTravelUpPx = static_cast<float>(layout.colorValueFlipTravelUpPxTimes100) * 0.01f;
    const float colorFlipTravelDownPx = static_cast<float>(layout.colorValueFlipTravelDownPxTimes100) * 0.01f;
    const float colorFlipTravelOutScale = static_cast<float>(layout.colorValueFlipTravelOutPct) * 0.01f;
    const float colorFlipTravelInScale = static_cast<float>(layout.colorValueFlipTravelInPct) * 0.01f;
    const float colorFlipShearAmount = static_cast<float>(layout.colorValueFlipShearPct) * 0.01f;
    const float colorFlipScaleAmount = static_cast<float>(layout.colorValueFlipMinScalePct) * 0.01f;
    const float colorFlipMinScale = 1.0f - juce::jlimit(0.0f, 1.0f, colorFlipScaleAmount);
    editor.colorValueLabel.setFlipAnimationParams(colorFlipEnabled, colorFlipDurationMs, colorFlipTravelUpPx, colorFlipTravelDownPx, colorFlipTravelOutScale, colorFlipTravelInScale, colorFlipShearAmount, colorFlipMinScale);

    const bool mixFlipEnabled = layout.mixValueFlipEnabled != 0;
    const int mixFlipDurationMs = layout.mixValueFlipDurationMs;
    const float mixFlipTravelUpPx = static_cast<float>(layout.mixValueFlipTravelUpPxTimes100) * 0.01f;
    const float mixFlipTravelDownPx = static_cast<float>(layout.mixValueFlipTravelDownPxTimes100) * 0.01f;
    const float mixFlipTravelOutScale = static_cast<float>(layout.mixValueFlipTravelOutPct) * 0.01f;
    const float mixFlipTravelInScale = static_cast<float>(layout.mixValueFlipTravelInPct) * 0.01f;
    const float mixFlipShearAmount = static_cast<float>(layout.mixValueFlipShearPct) * 0.01f;
    const float mixFlipScaleAmount = static_cast<float>(layout.mixValueFlipMinScalePct) * 0.01f;
    const float mixFlipMinScale = 1.0f - juce::jlimit(0.0f, 1.0f, mixFlipScaleAmount);
    editor.mixValueLabel.setFlipAnimationParams(mixFlipEnabled, mixFlipDurationMs, mixFlipTravelUpPx, mixFlipTravelDownPx, mixFlipTravelOutScale, mixFlipTravelInScale, mixFlipShearAmount, mixFlipMinScale);

    const bool mainFxEnabled = layout.valueFxEnabled != 0;
    const float mainGlowAlpha = static_cast<float>(layout.valueGlowAlphaPct) * 0.01f;
    const float mainGlowSpreadPx = static_cast<float>(layout.valueGlowSpreadPxTimes100) * 0.01f;
    const float mainPerCharOffsetX = static_cast<float>(layout.valueFxPerCharOffsetXPxTimes100) * 0.01f;
    const float mainPerCharOffsetY = static_cast<float>(layout.valueFxPerCharOffsetYPxTimes100) * 0.01f;
    const float mainTopAlpha = static_cast<float>(layout.valueTopReflectAlphaPct) * 0.01f;
    const float mainTopOffsetX = static_cast<float>(layout.valueTopReflectOffsetXPxTimes100) * 0.01f;
    const float mainTopOffsetY = static_cast<float>(layout.valueTopReflectOffsetYPxTimes100) * 0.01f;
    const float mainTopShear = static_cast<float>(layout.valueTopReflectShearPct) * 0.01f;
    const float mainTopRotateDeg = static_cast<float>(layout.valueTopReflectRotateDeg);
    const float mainBottomAlpha = static_cast<float>(layout.valueBottomReflectAlphaPct) * 0.01f;
    const float mainBottomOffsetX = static_cast<float>(layout.valueBottomReflectOffsetXPxTimes100) * 0.01f;
    const float mainBottomOffsetY = static_cast<float>(layout.valueBottomReflectOffsetYPxTimes100) * 0.01f;
    const float mainBottomShear = static_cast<float>(layout.valueBottomReflectShearPct) * 0.01f;
    const float mainBottomRotateDeg = static_cast<float>(layout.valueBottomReflectRotateDeg);
    const float mainReflectBlurPx = static_cast<float>(layout.valueReflectBlurPxTimes100) * 0.01f;
    const float mainReflectSquash = static_cast<float>(layout.valueReflectSquashPct) * 0.01f;
    const float mainReflectMotion = static_cast<float>(layout.valueReflectMotionPct) * 0.01f;

    const bool colorFxEnabled = layout.colorValueFxEnabled != 0;
    const float colorGlowAlpha = static_cast<float>(layout.colorValueGlowAlphaPct) * 0.01f;
    const float colorGlowSpreadPx = static_cast<float>(layout.colorValueGlowSpreadPxTimes100) * 0.01f;
    const float colorPerCharOffsetX = static_cast<float>(layout.colorValueFxPerCharOffsetXPxTimes100) * 0.01f;
    const float colorPerCharOffsetY = static_cast<float>(layout.colorValueFxPerCharOffsetYPxTimes100) * 0.01f;
    const float colorTopAlpha = static_cast<float>(layout.colorValueTopReflectAlphaPct) * 0.01f;
    const float colorTopOffsetX = static_cast<float>(layout.colorValueTopReflectOffsetXPxTimes100) * 0.01f;
    const float colorTopOffsetY = static_cast<float>(layout.colorValueTopReflectOffsetYPxTimes100) * 0.01f;
    const float colorTopShear = static_cast<float>(layout.colorValueTopReflectShearPct) * 0.01f;
    const float colorTopRotateDeg = static_cast<float>(layout.colorValueTopReflectRotateDeg);
    const float colorBottomAlpha = static_cast<float>(layout.colorValueBottomReflectAlphaPct) * 0.01f;
    const float colorBottomOffsetX = static_cast<float>(layout.colorValueBottomReflectOffsetXPxTimes100) * 0.01f;
    const float colorBottomOffsetY = static_cast<float>(layout.colorValueBottomReflectOffsetYPxTimes100) * 0.01f;
    const float colorBottomShear = static_cast<float>(layout.colorValueBottomReflectShearPct) * 0.01f;
    const float colorBottomRotateDeg = static_cast<float>(layout.colorValueBottomReflectRotateDeg);
    const float colorReflectBlurPx = static_cast<float>(layout.colorValueReflectBlurPxTimes100) * 0.01f;
    const float colorReflectSquash = static_cast<float>(layout.colorValueReflectSquashPct) * 0.01f;
    const float colorReflectMotion = static_cast<float>(layout.colorValueReflectMotionPct) * 0.01f;

    const bool mixFxEnabled = layout.mixValueFxEnabled != 0;
    const float mixGlowAlpha = static_cast<float>(layout.mixValueGlowAlphaPct) * 0.01f;
    const float mixGlowSpreadPx = static_cast<float>(layout.mixValueGlowSpreadPxTimes100) * 0.01f;
    const float mixPerCharOffsetX = static_cast<float>(layout.mixValueFxPerCharOffsetXPxTimes100) * 0.01f;
    const float mixPerCharOffsetY = static_cast<float>(layout.mixValueFxPerCharOffsetYPxTimes100) * 0.01f;
    const float mixTopAlpha = static_cast<float>(layout.mixValueTopReflectAlphaPct) * 0.01f;
    const float mixTopOffsetX = static_cast<float>(layout.mixValueTopReflectOffsetXPxTimes100) * 0.01f;
    const float mixTopOffsetY = static_cast<float>(layout.mixValueTopReflectOffsetYPxTimes100) * 0.01f;
    const float mixTopShear = static_cast<float>(layout.mixValueTopReflectShearPct) * 0.01f;
    const float mixTopRotateDeg = static_cast<float>(layout.mixValueTopReflectRotateDeg);
    const float mixBottomAlpha = static_cast<float>(layout.mixValueBottomReflectAlphaPct) * 0.01f;
    const float mixBottomOffsetX = static_cast<float>(layout.mixValueBottomReflectOffsetXPxTimes100) * 0.01f;
    const float mixBottomOffsetY = static_cast<float>(layout.mixValueBottomReflectOffsetYPxTimes100) * 0.01f;
    const float mixBottomShear = static_cast<float>(layout.mixValueBottomReflectShearPct) * 0.01f;
    const float mixBottomRotateDeg = static_cast<float>(layout.mixValueBottomReflectRotateDeg);
    const float mixReflectBlurPx = static_cast<float>(layout.mixValueReflectBlurPxTimes100) * 0.01f;
    const float mixReflectSquash = static_cast<float>(layout.mixValueReflectSquashPct) * 0.01f;
    const float mixReflectMotion = static_cast<float>(layout.mixValueReflectMotionPct) * 0.01f;

    editor.rateValueLabel.setValueFxParams(mainFxEnabled, mainGlowAlpha, mainGlowSpreadPx, mainPerCharOffsetX, mainPerCharOffsetY, mainTopAlpha, mainTopOffsetX, mainTopOffsetY, mainTopShear, mainTopRotateDeg, mainBottomAlpha, mainBottomOffsetX, mainBottomOffsetY, mainBottomShear, mainBottomRotateDeg, mainReflectBlurPx, mainReflectSquash, mainReflectMotion);
    editor.depthValueLabel.setValueFxParams(mainFxEnabled, mainGlowAlpha, mainGlowSpreadPx, mainPerCharOffsetX, mainPerCharOffsetY, mainTopAlpha, mainTopOffsetX, mainTopOffsetY, mainTopShear, mainTopRotateDeg, mainBottomAlpha, mainBottomOffsetX, mainBottomOffsetY, mainBottomShear, mainBottomRotateDeg, mainReflectBlurPx, mainReflectSquash, mainReflectMotion);
    editor.offsetValueLabel.setValueFxParams(mainFxEnabled, mainGlowAlpha, mainGlowSpreadPx, mainPerCharOffsetX, mainPerCharOffsetY, mainTopAlpha, mainTopOffsetX, mainTopOffsetY, mainTopShear, mainTopRotateDeg, mainBottomAlpha, mainBottomOffsetX, mainBottomOffsetY, mainBottomShear, mainBottomRotateDeg, mainReflectBlurPx, mainReflectSquash, mainReflectMotion);
    editor.widthValueLabel.setValueFxParams(mainFxEnabled, mainGlowAlpha, mainGlowSpreadPx, mainPerCharOffsetX, mainPerCharOffsetY, mainTopAlpha, mainTopOffsetX, mainTopOffsetY, mainTopShear, mainTopRotateDeg, mainBottomAlpha, mainBottomOffsetX, mainBottomOffsetY, mainBottomShear, mainBottomRotateDeg, mainReflectBlurPx, mainReflectSquash, mainReflectMotion);
    editor.colorValueLabel.setValueFxParams(colorFxEnabled, colorGlowAlpha, colorGlowSpreadPx, colorPerCharOffsetX, colorPerCharOffsetY, colorTopAlpha, colorTopOffsetX, colorTopOffsetY, colorTopShear, colorTopRotateDeg, colorBottomAlpha, colorBottomOffsetX, colorBottomOffsetY, colorBottomShear, colorBottomRotateDeg, colorReflectBlurPx, colorReflectSquash, colorReflectMotion);
    editor.mixValueLabel.setValueFxParams(mixFxEnabled, mixGlowAlpha, mixGlowSpreadPx, mixPerCharOffsetX, mixPerCharOffsetY, mixTopAlpha, mixTopOffsetX, mixTopOffsetY, mixTopShear, mixTopRotateDeg, mixBottomAlpha, mixBottomOffsetX, mixBottomOffsetY, mixBottomShear, mixBottomRotateDeg, mixReflectBlurPx, mixReflectSquash, mixReflectMotion);

    const int topButtonW = s(layout.topButtonsWidth);
    const int topButtonH = s(layout.topButtonsHeight);
    const int topButtonGap = s(layout.topButtonsGap);
    const int topButtonY = s(layout.topButtonsTopY);
    const int rightMargin = s(layout.topButtonsRightMargin);
    const int rightEdge = editor.getWidth() - rightMargin;
    editor.feedbackButton.setBounds(rightEdge - topButtonW, topButtonY, topButtonW, topButtonH);
    editor.helpButton.setBounds(rightEdge - (2 * topButtonW) - topButtonGap, topButtonY, topButtonW, topButtonH);
    editor.aboutButton.setBounds(rightEdge - (3 * topButtonW) - (2 * topButtonGap), topButtonY, topButtonW, topButtonH);

    const float topButtonFontHeight = static_cast<float>(layout.topButtonsFontSize) * editor.getUiScale();
    editor.feedbackButton.getProperties().set("customFontHeight", topButtonFontHeight);
    editor.helpButton.getProperties().set("customFontHeight", topButtonFontHeight);
    editor.aboutButton.getProperties().set("customFontHeight", topButtonFontHeight);

    const auto buttonBg = makeColour(layout.topButtonsBackgroundColour);
    const auto buttonBgOn = makeColour(layout.topButtonsOnBackgroundColour);
    const auto buttonText = makeColour(layout.topButtonsTextColour);
    editor.feedbackButton.setColour(juce::TextButton::buttonColourId, buttonBg);
    editor.feedbackButton.setColour(juce::TextButton::buttonOnColourId, buttonBgOn);
    editor.feedbackButton.setColour(juce::TextButton::textColourOffId, buttonText);
    editor.feedbackButton.setColour(juce::TextButton::textColourOnId, buttonText);
    editor.helpButton.setColour(juce::TextButton::buttonColourId, buttonBg);
    editor.helpButton.setColour(juce::TextButton::buttonOnColourId, buttonBgOn);
    editor.helpButton.setColour(juce::TextButton::textColourOffId, buttonText);
    editor.helpButton.setColour(juce::TextButton::textColourOnId, buttonText);
    editor.aboutButton.setColour(juce::TextButton::buttonColourId, buttonBg);
    editor.aboutButton.setColour(juce::TextButton::buttonOnColourId, buttonBgOn);
    editor.aboutButton.setColour(juce::TextButton::textColourOffId, buttonText);
    editor.aboutButton.setColour(juce::TextButton::textColourOnId, buttonText);

    editor.engineColorBox.setBounds(s(layout.engineSelectorX), s(layout.engineSelectorY), s(layout.engineSelectorW), s(layout.engineSelectorH));
    editor.engineColorBox.setColour(juce::ComboBox::textColourId, makeColour(layout.engineSelectorTextColour));
    editor.engineColorBox.setColour(juce::ComboBox::backgroundColourId, makeColour(layout.engineSelectorBackgroundColour));
    editor.engineColorBox.setColour(juce::ComboBox::outlineColourId, makeColour(layout.engineSelectorOutlineColour));
    editor.engineColorBox.setColour(juce::ComboBox::arrowColourId, makeColour(layout.engineSelectorArrowColour));
    editor.engineColorBox.setColour(juce::PopupMenu::backgroundColourId, makeColour(layout.engineSelectorPopupBackgroundColour));
    editor.engineColorBox.setColour(juce::PopupMenu::textColourId, makeColour(layout.engineSelectorPopupTextColour));
    editor.engineColorBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, makeColour(layout.engineSelectorPopupHighlightedBackgroundColour));
    editor.engineColorBox.setColour(juce::PopupMenu::highlightedTextColourId, makeColour(layout.engineSelectorPopupHighlightedTextColour));
    editor.engineColorBox.getProperties().set("customFontHeight", static_cast<float>(layout.engineSelectorFontSize) * editor.getUiScale());
    editor.customLookAndFeel.setColour(juce::PopupMenu::backgroundColourId, makeColour(layout.engineSelectorPopupBackgroundColour));
    editor.customLookAndFeel.setColour(juce::PopupMenu::textColourId, makeColour(layout.engineSelectorPopupTextColour));
    editor.customLookAndFeel.setColour(juce::PopupMenu::highlightedBackgroundColourId, makeColour(layout.engineSelectorPopupHighlightedBackgroundColour));
    editor.customLookAndFeel.setColour(juce::PopupMenu::highlightedTextColourId, makeColour(layout.engineSelectorPopupHighlightedTextColour));
    editor.customLookAndFeel.setPopupMenuFontHeight(static_cast<float>(layout.engineSelectorFontSize) * editor.getUiScale());
    editor.engineColorBox.setLookAndFeel(&editor.customLookAndFeel);
    editor.engineColorBox.setJustificationType(juce::Justification::centred);
    editor.engineColorBox.setTooltip("Engine Selection: Choose between five distinct chorus algorithms. Green=Classic, Blue=Modern, Red=Vintage, Purple=Experimental, Black=Linear.");

    const int hqSize = s(layout.hqSwitchSize);
    const int hqCenterX = s(350 + layout.hqSwitchOffsetX);
    const int hqCenterY = s(152 + layout.hqSwitchOffsetY);
    editor.hqButton.setBounds(hqCenterX - (hqSize / 2), hqCenterY - (hqSize / 2), hqSize, hqSize);

    const juce::Font hqFont = editor.makeUiTextFont(12.25f * editor.getUiScale(), true);
    editor.hqLabel.setFont(hqFont);
    const int hqLabelWidth = editor.calculateLabelWidth("HQ", hqFont);
    editor.hqLabel.setBounds(hqCenterX - (hqLabelWidth / 2), hqCenterY + (hqSize / 2) + s(1), hqLabelWidth, s(18));

    editor.rateSlider.setSmoothingTime(static_cast<float>(layout.rateKnobVisualResponseMs));
    editor.depthSlider.setSmoothingTime(static_cast<float>(layout.depthKnobVisualResponseMs));
    editor.offsetSlider.setSmoothingTime(static_cast<float>(layout.offsetKnobVisualResponseMs));
    editor.widthSlider.setSmoothingTime(static_cast<float>(layout.widthKnobVisualResponseMs));
    editor.mixSlider.setSmoothingTime(static_cast<float>(layout.mixKnobVisualResponseMs));

    const auto setKnobSweepProps = [&layout](juce::Slider& knob)
    {
        knob.getProperties().set("knobSweepStartDeg", layout.knobSweepStartDeg);
        knob.getProperties().set("knobSweepEndDeg", layout.knobSweepEndDeg);
        knob.getProperties().set("knobFrameCount", layout.knobFrameCount);
    };

    setKnobSweepProps(editor.rateSlider);
    setKnobSweepProps(editor.depthSlider);
    setKnobSweepProps(editor.offsetSlider);
    setKnobSweepProps(editor.widthSlider);
    setKnobSweepProps(editor.mixSlider);
}

void PluginEditorSetup::setupSliders(ChoroborosPluginEditor& editor)
{
    // Component IDs for sprite sheet selection
    editor.rateSlider.setComponentID("Rate");
    editor.depthSlider.setComponentID("Depth");
    editor.offsetSlider.setComponentID("Offset");
    editor.widthSlider.setComponentID("Width");
    editor.mixSlider.setComponentID("Mix");
    
    // Configure slider styles
    editor.rateSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.depthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.offsetSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.widthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.colorSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    editor.colorSlider.setVelocityBasedMode(false);  // Position-based drag, not velocity - reduces jank
    editor.mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    
    editor.rateSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.depthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.offsetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.widthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.colorSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    // Set mix slider name so CustomLookAndFeel can identify it
    editor.mixSlider.setName("Mix");
    
    // Set visual smoothing times
    editor.rateSlider.setUseExponential(true);
    editor.rateSlider.setSmoothingTime(100.0f);
    editor.depthSlider.setUseExponential(true);
    editor.depthSlider.setSmoothingTime(150.0f);
    editor.offsetSlider.setUseExponential(true);
    editor.offsetSlider.setSmoothingTime(100.0f);
    editor.widthSlider.setUseExponential(true);
    editor.widthSlider.setSmoothingTime(100.0f);
    editor.colorSlider.setSmoothingTime(25.0f);  // Snappier for linear slider - reduces lag/stuck feel
    editor.mixSlider.setSmoothingTime(100.0f);

    applyLayout(editor, editor.layoutTuning);
}

void PluginEditorSetup::setupValueLabels(ChoroborosPluginEditor& editor)
{
    const juce::Font valueFont = editor.makeValueLabelFont(15.3f * editor.getUiScale(), true);

    editor.rateValueLabel.setJustificationType(juce::Justification::centredRight);
    editor.depthValueLabel.setJustificationType(juce::Justification::centredRight);
    editor.offsetValueLabel.setJustificationType(juce::Justification::centredRight);
    editor.widthValueLabel.setJustificationType(juce::Justification::centredRight);
    editor.colorValueLabel.setJustificationType(juce::Justification::centredRight);
    editor.mixValueLabel.setJustificationType(juce::Justification::centredRight);
    
    editor.rateValueLabel.setValueLabelStyle(true);
    editor.depthValueLabel.setValueLabelStyle(true);
    editor.offsetValueLabel.setValueLabelStyle(true);
    editor.widthValueLabel.setValueLabelStyle(true);
    editor.colorValueLabel.setValueLabelStyle(true);
    editor.mixValueLabel.setValueLabelStyle(true);

    editor.rateValueLabel.setFont(valueFont);
    editor.depthValueLabel.setFont(valueFont);
    editor.offsetValueLabel.setFont(valueFont);
    editor.widthValueLabel.setFont(valueFont);
    editor.colorValueLabel.setFont(valueFont);
    editor.mixValueLabel.setFont(valueFont);
    
    juce::Colour valueTextColor(0xff9dbd78);
    editor.rateValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.depthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.offsetValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.widthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.colorValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.mixValueLabel.setColour(juce::Label::textColourId, valueTextColor);
}

void PluginEditorSetup::setupLabels(ChoroborosPluginEditor& editor)
{
    editor.rateLabel.setVisible(false);
    editor.depthLabel.setVisible(false);
    editor.offsetLabel.setVisible(false);
    editor.widthLabel.setVisible(false);
    editor.colorLabel.setVisible(false);

    // Mix value label uses 25% smaller font (14.0f * 0.75 = 10.5f)
    const juce::Font mixValueFont = editor.makeValueLabelFont(11.5f * editor.getUiScale(), true);
    editor.mixValueLabel.setFont(mixValueFont);
    editor.mixLabel.setVisible(false);

    applyLayout(editor, editor.layoutTuning);
}

void PluginEditorSetup::setupHQButton(ChoroborosPluginEditor& editor)
{
    editor.addAndMakeVisible(editor.hqButton);
    editor.addAndMakeVisible(editor.hqLabel);
    
    editor.hqLabel.setText("HQ", juce::dontSendNotification);
    editor.hqLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    editor.hqLabel.setJustificationType(juce::Justification::centred);

    // Set HQ button tooltip
    editor.hqButton.setTooltip("High Quality Mode: Enables higher-quality algorithm variant for the selected engine. Increases CPU usage but improves audio fidelity.");
    editor.hqLabel.setVisible(false);

    // Repaint editor when HQ switch animates so lit backpanel overlay stays synced (all themes)
    editor.hqButton.onAnimationTick = [&editor] { editor.repaint(); };

    applyLayout(editor, editor.layoutTuning);
}
