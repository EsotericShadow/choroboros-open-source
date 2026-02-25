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

#pragma once

class ChoroborosPluginEditor;

struct LayoutTuning
{
    int mainKnobSize = 128; // Legacy fallback/default
    int mainKnobSizeGreen = 128;
    int mainKnobSizeBlue = 128;
    int mainKnobSizeRed = 128;
    int mainKnobSizePurple = 128;
    int mainKnobSizeBlack = 128;
    int knobTopY = 78;
    int knobTopYGreen = 78;
    int knobTopYBlue = 78;
    int knobTopYRed = 78;
    int knobTopYPurple = 78;
    int knobTopYBlack = 78;
    int rateCenterX = 102;
    int rateCenterXGreen = 102;
    int rateCenterXBlue = 102;
    int rateCenterXRed = 102;
    int rateCenterXPurple = 102;
    int rateCenterXBlack = 102;
    int depthCenterX = 251;
    int depthCenterXGreen = 251;
    int depthCenterXBlue = 251;
    int depthCenterXRed = 251;
    int depthCenterXPurple = 251;
    int depthCenterXBlack = 251;
    int offsetCenterX = 466;
    int offsetCenterXGreen = 466;
    int offsetCenterXBlue = 466;
    int offsetCenterXRed = 466;
    int offsetCenterXPurple = 466;
    int offsetCenterXBlack = 466;
    int widthCenterX = 607;
    int widthCenterXGreen = 607;
    int widthCenterXBlue = 607;
    int widthCenterXRed = 607;
    int widthCenterXPurple = 607;
    int widthCenterXBlack = 607;

    // Slider (color bar) - track start/end define position and length, size = thumb height
    int sliderTrackStartX = 235;
    int sliderTrackStartY = 268;
    int sliderTrackEndX = 485;
    int sliderTrackEndY = 268;
    int sliderSize = 100;  // Thumb height scale: 100 = 18px, 200 = 36px, etc.
    // Per-engine slider overrides (fallback to base values above when not set)
    int sliderTrackStartXGreen = 235;
    int sliderTrackStartYGreen = 268;
    int sliderTrackEndXGreen = 485;
    int sliderTrackEndYGreen = 268;
    int sliderSizeGreen = 100;
    int sliderTrackStartXBlue = 235;
    int sliderTrackStartYBlue = 268;
    int sliderTrackEndXBlue = 485;
    int sliderTrackEndYBlue = 268;
    int sliderSizeBlue = 100;
    int sliderTrackStartXRed = 235;
    int sliderTrackStartYRed = 268;
    int sliderTrackEndXRed = 485;
    int sliderTrackEndYRed = 268;
    int sliderSizeRed = 100;
    int sliderTrackStartXPurple = 235;
    int sliderTrackStartYPurple = 268;
    int sliderTrackEndXPurple = 485;
    int sliderTrackEndYPurple = 268;
    int sliderSizePurple = 100;
    int sliderTrackStartXBlack = 235;
    int sliderTrackStartYBlack = 268;
    int sliderTrackEndXBlack = 485;
    int sliderTrackEndYBlack = 268;
    int sliderSizeBlack = 100;

    int mixKnobSize = 58; // Legacy fallback/default
    int mixKnobSizeGreen = 58;
    int mixKnobSizeBlue = 58;
    int mixKnobSizeRed = 58;
    int mixKnobSizePurple = 58;
    int mixKnobSizeBlack = 58;
    int mixCenterX = 612;
    int mixCenterXGreen = 612;
    int mixCenterXBlue = 612;
    int mixCenterXRed = 612;
    int mixCenterXPurple = 612;
    int mixCenterXBlack = 612;
    int mixKnobY = 251;  // Mix knob top Y - legacy fallback
    int mixKnobYGreen = 251;
    int mixKnobYBlue = 251;
    int mixKnobYRed = 251;
    int mixKnobYPurple = 251;
    int mixKnobYBlack = 251;
    int mixKnobYOffset = 12;
    int mixKnobYOffsetGreen = 12;
    int mixKnobYOffsetBlue = 12;
    int mixKnobYOffsetRed = 12;
    int mixKnobYOffsetPurple = 12;
    int mixKnobYOffsetBlack = 12;

    int valueLabelWidth = 95;
    int valueLabelHeight = 33;
    int valueLabelY = 201;
    int valueLabelYGreen = 201;
    int valueLabelYBlue = 201;
    int valueLabelYRed = 201;
    int valueLabelYPurple = 201;
    int valueLabelYBlack = 201;
    int rateValueOffsetX = -18;
    int rateValueOffsetXGreen = -18;
    int rateValueOffsetXBlue = -18;
    int rateValueOffsetXRed = -18;
    int rateValueOffsetXPurple = -18;
    int rateValueOffsetXBlack = -18;
    int depthValueOffsetX = -27;
    int depthValueOffsetXGreen = -27;
    int depthValueOffsetXBlue = -27;
    int depthValueOffsetXRed = -27;
    int depthValueOffsetXPurple = -27;
    int depthValueOffsetXBlack = -27;
    int offsetValueOffsetX = -27;
    int offsetValueOffsetXGreen = -27;
    int offsetValueOffsetXBlue = -27;
    int offsetValueOffsetXRed = -27;
    int offsetValueOffsetXPurple = -27;
    int offsetValueOffsetXBlack = -27;
    int widthValueOffsetX = -25;
    int widthValueOffsetXGreen = -25;
    int widthValueOffsetXBlue = -25;
    int widthValueOffsetXRed = -25;
    int widthValueOffsetXPurple = -25;
    int widthValueOffsetXBlack = -25;
    int rateValueOffsetY = 0;
    int depthValueOffsetY = 0;
    int offsetValueOffsetY = 0;
    int widthValueOffsetY = 0;

    int colorValueCenterX = 360;  // Color value/label horizontal center - independent of slider
    int colorValueWidth = 65;
    int colorValueHeight = 25;
    int colorValueY = 301;
    int colorValueYGreen = 301;
    int colorValueYBlue = 301;
    int colorValueYRed = 301;
    int colorValueYPurple = 301;
    int colorValueYBlack = 301;
    int colorValueXOffset = 0;
    int colorValueXOffsetGreen = 0;
    int colorValueXOffsetBlue = 0;
    int colorValueXOffsetRed = 0;
    int colorValueXOffsetPurple = 0;
    int colorValueXOffsetBlack = 0;

    int mixValueWidth = 55;
    int mixValueHeight = 15;
    int mixValueY = 333;
    int mixValueYGreen = 333;
    int mixValueYBlue = 333;
    int mixValueYRed = 333;
    int mixValueYPurple = 333;
    int mixValueYBlack = 333;
    int mixValueOffsetX = -15;
    int mixValueOffsetXGreen = -15;
    int mixValueOffsetXBlue = -15;
    int mixValueOffsetXRed = -15;
    int mixValueOffsetXPurple = -15;
    int mixValueOffsetXBlack = -15;

    int knobValueFontSize = 15;
    int colorValueFontSize = 15;
    int mixValueFontSize = 11;
    int valueTextAlphaPct = 100;

    int topButtonsWidth = 45;
    int topButtonsHeight = 10;
    int topButtonsGap = 5;
    int topButtonsRightMargin = 10;
    int topButtonsTopY = 5;
    int topButtonsFontSize = 10;
    int topButtonsTextColour = static_cast<int>(0xffd3d3d3);
    int topButtonsBackgroundColour = static_cast<int>(0xff4a4a4a);
    int topButtonsOnBackgroundColour = static_cast<int>(0xff5a5a5a);

    int engineSelectorX = 20;
    int engineSelectorY = 335;
    int engineSelectorW = 80;
    int engineSelectorH = 14;
    int engineSelectorFontSize = 10;
    int engineSelectorTextColour = static_cast<int>(0xffffffff);
    int engineSelectorBackgroundColour = static_cast<int>(0x00000000);
    int engineSelectorOutlineColour = static_cast<int>(0x00000000);
    int engineSelectorArrowColour = static_cast<int>(0xffffffff);
    int engineSelectorPopupBackgroundColour = static_cast<int>(0xff1c1c1c);
    int engineSelectorPopupTextColour = static_cast<int>(0xffffffff);
    int engineSelectorPopupHighlightedBackgroundColour = static_cast<int>(0xff3a3a3a);
    int engineSelectorPopupHighlightedTextColour = static_cast<int>(0xffffffff);
    int hqSwitchSize = 45;
    int hqSwitchOffsetX = 0;
    int hqSwitchOffsetY = 0;
    int rateKnobVisualResponseMs = 100;
    int depthKnobVisualResponseMs = 150;
    int offsetKnobVisualResponseMs = 100;
    int widthKnobVisualResponseMs = 100;
    int mixKnobVisualResponseMs = 100;
    int knobSweepStartDeg = 0;
    int knobSweepEndDeg = 360;
    int knobFrameCount = 156;

    int mainValueFlipEnabled = 1;
    int mainValueFlipDurationMs = 140;
    int mainValueFlipTravelUpPxTimes100 = 30;   // 0.30 px default
    int mainValueFlipTravelDownPxTimes100 = 30; // 0.30 px default
    int mainValueFlipTravelOutPct = 100;       // 1.0 default
    int mainValueFlipTravelInPct = 100;        // 1.0 default
    int mainValueFlipShearPct = 32;            // 0.32 default
    int mainValueFlipMinScalePct = 35;         // 0.35 default

    int colorValueFlipEnabled = 1;
    int colorValueFlipDurationMs = 140;
    int colorValueFlipTravelUpPxTimes100 = 30;   // 0.30 px default
    int colorValueFlipTravelDownPxTimes100 = 30; // 0.30 px default
    int colorValueFlipTravelOutPct = 100;       // 1.0 default
    int colorValueFlipTravelInPct = 100;        // 1.0 default
    int colorValueFlipShearPct = 32;            // 0.32 default
    int colorValueFlipMinScalePct = 35;         // 0.35 default

    int mixValueFlipEnabled = 1;
    int mixValueFlipDurationMs = 140;
    int mixValueFlipTravelUpPxTimes100 = 30;   // 0.30 px default
    int mixValueFlipTravelDownPxTimes100 = 30; // 0.30 px default
    int mixValueFlipTravelOutPct = 100;       // 1.0 default
    int mixValueFlipTravelInPct = 100;        // 1.0 default
    int mixValueFlipShearPct = 32;            // 0.32 default
    int mixValueFlipMinScalePct = 35;         // 0.35 default

    int valueFxEnabled = 1;
    int valueGlowAlphaPct = 4;
    int valueGlowSpreadPxTimes100 = 65;
    int valueFxPerCharOffsetXPxTimes100 = 0;
    int valueFxPerCharOffsetYPxTimes100 = 0;
    int valueTopReflectAlphaPct = 2;
    int valueTopReflectOffsetXPxTimes100 = 0;
    int valueTopReflectOffsetYPxTimes100 = -80;
    int valueTopReflectShearPct = 6;
    int valueTopReflectRotateDeg = 0;
    int valueBottomReflectAlphaPct = 3;
    int valueBottomReflectOffsetXPxTimes100 = 0;
    int valueBottomReflectOffsetYPxTimes100 = 110;
    int valueBottomReflectShearPct = -8;
    int valueBottomReflectRotateDeg = 0;
    int valueReflectBlurPxTimes100 = 70;
    int valueReflectSquashPct = 35;
    int valueReflectMotionPct = 20;

    int colorValueFxEnabled = 1;
    int colorValueGlowAlphaPct = 4;
    int colorValueGlowSpreadPxTimes100 = 65;
    int colorValueFxPerCharOffsetXPxTimes100 = 0;
    int colorValueFxPerCharOffsetYPxTimes100 = 0;
    int colorValueTopReflectAlphaPct = 2;
    int colorValueTopReflectOffsetXPxTimes100 = 0;
    int colorValueTopReflectOffsetYPxTimes100 = -80;
    int colorValueTopReflectShearPct = 6;
    int colorValueTopReflectRotateDeg = 0;
    int colorValueBottomReflectAlphaPct = 3;
    int colorValueBottomReflectOffsetXPxTimes100 = 0;
    int colorValueBottomReflectOffsetYPxTimes100 = 110;
    int colorValueBottomReflectShearPct = -8;
    int colorValueBottomReflectRotateDeg = 0;
    int colorValueReflectBlurPxTimes100 = 70;
    int colorValueReflectSquashPct = 35;
    int colorValueReflectMotionPct = 20;

    int mixValueFxEnabled = 1;
    int mixValueGlowAlphaPct = 4;
    int mixValueGlowSpreadPxTimes100 = 65;
    int mixValueFxPerCharOffsetXPxTimes100 = 0;
    int mixValueFxPerCharOffsetYPxTimes100 = 0;
    int mixValueTopReflectAlphaPct = 2;
    int mixValueTopReflectOffsetXPxTimes100 = 0;
    int mixValueTopReflectOffsetYPxTimes100 = -80;
    int mixValueTopReflectShearPct = 6;
    int mixValueTopReflectRotateDeg = 0;
    int mixValueBottomReflectAlphaPct = 3;
    int mixValueBottomReflectOffsetXPxTimes100 = 0;
    int mixValueBottomReflectOffsetYPxTimes100 = 110;
    int mixValueBottomReflectShearPct = -8;
    int mixValueBottomReflectRotateDeg = 0;
    int mixValueReflectBlurPxTimes100 = 70;
    int mixValueReflectSquashPct = 35;
    int mixValueReflectMotionPct = 20;
};

class PluginEditorSetup
{
public:
    static LayoutTuning makeDefaultLayout();
    static void applyLayout(ChoroborosPluginEditor& editor, const LayoutTuning& layout);
    static void setupSliders(ChoroborosPluginEditor& editor);
    static void setupValueLabels(ChoroborosPluginEditor& editor);
    static void setupLabels(ChoroborosPluginEditor& editor);
    static void setupHQButton(ChoroborosPluginEditor& editor);
};
