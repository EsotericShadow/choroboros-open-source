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

#include "DevPanel.h"
#include "../Plugin/PluginEditor.h"
#include "../Plugin/PluginProcessor.h"
#include "../Config/DefaultsPersistence.h"
#include "DevPanelSupport.h"

using namespace devpanel;

juce::String DevPanel::buildJson() const
{
    auto& tuning = processor.getTuningState();
    const auto& layout = editor.getLayoutTuning();

    juce::String json;
    json << "{\n  \"tuning\": {\n";
    auto appendParam = [&](const juce::String& name, const ChoroborosAudioProcessor::ParamTuning& param, bool isLast)
    {
        json << "    \"" << name << "\": {"
             << "\"min\": " << formatFloat(param.min.load())
             << ", \"max\": " << formatFloat(param.max.load())
             << ", \"curve\": " << formatFloat(param.curve.load())
             << ", \"uiSkew\": " << formatFloat(param.uiSkew.load())
             << "}" << (isLast ? "\n" : ",\n");
    };

    appendParam("rate", tuning.rate, false);
    appendParam("depth", tuning.depth, false);
    appendParam("offset", tuning.offset, false);
    appendParam("width", tuning.width, false);
    appendParam("color", tuning.color, false);
    appendParam("mix", tuning.mix, true);
    json << "  },\n";

    auto appendInternalsObject = [&](const juce::String& key, const ChorusDSP::RuntimeTuning& internals)
    {
        json << "  \"" << key << "\": {\n";
        json << "    \"rateSmoothingMs\": " << formatFloat(internals.rateSmoothingMs.load()) << ",\n";
        json << "    \"depthSmoothingMs\": " << formatFloat(internals.depthSmoothingMs.load()) << ",\n";
        json << "    \"depthRateLimit\": " << formatFloat(internals.depthRateLimit.load()) << ",\n";
        json << "    \"centreDelaySmoothingMs\": " << formatFloat(internals.centreDelaySmoothingMs.load()) << ",\n";
        json << "    \"centreDelayBaseMs\": " << formatFloat(internals.centreDelayBaseMs.load()) << ",\n";
        json << "    \"centreDelayScale\": " << formatFloat(internals.centreDelayScale.load()) << ",\n";
        json << "    \"colorSmoothingMs\": " << formatFloat(internals.colorSmoothingMs.load()) << ",\n";
        json << "    \"widthSmoothingMs\": " << formatFloat(internals.widthSmoothingMs.load()) << ",\n";
        json << "    \"hpfCutoffHz\": " << formatFloat(internals.hpfCutoffHz.load()) << ",\n";
        json << "    \"hpfQ\": " << formatFloat(internals.hpfQ.load()) << ",\n";
        json << "    \"lpfCutoffHz\": " << formatFloat(internals.lpfCutoffHz.load()) << ",\n";
        json << "    \"lpfQ\": " << formatFloat(internals.lpfQ.load()) << ",\n";
        json << "    \"preEmphasisFreqHz\": " << formatFloat(internals.preEmphasisFreqHz.load()) << ",\n";
        json << "    \"preEmphasisQ\": " << formatFloat(internals.preEmphasisQ.load()) << ",\n";
        json << "    \"preEmphasisGain\": " << formatFloat(internals.preEmphasisGain.load()) << ",\n";
        json << "    \"preEmphasisLevelSmoothing\": " << formatFloat(internals.preEmphasisLevelSmoothing.load()) << ",\n";
        json << "    \"preEmphasisQuietThreshold\": " << formatFloat(internals.preEmphasisQuietThreshold.load()) << ",\n";
        json << "    \"preEmphasisMaxAmount\": " << formatFloat(internals.preEmphasisMaxAmount.load()) << ",\n";
        json << "    \"compressorAttackMs\": " << formatFloat(internals.compressorAttackMs.load()) << ",\n";
        json << "    \"compressorReleaseMs\": " << formatFloat(internals.compressorReleaseMs.load()) << ",\n";
        json << "    \"compressorThresholdDb\": " << formatFloat(internals.compressorThresholdDb.load()) << ",\n";
        json << "    \"compressorRatio\": " << formatFloat(internals.compressorRatio.load()) << ",\n";
        json << "    \"saturationDriveScale\": " << formatFloat(internals.saturationDriveScale.load()) << ",\n";
        json << "    \"greenBloomExponent\": " << formatFloat(internals.greenBloomExponent.load()) << ",\n";
        json << "    \"greenBloomDepthScale\": " << formatFloat(internals.greenBloomDepthScale.load()) << ",\n";
        json << "    \"greenBloomCentreOffsetMs\": " << formatFloat(internals.greenBloomCentreOffsetMs.load()) << ",\n";
        json << "    \"greenBloomCutoffMaxHz\": " << formatFloat(internals.greenBloomCutoffMaxHz.load()) << ",\n";
        json << "    \"greenBloomCutoffMinHz\": " << formatFloat(internals.greenBloomCutoffMinHz.load()) << ",\n";
        json << "    \"greenBloomWetBlend\": " << formatFloat(internals.greenBloomWetBlend.load()) << ",\n";
        json << "    \"greenBloomGain\": " << formatFloat(internals.greenBloomGain.load()) << ",\n";
        json << "    \"blueFocusExponent\": " << formatFloat(internals.blueFocusExponent.load()) << ",\n";
        json << "    \"blueFocusHpMinHz\": " << formatFloat(internals.blueFocusHpMinHz.load()) << ",\n";
        json << "    \"blueFocusHpMaxHz\": " << formatFloat(internals.blueFocusHpMaxHz.load()) << ",\n";
        json << "    \"blueFocusLpMaxHz\": " << formatFloat(internals.blueFocusLpMaxHz.load()) << ",\n";
        json << "    \"blueFocusLpMinHz\": " << formatFloat(internals.blueFocusLpMinHz.load()) << ",\n";
        json << "    \"bluePresenceFreqMinHz\": " << formatFloat(internals.bluePresenceFreqMinHz.load()) << ",\n";
        json << "    \"bluePresenceFreqMaxHz\": " << formatFloat(internals.bluePresenceFreqMaxHz.load()) << ",\n";
        json << "    \"bluePresenceQMin\": " << formatFloat(internals.bluePresenceQMin.load()) << ",\n";
        json << "    \"bluePresenceQMax\": " << formatFloat(internals.bluePresenceQMax.load()) << ",\n";
        json << "    \"bluePresenceGainMaxDb\": " << formatFloat(internals.bluePresenceGainMaxDb.load()) << ",\n";
        json << "    \"blueFocusWetBlend\": " << formatFloat(internals.blueFocusWetBlend.load()) << ",\n";
        json << "    \"blueFocusOutputGain\": " << formatFloat(internals.blueFocusOutputGain.load()) << ",\n";
        json << "    \"purpleWarpA\": " << formatFloat(internals.purpleWarpA.load()) << ",\n";
        json << "    \"purpleWarpB\": " << formatFloat(internals.purpleWarpB.load()) << ",\n";
        json << "    \"purpleWarpKBase\": " << formatFloat(internals.purpleWarpKBase.load()) << ",\n";
        json << "    \"purpleWarpKScale\": " << formatFloat(internals.purpleWarpKScale.load()) << ",\n";
        json << "    \"purpleWarpDelaySmoothingMs\": " << formatFloat(internals.purpleWarpDelaySmoothingMs.load()) << ",\n";
        json << "    \"purpleOrbitEccentricity\": " << formatFloat(internals.purpleOrbitEccentricity.load()) << ",\n";
        json << "    \"purpleOrbitThetaRateBaseHz\": " << formatFloat(internals.purpleOrbitThetaRateBaseHz.load()) << ",\n";
        json << "    \"purpleOrbitThetaRateScaleHz\": " << formatFloat(internals.purpleOrbitThetaRateScaleHz.load()) << ",\n";
        json << "    \"purpleOrbitThetaRate2Ratio\": " << formatFloat(internals.purpleOrbitThetaRate2Ratio.load()) << ",\n";
        json << "    \"purpleOrbitEccentricity2Ratio\": " << formatFloat(internals.purpleOrbitEccentricity2Ratio.load()) << ",\n";
        json << "    \"purpleOrbitMix1\": " << formatFloat(internals.purpleOrbitMix1.load()) << ",\n";
        json << "    \"purpleOrbitStereoThetaOffset\": " << formatFloat(internals.purpleOrbitStereoThetaOffset.load()) << ",\n";
        json << "    \"purpleOrbitDelaySmoothingMs\": " << formatFloat(internals.purpleOrbitDelaySmoothingMs.load()) << ",\n";
        json << "    \"blackNqDepthBase\": " << formatFloat(internals.blackNqDepthBase.load()) << ",\n";
        json << "    \"blackNqDepthScale\": " << formatFloat(internals.blackNqDepthScale.load()) << ",\n";
        json << "    \"blackNqDelayGlideMs\": " << formatFloat(internals.blackNqDelayGlideMs.load()) << ",\n";
        json << "    \"blackHqTap2MixBase\": " << formatFloat(internals.blackHqTap2MixBase.load()) << ",\n";
        json << "    \"blackHqTap2MixScale\": " << formatFloat(internals.blackHqTap2MixScale.load()) << ",\n";
        json << "    \"blackHqSecondTapDepthBase\": " << formatFloat(internals.blackHqSecondTapDepthBase.load()) << ",\n";
        json << "    \"blackHqSecondTapDepthScale\": " << formatFloat(internals.blackHqSecondTapDepthScale.load()) << ",\n";
        json << "    \"blackHqSecondTapDelayOffsetBase\": " << formatFloat(internals.blackHqSecondTapDelayOffsetBase.load()) << ",\n";
        json << "    \"blackHqSecondTapDelayOffsetScale\": " << formatFloat(internals.blackHqSecondTapDelayOffsetScale.load()) << ",\n";
        json << "    \"bbdDelaySmoothingMs\": " << formatFloat(internals.bbdDelaySmoothingMs.load()) << ",\n";
        json << "    \"bbdDelayMinMs\": " << formatFloat(internals.bbdDelayMinMs.load()) << ",\n";
        json << "    \"bbdDelayMaxMs\": " << formatFloat(internals.bbdDelayMaxMs.load()) << ",\n";
        json << "    \"bbdCentreBaseMs\": " << formatFloat(internals.bbdCentreBaseMs.load()) << ",\n";
        json << "    \"bbdCentreScale\": " << formatFloat(internals.bbdCentreScale.load()) << ",\n";
        json << "    \"bbdDepthMs\": " << formatFloat(internals.bbdDepthMs.load()) << ",\n";
        json << "    \"bbdClockSmoothingMs\": " << formatFloat(internals.bbdClockSmoothingMs.load()) << ",\n";
        json << "    \"bbdFilterSmoothingMs\": " << formatFloat(internals.bbdFilterSmoothingMs.load()) << ",\n";
        json << "    \"bbdFilterCutoffMinHz\": " << formatFloat(internals.bbdFilterCutoffMinHz.load()) << ",\n";
        json << "    \"bbdFilterCutoffMaxHz\": " << formatFloat(internals.bbdFilterCutoffMaxHz.load()) << ",\n";
        json << "    \"bbdFilterCutoffScale\": " << formatFloat(internals.bbdFilterCutoffScale.load()) << ",\n";
        json << "    \"bbdClockMinHz\": " << formatFloat(internals.bbdClockMinHz.load()) << ",\n";
        json << "    \"bbdClockMaxRatio\": " << formatFloat(internals.bbdClockMaxRatio.load()) << ",\n";
        json << "    \"bbdStages\": " << formatFloat(internals.bbdStages.load()) << ",\n";
        json << "    \"bbdFilterMaxRatio\": " << formatFloat(internals.bbdFilterMaxRatio.load()) << ",\n";
        json << "    \"tapeDelaySmoothingMs\": " << formatFloat(internals.tapeDelaySmoothingMs.load()) << ",\n";
        json << "    \"tapeCentreBaseMs\": " << formatFloat(internals.tapeCentreBaseMs.load()) << ",\n";
        json << "    \"tapeCentreScale\": " << formatFloat(internals.tapeCentreScale.load()) << ",\n";
        json << "    \"tapeToneMaxHz\": " << formatFloat(internals.tapeToneMaxHz.load()) << ",\n";
        json << "    \"tapeToneMinHz\": " << formatFloat(internals.tapeToneMinHz.load()) << ",\n";
        json << "    \"tapeToneSmoothingCoeff\": " << formatFloat(internals.tapeToneSmoothingCoeff.load()) << ",\n";
        json << "    \"tapeDriveScale\": " << formatFloat(internals.tapeDriveScale.load()) << ",\n";
        json << "    \"tapeLfoRatioScale\": " << formatFloat(internals.tapeLfoRatioScale.load()) << ",\n";
        json << "    \"tapeLfoModSmoothingCoeff\": " << formatFloat(internals.tapeLfoModSmoothingCoeff.load()) << ",\n";
        json << "    \"tapeRatioSmoothingCoeff\": " << formatFloat(internals.tapeRatioSmoothingCoeff.load()) << ",\n";
        json << "    \"tapePhaseDamping\": " << formatFloat(internals.tapePhaseDamping.load()) << ",\n";
        json << "    \"tapeWowFreqBase\": " << formatFloat(internals.tapeWowFreqBase.load()) << ",\n";
        json << "    \"tapeWowFreqSpread\": " << formatFloat(internals.tapeWowFreqSpread.load()) << ",\n";
        json << "    \"tapeFlutterFreqBase\": " << formatFloat(internals.tapeFlutterFreqBase.load()) << ",\n";
        json << "    \"tapeFlutterFreqSpread\": " << formatFloat(internals.tapeFlutterFreqSpread.load()) << ",\n";
        json << "    \"tapeWowDepthBase\": " << formatFloat(internals.tapeWowDepthBase.load()) << ",\n";
        json << "    \"tapeWowDepthSpread\": " << formatFloat(internals.tapeWowDepthSpread.load()) << ",\n";
        json << "    \"tapeFlutterDepthBase\": " << formatFloat(internals.tapeFlutterDepthBase.load()) << ",\n";
        json << "    \"tapeFlutterDepthSpread\": " << formatFloat(internals.tapeFlutterDepthSpread.load()) << ",\n";
        json << "    \"tapeRatioMin\": " << formatFloat(internals.tapeRatioMin.load()) << ",\n";
        json << "    \"tapeRatioMax\": " << formatFloat(internals.tapeRatioMax.load()) << ",\n";
        json << "    \"tapeWetGain\": " << formatFloat(internals.tapeWetGain.load()) << ",\n";
        json << "    \"tapeHermiteTension\": " << formatFloat(internals.tapeHermiteTension.load()) << "\n";
        json << "  },\n";
    };

    appendInternalsObject("internals", processor.getDspInternals());
    appendInternalsObject("internalsGreen", processor.getEngineDspInternals(0, false));
    appendInternalsObject("internalsBlue", processor.getEngineDspInternals(1, false));
    appendInternalsObject("internalsRed", processor.getEngineDspInternals(2, false));
    appendInternalsObject("internalsPurple", processor.getEngineDspInternals(3, false));
    appendInternalsObject("internalsBlack", processor.getEngineDspInternals(4, false));
    appendInternalsObject("internalsGreenHQ", processor.getEngineDspInternals(0, true));
    appendInternalsObject("internalsBlueHQ", processor.getEngineDspInternals(1, true));
    appendInternalsObject("internalsRedHQ", processor.getEngineDspInternals(2, true));
    appendInternalsObject("internalsPurpleHQ", processor.getEngineDspInternals(3, true));
    appendInternalsObject("internalsBlackHQ", processor.getEngineDspInternals(4, true));

    json << "  \"modularCoresEnabled\": " << (processor.isModularCoresEnabled() ? "true" : "false") << ",\n";
    json << "  \"coreAssignments\": {\n";
    for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
    {
        const juce::String engineToken(choroboros::kEngineColorTokens[static_cast<std::size_t>(engine)]);
        const auto& assignments = processor.getCoreAssignments();
        json << "    \"" << engineToken << "\": {"
             << "\"nq\": \"" << juce::String(choroboros::coreIdToToken(assignments.get(engine, false))) << "\""
             << ", \"hq\": \"" << juce::String(choroboros::coreIdToToken(assignments.get(engine, true))) << "\""
             << "}" << (engine < choroboros::kEngineColorCount - 1 ? ",\n" : "\n");
    }
    json << "  },\n";

    json << "  \"engineParamProfiles\": {\n";
    const char* engineKeys[] = { "green", "blue", "red", "purple", "black" };
    for (int i = 0; i < 5; ++i)
    {
        const auto& p = processor.getEngineParamProfiles()[i];
        json << "    \"" << engineKeys[i] << "\": {"
             << "\"valid\": " << (p.valid ? "true" : "false")
             << ", \"rate\": " << formatFloat(p.rate)
             << ", \"depth\": " << formatFloat(p.depth)
             << ", \"offset\": " << formatFloat(p.offset)
             << ", \"width\": " << formatFloat(p.width)
             << ", \"mix\": " << formatFloat(p.mix)
             << ", \"color\": " << formatFloat(p.color)
             << "}" << (i < 4 ? ",\n" : "\n");
    }
    json << "  },\n";

    json << "  \"layout\": {\n";
    json << "    \"mainKnobSize\": " << layout.mainKnobSizeGreen << ",\n";
    json << "    \"mainKnobSizeGreen\": " << layout.mainKnobSizeGreen << ",\n";
    json << "    \"mainKnobSizeBlue\": " << layout.mainKnobSizeBlue << ",\n";
    json << "    \"mainKnobSizeRed\": " << layout.mainKnobSizeRed << ",\n";
    json << "    \"mainKnobSizePurple\": " << layout.mainKnobSizePurple << ",\n";
    json << "    \"mainKnobSizeBlack\": " << layout.mainKnobSizeBlack << ",\n";
    json << "    \"knobTopY\": " << layout.knobTopYGreen << ",\n";
    json << "    \"knobTopYGreen\": " << layout.knobTopYGreen << ",\n";
    json << "    \"knobTopYBlue\": " << layout.knobTopYBlue << ",\n";
    json << "    \"knobTopYRed\": " << layout.knobTopYRed << ",\n";
    json << "    \"knobTopYPurple\": " << layout.knobTopYPurple << ",\n";
    json << "    \"knobTopYBlack\": " << layout.knobTopYBlack << ",\n";
    json << "    \"rateCenterX\": " << layout.rateCenterXGreen << ",\n";
    json << "    \"rateCenterXGreen\": " << layout.rateCenterXGreen << ",\n";
    json << "    \"rateCenterXBlue\": " << layout.rateCenterXBlue << ",\n";
    json << "    \"rateCenterXRed\": " << layout.rateCenterXRed << ",\n";
    json << "    \"rateCenterXPurple\": " << layout.rateCenterXPurple << ",\n";
    json << "    \"rateCenterXBlack\": " << layout.rateCenterXBlack << ",\n";
    json << "    \"depthCenterX\": " << layout.depthCenterXGreen << ",\n";
    json << "    \"depthCenterXGreen\": " << layout.depthCenterXGreen << ",\n";
    json << "    \"depthCenterXBlue\": " << layout.depthCenterXBlue << ",\n";
    json << "    \"depthCenterXRed\": " << layout.depthCenterXRed << ",\n";
    json << "    \"depthCenterXPurple\": " << layout.depthCenterXPurple << ",\n";
    json << "    \"depthCenterXBlack\": " << layout.depthCenterXBlack << ",\n";
    json << "    \"offsetCenterX\": " << layout.offsetCenterXGreen << ",\n";
    json << "    \"offsetCenterXGreen\": " << layout.offsetCenterXGreen << ",\n";
    json << "    \"offsetCenterXBlue\": " << layout.offsetCenterXBlue << ",\n";
    json << "    \"offsetCenterXRed\": " << layout.offsetCenterXRed << ",\n";
    json << "    \"offsetCenterXPurple\": " << layout.offsetCenterXPurple << ",\n";
    json << "    \"offsetCenterXBlack\": " << layout.offsetCenterXBlack << ",\n";
    json << "    \"widthCenterX\": " << layout.widthCenterXGreen << ",\n";
    json << "    \"widthCenterXGreen\": " << layout.widthCenterXGreen << ",\n";
    json << "    \"widthCenterXBlue\": " << layout.widthCenterXBlue << ",\n";
    json << "    \"widthCenterXRed\": " << layout.widthCenterXRed << ",\n";
    json << "    \"widthCenterXPurple\": " << layout.widthCenterXPurple << ",\n";
    json << "    \"widthCenterXBlack\": " << layout.widthCenterXBlack << ",\n";
    json << "    \"sliderTrackStartX\": " << layout.sliderTrackStartX << ",\n";
    json << "    \"sliderTrackStartY\": " << layout.sliderTrackStartY << ",\n";
    json << "    \"sliderTrackEndX\": " << layout.sliderTrackEndX << ",\n";
    json << "    \"sliderTrackEndY\": " << layout.sliderTrackEndY << ",\n";
    json << "    \"sliderSize\": " << layout.sliderSize << ",\n";
    json << "    \"sliderTrackStartXGreen\": " << layout.sliderTrackStartXGreen << ",\n";
    json << "    \"sliderTrackStartYGreen\": " << layout.sliderTrackStartYGreen << ",\n";
    json << "    \"sliderTrackEndXGreen\": " << layout.sliderTrackEndXGreen << ",\n";
    json << "    \"sliderTrackEndYGreen\": " << layout.sliderTrackEndYGreen << ",\n";
    json << "    \"sliderSizeGreen\": " << layout.sliderSizeGreen << ",\n";
    json << "    \"sliderTrackStartXBlue\": " << layout.sliderTrackStartXBlue << ",\n";
    json << "    \"sliderTrackStartYBlue\": " << layout.sliderTrackStartYBlue << ",\n";
    json << "    \"sliderTrackEndXBlue\": " << layout.sliderTrackEndXBlue << ",\n";
    json << "    \"sliderTrackEndYBlue\": " << layout.sliderTrackEndYBlue << ",\n";
    json << "    \"sliderSizeBlue\": " << layout.sliderSizeBlue << ",\n";
    json << "    \"sliderTrackStartXRed\": " << layout.sliderTrackStartXRed << ",\n";
    json << "    \"sliderTrackStartYRed\": " << layout.sliderTrackStartYRed << ",\n";
    json << "    \"sliderTrackEndXRed\": " << layout.sliderTrackEndXRed << ",\n";
    json << "    \"sliderTrackEndYRed\": " << layout.sliderTrackEndYRed << ",\n";
    json << "    \"sliderSizeRed\": " << layout.sliderSizeRed << ",\n";
    json << "    \"sliderTrackStartXPurple\": " << layout.sliderTrackStartXPurple << ",\n";
    json << "    \"sliderTrackStartYPurple\": " << layout.sliderTrackStartYPurple << ",\n";
    json << "    \"sliderTrackEndXPurple\": " << layout.sliderTrackEndXPurple << ",\n";
    json << "    \"sliderTrackEndYPurple\": " << layout.sliderTrackEndYPurple << ",\n";
    json << "    \"sliderSizePurple\": " << layout.sliderSizePurple << ",\n";
    json << "    \"sliderTrackStartXBlack\": " << layout.sliderTrackStartXBlack << ",\n";
    json << "    \"sliderTrackStartYBlack\": " << layout.sliderTrackStartYBlack << ",\n";
    json << "    \"sliderTrackEndXBlack\": " << layout.sliderTrackEndXBlack << ",\n";
    json << "    \"sliderTrackEndYBlack\": " << layout.sliderTrackEndYBlack << ",\n";
    json << "    \"sliderSizeBlack\": " << layout.sliderSizeBlack << ",\n";
    json << "    \"mixKnobSize\": " << layout.mixKnobSizeGreen << ",\n";
    json << "    \"mixKnobSizeGreen\": " << layout.mixKnobSizeGreen << ",\n";
    json << "    \"mixKnobSizeBlue\": " << layout.mixKnobSizeBlue << ",\n";
    json << "    \"mixKnobSizeRed\": " << layout.mixKnobSizeRed << ",\n";
    json << "    \"mixKnobSizePurple\": " << layout.mixKnobSizePurple << ",\n";
    json << "    \"mixKnobSizeBlack\": " << layout.mixKnobSizeBlack << ",\n";
    json << "    \"mixCenterX\": " << layout.mixCenterXGreen << ",\n";
    json << "    \"mixCenterXGreen\": " << layout.mixCenterXGreen << ",\n";
    json << "    \"mixCenterXBlue\": " << layout.mixCenterXBlue << ",\n";
    json << "    \"mixCenterXRed\": " << layout.mixCenterXRed << ",\n";
    json << "    \"mixCenterXPurple\": " << layout.mixCenterXPurple << ",\n";
    json << "    \"mixCenterXBlack\": " << layout.mixCenterXBlack << ",\n";
    json << "    \"mixKnobY\": " << layout.mixKnobY << ",\n";
    json << "    \"mixKnobYGreen\": " << layout.mixKnobYGreen << ",\n";
    json << "    \"mixKnobYBlue\": " << layout.mixKnobYBlue << ",\n";
    json << "    \"mixKnobYRed\": " << layout.mixKnobYRed << ",\n";
    json << "    \"mixKnobYPurple\": " << layout.mixKnobYPurple << ",\n";
    json << "    \"mixKnobYBlack\": " << layout.mixKnobYBlack << ",\n";
    json << "    \"mixKnobYOffset\": " << layout.mixKnobYOffsetGreen << ",\n";
    json << "    \"mixKnobYOffsetGreen\": " << layout.mixKnobYOffsetGreen << ",\n";
    json << "    \"mixKnobYOffsetBlue\": " << layout.mixKnobYOffsetBlue << ",\n";
    json << "    \"mixKnobYOffsetRed\": " << layout.mixKnobYOffsetRed << ",\n";
    json << "    \"mixKnobYOffsetPurple\": " << layout.mixKnobYOffsetPurple << ",\n";
    json << "    \"mixKnobYOffsetBlack\": " << layout.mixKnobYOffsetBlack << ",\n";
    json << "    \"valueLabelWidth\": " << layout.valueLabelWidth << ",\n";
    json << "    \"valueLabelHeight\": " << layout.valueLabelHeight << ",\n";
    json << "    \"valueLabelY\": " << layout.valueLabelYGreen << ",\n";
    json << "    \"valueLabelYGreen\": " << layout.valueLabelYGreen << ",\n";
    json << "    \"valueLabelYBlue\": " << layout.valueLabelYBlue << ",\n";
    json << "    \"valueLabelYRed\": " << layout.valueLabelYRed << ",\n";
    json << "    \"valueLabelYPurple\": " << layout.valueLabelYPurple << ",\n";
    json << "    \"valueLabelYBlack\": " << layout.valueLabelYBlack << ",\n";
    json << "    \"rateValueOffsetX\": " << layout.rateValueOffsetXGreen << ",\n";
    json << "    \"rateValueOffsetXGreen\": " << layout.rateValueOffsetXGreen << ",\n";
    json << "    \"rateValueOffsetXBlue\": " << layout.rateValueOffsetXBlue << ",\n";
    json << "    \"rateValueOffsetXRed\": " << layout.rateValueOffsetXRed << ",\n";
    json << "    \"rateValueOffsetXPurple\": " << layout.rateValueOffsetXPurple << ",\n";
    json << "    \"rateValueOffsetXBlack\": " << layout.rateValueOffsetXBlack << ",\n";
    json << "    \"depthValueOffsetX\": " << layout.depthValueOffsetXGreen << ",\n";
    json << "    \"depthValueOffsetXGreen\": " << layout.depthValueOffsetXGreen << ",\n";
    json << "    \"depthValueOffsetXBlue\": " << layout.depthValueOffsetXBlue << ",\n";
    json << "    \"depthValueOffsetXRed\": " << layout.depthValueOffsetXRed << ",\n";
    json << "    \"depthValueOffsetXPurple\": " << layout.depthValueOffsetXPurple << ",\n";
    json << "    \"depthValueOffsetXBlack\": " << layout.depthValueOffsetXBlack << ",\n";
    json << "    \"offsetValueOffsetX\": " << layout.offsetValueOffsetXGreen << ",\n";
    json << "    \"offsetValueOffsetXGreen\": " << layout.offsetValueOffsetXGreen << ",\n";
    json << "    \"offsetValueOffsetXBlue\": " << layout.offsetValueOffsetXBlue << ",\n";
    json << "    \"offsetValueOffsetXRed\": " << layout.offsetValueOffsetXRed << ",\n";
    json << "    \"offsetValueOffsetXPurple\": " << layout.offsetValueOffsetXPurple << ",\n";
    json << "    \"offsetValueOffsetXBlack\": " << layout.offsetValueOffsetXBlack << ",\n";
    json << "    \"widthValueOffsetX\": " << layout.widthValueOffsetXGreen << ",\n";
    json << "    \"widthValueOffsetXGreen\": " << layout.widthValueOffsetXGreen << ",\n";
    json << "    \"widthValueOffsetXBlue\": " << layout.widthValueOffsetXBlue << ",\n";
    json << "    \"widthValueOffsetXRed\": " << layout.widthValueOffsetXRed << ",\n";
    json << "    \"widthValueOffsetXPurple\": " << layout.widthValueOffsetXPurple << ",\n";
    json << "    \"widthValueOffsetXBlack\": " << layout.widthValueOffsetXBlack << ",\n";
    json << "    \"colorValueCenterX\": " << layout.colorValueCenterX << ",\n";
    json << "    \"colorValueWidth\": " << layout.colorValueWidth << ",\n";
    json << "    \"colorValueHeight\": " << layout.colorValueHeight << ",\n";
    json << "    \"colorValueY\": " << layout.colorValueYGreen << ",\n";
    json << "    \"colorValueYGreen\": " << layout.colorValueYGreen << ",\n";
    json << "    \"colorValueYBlue\": " << layout.colorValueYBlue << ",\n";
    json << "    \"colorValueYRed\": " << layout.colorValueYRed << ",\n";
    json << "    \"colorValueYPurple\": " << layout.colorValueYPurple << ",\n";
    json << "    \"colorValueYBlack\": " << layout.colorValueYBlack << ",\n";
    json << "    \"colorValueXOffset\": " << layout.colorValueXOffsetGreen << ",\n";
    json << "    \"colorValueXOffsetGreen\": " << layout.colorValueXOffsetGreen << ",\n";
    json << "    \"colorValueXOffsetBlue\": " << layout.colorValueXOffsetBlue << ",\n";
    json << "    \"colorValueXOffsetRed\": " << layout.colorValueXOffsetRed << ",\n";
    json << "    \"colorValueXOffsetPurple\": " << layout.colorValueXOffsetPurple << ",\n";
    json << "    \"colorValueXOffsetBlack\": " << layout.colorValueXOffsetBlack << ",\n";
    json << "    \"mixValueY\": " << layout.mixValueYGreen << ",\n";
    json << "    \"mixValueYGreen\": " << layout.mixValueYGreen << ",\n";
    json << "    \"mixValueYBlue\": " << layout.mixValueYBlue << ",\n";
    json << "    \"mixValueYRed\": " << layout.mixValueYRed << ",\n";
    json << "    \"mixValueYPurple\": " << layout.mixValueYPurple << ",\n";
    json << "    \"mixValueYBlack\": " << layout.mixValueYBlack << ",\n";
    json << "    \"mixValueWidth\": " << layout.mixValueWidth << ",\n";
    json << "    \"mixValueHeight\": " << layout.mixValueHeight << ",\n";
    json << "    \"mixValueOffsetX\": " << layout.mixValueOffsetXGreen << ",\n";
    json << "    \"mixValueOffsetXGreen\": " << layout.mixValueOffsetXGreen << ",\n";
    json << "    \"mixValueOffsetXBlue\": " << layout.mixValueOffsetXBlue << ",\n";
    json << "    \"mixValueOffsetXRed\": " << layout.mixValueOffsetXRed << ",\n";
    json << "    \"mixValueOffsetXPurple\": " << layout.mixValueOffsetXPurple << ",\n";
    json << "    \"mixValueOffsetXBlack\": " << layout.mixValueOffsetXBlack << ",\n";
    json << "    \"rateValueOffsetY\": " << layout.rateValueOffsetY << ",\n";
    json << "    \"depthValueOffsetY\": " << layout.depthValueOffsetY << ",\n";
    json << "    \"offsetValueOffsetY\": " << layout.offsetValueOffsetY << ",\n";
    json << "    \"widthValueOffsetY\": " << layout.widthValueOffsetY << ",\n";
    json << "    \"knobValueFontSize\": " << layout.knobValueFontSize << ",\n";
    json << "    \"colorValueFontSize\": " << layout.colorValueFontSize << ",\n";
    json << "    \"mixValueFontSize\": " << layout.mixValueFontSize << ",\n";
    json << "    \"valueTextAlphaPct\": " << layout.valueTextAlphaPct << ",\n";
    json << "    \"valueTextColourMode\": " << layout.valueTextColourMode << ",\n";
    json << "    \"valueTextColour\": " << layout.valueTextColour << ",\n";
    json << "    \"topButtonsWidth\": " << layout.topButtonsWidth << ",\n";
    json << "    \"topButtonsHeight\": " << layout.topButtonsHeight << ",\n";
    json << "    \"topButtonsGap\": " << layout.topButtonsGap << ",\n";
    json << "    \"topButtonsRightMargin\": " << layout.topButtonsRightMargin << ",\n";
    json << "    \"topButtonsTopY\": " << layout.topButtonsTopY << ",\n";
    json << "    \"topButtonsFontSize\": " << layout.topButtonsFontSize << ",\n";
    json << "    \"topButtonsTextColour\": " << layout.topButtonsTextColour << ",\n";
    json << "    \"topButtonsBackgroundColour\": " << layout.topButtonsBackgroundColour << ",\n";
    json << "    \"topButtonsOnBackgroundColour\": " << layout.topButtonsOnBackgroundColour << ",\n";
    json << "    \"engineSelectorX\": " << layout.engineSelectorX << ",\n";
    json << "    \"engineSelectorY\": " << layout.engineSelectorY << ",\n";
    json << "    \"engineSelectorW\": " << layout.engineSelectorW << ",\n";
    json << "    \"engineSelectorH\": " << layout.engineSelectorH << ",\n";
    json << "    \"engineSelectorFontSize\": " << layout.engineSelectorFontSize << ",\n";
    json << "    \"engineSelectorTextColour\": " << layout.engineSelectorTextColour << ",\n";
    json << "    \"engineSelectorBackgroundColour\": " << layout.engineSelectorBackgroundColour << ",\n";
    json << "    \"engineSelectorOutlineColour\": " << layout.engineSelectorOutlineColour << ",\n";
    json << "    \"engineSelectorArrowColour\": " << layout.engineSelectorArrowColour << ",\n";
    json << "    \"engineSelectorPopupBackgroundColour\": " << layout.engineSelectorPopupBackgroundColour << ",\n";
    json << "    \"engineSelectorPopupTextColour\": " << layout.engineSelectorPopupTextColour << ",\n";
    json << "    \"engineSelectorPopupHighlightedBackgroundColour\": " << layout.engineSelectorPopupHighlightedBackgroundColour << ",\n";
    json << "    \"engineSelectorPopupHighlightedTextColour\": " << layout.engineSelectorPopupHighlightedTextColour << ",\n";
    json << "    \"hqSwitchSize\": " << layout.hqSwitchSize << ",\n";
    json << "    \"hqSwitchOffsetX\": " << layout.hqSwitchOffsetX << ",\n";
    json << "    \"hqSwitchOffsetY\": " << layout.hqSwitchOffsetY << ",\n";
    json << "    \"rateKnobVisualResponseMs\": " << layout.rateKnobVisualResponseMs << ",\n";
    json << "    \"depthKnobVisualResponseMs\": " << layout.depthKnobVisualResponseMs << ",\n";
    json << "    \"offsetKnobVisualResponseMs\": " << layout.offsetKnobVisualResponseMs << ",\n";
    json << "    \"widthKnobVisualResponseMs\": " << layout.widthKnobVisualResponseMs << ",\n";
    json << "    \"mixKnobVisualResponseMs\": " << layout.mixKnobVisualResponseMs << ",\n";
    json << "    \"knobDragSensitivityPct\": " << layout.knobDragSensitivityPct << ",\n";
    json << "    \"knobRollOffSpeedPct\": " << layout.knobRollOffSpeedPct << ",\n";
    json << "    \"knobSweepStartDeg\": " << layout.knobSweepStartDeg << ",\n";
    json << "    \"knobSweepEndDeg\": " << layout.knobSweepEndDeg << ",\n";
    json << "    \"knobFrameCount\": " << layout.knobFrameCount << ",\n";
    json << "    \"valueFxEnabled\": " << layout.valueFxEnabled << ",\n";
    json << "    \"valueGlowAlphaPct\": " << layout.valueGlowAlphaPct << ",\n";
    json << "    \"valueGlowSpreadPxTimes100\": " << layout.valueGlowSpreadPxTimes100 << ",\n";
    json << "    \"valueFxPerCharOffsetXPxTimes100\": " << layout.valueFxPerCharOffsetXPxTimes100 << ",\n";
    json << "    \"valueFxPerCharOffsetYPxTimes100\": " << layout.valueFxPerCharOffsetYPxTimes100 << ",\n";
    json << "    \"valueTopReflectAlphaPct\": " << layout.valueTopReflectAlphaPct << ",\n";
    json << "    \"valueTopReflectOffsetXPxTimes100\": " << layout.valueTopReflectOffsetXPxTimes100 << ",\n";
    json << "    \"valueTopReflectOffsetYPxTimes100\": " << layout.valueTopReflectOffsetYPxTimes100 << ",\n";
    json << "    \"valueTopReflectShearPct\": " << layout.valueTopReflectShearPct << ",\n";
    json << "    \"valueTopReflectRotateDeg\": " << layout.valueTopReflectRotateDeg << ",\n";
    json << "    \"valueBottomReflectAlphaPct\": " << layout.valueBottomReflectAlphaPct << ",\n";
    json << "    \"valueBottomReflectOffsetXPxTimes100\": " << layout.valueBottomReflectOffsetXPxTimes100 << ",\n";
    json << "    \"valueBottomReflectOffsetYPxTimes100\": " << layout.valueBottomReflectOffsetYPxTimes100 << ",\n";
    json << "    \"valueBottomReflectShearPct\": " << layout.valueBottomReflectShearPct << ",\n";
    json << "    \"valueBottomReflectRotateDeg\": " << layout.valueBottomReflectRotateDeg << ",\n";
    json << "    \"valueReflectBlurPxTimes100\": " << layout.valueReflectBlurPxTimes100 << ",\n";
    json << "    \"valueReflectSquashPct\": " << layout.valueReflectSquashPct << ",\n";
    json << "    \"valueReflectMotionPct\": " << layout.valueReflectMotionPct << ",\n";
    json << "    \"colorValueFxEnabled\": " << layout.colorValueFxEnabled << ",\n";
    json << "    \"colorValueGlowAlphaPct\": " << layout.colorValueGlowAlphaPct << ",\n";
    json << "    \"colorValueGlowSpreadPxTimes100\": " << layout.colorValueGlowSpreadPxTimes100 << ",\n";
    json << "    \"colorValueFxPerCharOffsetXPxTimes100\": " << layout.colorValueFxPerCharOffsetXPxTimes100 << ",\n";
    json << "    \"colorValueFxPerCharOffsetYPxTimes100\": " << layout.colorValueFxPerCharOffsetYPxTimes100 << ",\n";
    json << "    \"colorValueTopReflectAlphaPct\": " << layout.colorValueTopReflectAlphaPct << ",\n";
    json << "    \"colorValueTopReflectOffsetXPxTimes100\": " << layout.colorValueTopReflectOffsetXPxTimes100 << ",\n";
    json << "    \"colorValueTopReflectOffsetYPxTimes100\": " << layout.colorValueTopReflectOffsetYPxTimes100 << ",\n";
    json << "    \"colorValueTopReflectShearPct\": " << layout.colorValueTopReflectShearPct << ",\n";
    json << "    \"colorValueTopReflectRotateDeg\": " << layout.colorValueTopReflectRotateDeg << ",\n";
    json << "    \"colorValueBottomReflectAlphaPct\": " << layout.colorValueBottomReflectAlphaPct << ",\n";
    json << "    \"colorValueBottomReflectOffsetXPxTimes100\": " << layout.colorValueBottomReflectOffsetXPxTimes100 << ",\n";
    json << "    \"colorValueBottomReflectOffsetYPxTimes100\": " << layout.colorValueBottomReflectOffsetYPxTimes100 << ",\n";
    json << "    \"colorValueBottomReflectShearPct\": " << layout.colorValueBottomReflectShearPct << ",\n";
    json << "    \"colorValueBottomReflectRotateDeg\": " << layout.colorValueBottomReflectRotateDeg << ",\n";
    json << "    \"colorValueReflectBlurPxTimes100\": " << layout.colorValueReflectBlurPxTimes100 << ",\n";
    json << "    \"colorValueReflectSquashPct\": " << layout.colorValueReflectSquashPct << ",\n";
    json << "    \"colorValueReflectMotionPct\": " << layout.colorValueReflectMotionPct << ",\n";
    json << "    \"mixValueFxEnabled\": " << layout.mixValueFxEnabled << ",\n";
    json << "    \"mixValueGlowAlphaPct\": " << layout.mixValueGlowAlphaPct << ",\n";
    json << "    \"mixValueGlowSpreadPxTimes100\": " << layout.mixValueGlowSpreadPxTimes100 << ",\n";
    json << "    \"mixValueFxPerCharOffsetXPxTimes100\": " << layout.mixValueFxPerCharOffsetXPxTimes100 << ",\n";
    json << "    \"mixValueFxPerCharOffsetYPxTimes100\": " << layout.mixValueFxPerCharOffsetYPxTimes100 << ",\n";
    json << "    \"mixValueTopReflectAlphaPct\": " << layout.mixValueTopReflectAlphaPct << ",\n";
    json << "    \"mixValueTopReflectOffsetXPxTimes100\": " << layout.mixValueTopReflectOffsetXPxTimes100 << ",\n";
    json << "    \"mixValueTopReflectOffsetYPxTimes100\": " << layout.mixValueTopReflectOffsetYPxTimes100 << ",\n";
    json << "    \"mixValueTopReflectShearPct\": " << layout.mixValueTopReflectShearPct << ",\n";
    json << "    \"mixValueTopReflectRotateDeg\": " << layout.mixValueTopReflectRotateDeg << ",\n";
    json << "    \"mixValueBottomReflectAlphaPct\": " << layout.mixValueBottomReflectAlphaPct << ",\n";
    json << "    \"mixValueBottomReflectOffsetXPxTimes100\": " << layout.mixValueBottomReflectOffsetXPxTimes100 << ",\n";
    json << "    \"mixValueBottomReflectOffsetYPxTimes100\": " << layout.mixValueBottomReflectOffsetYPxTimes100 << ",\n";
    json << "    \"mixValueBottomReflectShearPct\": " << layout.mixValueBottomReflectShearPct << ",\n";
    json << "    \"mixValueBottomReflectRotateDeg\": " << layout.mixValueBottomReflectRotateDeg << ",\n";
    json << "    \"mixValueReflectBlurPxTimes100\": " << layout.mixValueReflectBlurPxTimes100 << ",\n";
    json << "    \"mixValueReflectSquashPct\": " << layout.mixValueReflectSquashPct << ",\n";
    json << "    \"mixValueReflectMotionPct\": " << layout.mixValueReflectMotionPct << ",\n";
    json << "    \"mainValueFlipEnabled\": " << layout.mainValueFlipEnabled << ",\n";
    json << "    \"mainValueFlipDurationMs\": " << layout.mainValueFlipDurationMs << ",\n";
    json << "    \"mainValueFlipTravelUpPxTimes100\": " << layout.mainValueFlipTravelUpPxTimes100 << ",\n";
    json << "    \"mainValueFlipTravelDownPxTimes100\": " << layout.mainValueFlipTravelDownPxTimes100 << ",\n";
    json << "    \"mainValueFlipTravelOutPct\": " << layout.mainValueFlipTravelOutPct << ",\n";
    json << "    \"mainValueFlipTravelInPct\": " << layout.mainValueFlipTravelInPct << ",\n";
    json << "    \"mainValueFlipShearPct\": " << layout.mainValueFlipShearPct << ",\n";
    json << "    \"mainValueFlipMinScalePct\": " << layout.mainValueFlipMinScalePct << ",\n";
    json << "    \"colorValueFlipEnabled\": " << layout.colorValueFlipEnabled << ",\n";
    json << "    \"colorValueFlipDurationMs\": " << layout.colorValueFlipDurationMs << ",\n";
    json << "    \"colorValueFlipTravelUpPxTimes100\": " << layout.colorValueFlipTravelUpPxTimes100 << ",\n";
    json << "    \"colorValueFlipTravelDownPxTimes100\": " << layout.colorValueFlipTravelDownPxTimes100 << ",\n";
    json << "    \"colorValueFlipTravelOutPct\": " << layout.colorValueFlipTravelOutPct << ",\n";
    json << "    \"colorValueFlipTravelInPct\": " << layout.colorValueFlipTravelInPct << ",\n";
    json << "    \"colorValueFlipShearPct\": " << layout.colorValueFlipShearPct << ",\n";
    json << "    \"colorValueFlipMinScalePct\": " << layout.colorValueFlipMinScalePct << ",\n";
    json << "    \"mixValueFlipEnabled\": " << layout.mixValueFlipEnabled << ",\n";
    json << "    \"mixValueFlipDurationMs\": " << layout.mixValueFlipDurationMs << ",\n";
    json << "    \"mixValueFlipTravelUpPxTimes100\": " << layout.mixValueFlipTravelUpPxTimes100 << ",\n";
    json << "    \"mixValueFlipTravelDownPxTimes100\": " << layout.mixValueFlipTravelDownPxTimes100 << ",\n";
    json << "    \"mixValueFlipTravelOutPct\": " << layout.mixValueFlipTravelOutPct << ",\n";
    json << "    \"mixValueFlipTravelInPct\": " << layout.mixValueFlipTravelInPct << ",\n";
    json << "    \"mixValueFlipShearPct\": " << layout.mixValueFlipShearPct << ",\n";
    json << "    \"mixValueFlipMinScalePct\": " << layout.mixValueFlipMinScalePct;

    const auto appendLayoutInt = [&json](const juce::String& key, int value)
    {
        json << ",\n    \"" << key << "\": " << value;
    };

    appendLayoutInt("rateValueOffsetYGreen", layout.rateValueOffsetYGreen);
    appendLayoutInt("rateValueOffsetYBlue", layout.rateValueOffsetYBlue);
    appendLayoutInt("rateValueOffsetYRed", layout.rateValueOffsetYRed);
    appendLayoutInt("rateValueOffsetYPurple", layout.rateValueOffsetYPurple);
    appendLayoutInt("rateValueOffsetYBlack", layout.rateValueOffsetYBlack);
    appendLayoutInt("depthValueOffsetYGreen", layout.depthValueOffsetYGreen);
    appendLayoutInt("depthValueOffsetYBlue", layout.depthValueOffsetYBlue);
    appendLayoutInt("depthValueOffsetYRed", layout.depthValueOffsetYRed);
    appendLayoutInt("depthValueOffsetYPurple", layout.depthValueOffsetYPurple);
    appendLayoutInt("depthValueOffsetYBlack", layout.depthValueOffsetYBlack);
    appendLayoutInt("offsetValueOffsetYGreen", layout.offsetValueOffsetYGreen);
    appendLayoutInt("offsetValueOffsetYBlue", layout.offsetValueOffsetYBlue);
    appendLayoutInt("offsetValueOffsetYRed", layout.offsetValueOffsetYRed);
    appendLayoutInt("offsetValueOffsetYPurple", layout.offsetValueOffsetYPurple);
    appendLayoutInt("offsetValueOffsetYBlack", layout.offsetValueOffsetYBlack);
    appendLayoutInt("widthValueOffsetYGreen", layout.widthValueOffsetYGreen);
    appendLayoutInt("widthValueOffsetYBlue", layout.widthValueOffsetYBlue);
    appendLayoutInt("widthValueOffsetYRed", layout.widthValueOffsetYRed);
    appendLayoutInt("widthValueOffsetYPurple", layout.widthValueOffsetYPurple);
    appendLayoutInt("widthValueOffsetYBlack", layout.widthValueOffsetYBlack);
    appendLayoutInt("hqSwitchOffsetXGreen", layout.hqSwitchOffsetXGreen);
    appendLayoutInt("hqSwitchOffsetXBlue", layout.hqSwitchOffsetXBlue);
    appendLayoutInt("hqSwitchOffsetXRed", layout.hqSwitchOffsetXRed);
    appendLayoutInt("hqSwitchOffsetXPurple", layout.hqSwitchOffsetXPurple);
    appendLayoutInt("hqSwitchOffsetXBlack", layout.hqSwitchOffsetXBlack);
    appendLayoutInt("hqSwitchOffsetYGreen", layout.hqSwitchOffsetYGreen);
    appendLayoutInt("hqSwitchOffsetYBlue", layout.hqSwitchOffsetYBlue);
    appendLayoutInt("hqSwitchOffsetYRed", layout.hqSwitchOffsetYRed);
    appendLayoutInt("hqSwitchOffsetYPurple", layout.hqSwitchOffsetYPurple);
    appendLayoutInt("hqSwitchOffsetYBlack", layout.hqSwitchOffsetYBlack);

    const std::array<juce::String, LayoutTuning::engineCount> engineSuffixes { { "Green", "Blue", "Red", "Purple", "Black" } };
    const std::array<juce::String, LayoutTuning::mainValueFieldCount> fieldPrefixes { { "Rate", "Depth", "Offset", "Width" } };
    for (int engineIndex = 0; engineIndex < LayoutTuning::engineCount; ++engineIndex)
    {
        for (int fieldIndex = 0; fieldIndex < LayoutTuning::mainValueFieldCount; ++fieldIndex)
        {
            const auto& anim = layout.mainValueAnimationsByEngine[static_cast<std::size_t>(engineIndex)][static_cast<std::size_t>(fieldIndex)];
            const auto key = [&fieldPrefixes, &engineSuffixes, fieldIndex, engineIndex](const juce::String& suffix)
            {
                return "mainValue" + fieldPrefixes[static_cast<std::size_t>(fieldIndex)] + suffix
                       + juce::String(engineSuffixes[static_cast<std::size_t>(engineIndex)]);
            };

            appendLayoutInt(key("FxEnabled"), anim.fx.enabled);
            appendLayoutInt(key("GlowAlphaPct"), anim.fx.glowAlphaPct);
            appendLayoutInt(key("GlowSpreadPxTimes100"), anim.fx.glowSpreadPxTimes100);
            appendLayoutInt(key("PerCharOffsetXPxTimes100"), anim.fx.perCharOffsetXPxTimes100);
            appendLayoutInt(key("PerCharOffsetYPxTimes100"), anim.fx.perCharOffsetYPxTimes100);
            appendLayoutInt(key("TopReflectAlphaPct"), anim.fx.topReflectAlphaPct);
            appendLayoutInt(key("TopReflectOffsetXPxTimes100"), anim.fx.topReflectOffsetXPxTimes100);
            appendLayoutInt(key("TopReflectOffsetYPxTimes100"), anim.fx.topReflectOffsetYPxTimes100);
            appendLayoutInt(key("TopReflectShearPct"), anim.fx.topReflectShearPct);
            appendLayoutInt(key("TopReflectRotateDeg"), anim.fx.topReflectRotateDeg);
            appendLayoutInt(key("BottomReflectAlphaPct"), anim.fx.bottomReflectAlphaPct);
            appendLayoutInt(key("BottomReflectOffsetXPxTimes100"), anim.fx.bottomReflectOffsetXPxTimes100);
            appendLayoutInt(key("BottomReflectOffsetYPxTimes100"), anim.fx.bottomReflectOffsetYPxTimes100);
            appendLayoutInt(key("BottomReflectShearPct"), anim.fx.bottomReflectShearPct);
            appendLayoutInt(key("BottomReflectRotateDeg"), anim.fx.bottomReflectRotateDeg);
            appendLayoutInt(key("ReflectBlurPxTimes100"), anim.fx.reflectBlurPxTimes100);
            appendLayoutInt(key("ReflectSquashPct"), anim.fx.reflectSquashPct);
            appendLayoutInt(key("ReflectMotionPct"), anim.fx.reflectMotionPct);
            appendLayoutInt(key("FlipEnabled"), anim.flip.enabled);
            appendLayoutInt(key("FlipDurationMs"), anim.flip.durationMs);
            appendLayoutInt(key("FlipTravelUpPxTimes100"), anim.flip.travelUpPxTimes100);
            appendLayoutInt(key("FlipTravelDownPxTimes100"), anim.flip.travelDownPxTimes100);
            appendLayoutInt(key("FlipTravelOutPct"), anim.flip.travelOutPct);
            appendLayoutInt(key("FlipTravelInPct"), anim.flip.travelInPct);
            appendLayoutInt(key("FlipShearPct"), anim.flip.shearPct);
            appendLayoutInt(key("FlipMinScalePct"), anim.flip.minScalePct);
        }
    }
    json << "\n";
    json << "  }\n}\n";

    return json;
}

void DevPanel::setEditingLocked(bool shouldLock)
{
    editingLocked = shouldLock;
    for (auto* property : lockableProperties)
    {
        if (auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(property))
            lockable->setLocked(editingLocked);
    }
    lockToggleButton.setButtonText(editingLocked ? "Unlock All" : "Lock All");
}

void DevPanel::saveCurrentAsDefaults()
{
    saveDefaultsButton.setEnabled(false);
    const auto json = buildJson();

    juce::String err;
    const bool ok = DefaultsPersistence::saveUser(json, &err);

    if (ok)
    {
        const auto roundTrip = juce::JSON::parse(DefaultsPersistence::loadUser());
        const auto generated = juce::JSON::parse(json);
        const auto* root = roundTrip.getDynamicObject();
        const bool semanticMatch = !generated.isVoid() && varsEquivalent(generated, roundTrip);
        const bool hasCoreSections = (root != nullptr
            && root->hasProperty("tuning") && root->hasProperty("layout")
            && root->hasProperty("coreAssignments")
            && root->hasProperty("modularCoresEnabled")
            && root->hasProperty("engineParamProfiles")
            && root->hasProperty("internalsGreen") && root->hasProperty("internalsGreenHQ")
            && root->hasProperty("internalsBlue") && root->hasProperty("internalsBlueHQ")
            && root->hasProperty("internalsRed") && root->hasProperty("internalsRedHQ")
            && root->hasProperty("internalsPurple") && root->hasProperty("internalsPurpleHQ")
            && root->hasProperty("internalsBlack") && root->hasProperty("internalsBlackHQ"));

        if (semanticMatch && hasCoreSections)
            saveDefaultsButton.setButtonText("Defaults Saved (Verified)");
        else if (!semanticMatch)
            saveDefaultsButton.setButtonText("Save Failed (Mismatch)");
        else
            saveDefaultsButton.setButtonText("Save Failed (Incomplete)");
    }
    else
    {
        const bool recoveryExists = DefaultsPersistence::getRecoveryFile(false).existsAsFile();
        saveDefaultsButton.setButtonText(recoveryExists ? "Save Failed (Recovery Saved)" : "Save Failed");
    }
    triggerSaveButtonReset();
}

void DevPanel::triggerSaveButtonReset()
{
    saveButtonResetCountdownTicks = 20; // ~2 seconds at 10 Hz timer
}

void DevPanel::resetToFactoryDefaults()
{
    resetFactoryButton.setEnabled(false);

    processor.resetToFactoryDefaults();
    editor.resetLayoutToFactoryDefaults();

    const auto factoryJson = buildJson();
    juce::String factoryErr;
    juce::String userErr;
    const bool factoryOk = DefaultsPersistence::saveFactory(factoryJson, &factoryErr);
    const bool userOk = DefaultsPersistence::saveUser(factoryJson, &userErr);

    auto refreshPanel = [](juce::PropertyPanel& panel) { panel.refreshAll(); };
    refreshPanel(mappingPanel);
    refreshPanel(uiPanel);
    refreshPanel(overviewPanel);
    refreshPanel(modulationPanel);
    refreshPanel(tonePanel);
    refreshPanel(enginePanel);
    refreshPanel(validationPanel);
    refreshPanel(internalsGreenNqPanel);
    refreshPanel(internalsGreenHqPanel);
    refreshPanel(internalsBlueNqPanel);
    refreshPanel(internalsBlueHqPanel);
    refreshPanel(internalsRedNqPanel);
    refreshPanel(internalsRedHqPanel);
    refreshPanel(internalsPurpleNqPanel);
    refreshPanel(internalsPurpleHqPanel);
    refreshPanel(internalsBlackNqPanel);
    refreshPanel(internalsBlackHqPanel);
    refreshPanel(bbdPanel);
    refreshPanel(tapePanel);
    refreshPanel(layoutGlobalPanel);
    refreshPanel(layoutTextAnimationPanel);
    refreshPanel(layoutGreenPanel);
    refreshPanel(layoutBluePanel);
    refreshPanel(layoutRedPanel);
    refreshPanel(layoutPurplePanel);
    refreshPanel(layoutBlackPanel);

    updateActiveProfileLabel();
    updateRightTabVisibility();
    refreshSecondaryTabButtons();
    resized();

    if (factoryOk && userOk)
        resetFactoryButton.setButtonText("Factory Reset (Saved)");
    else if (userOk)
        resetFactoryButton.setButtonText("Factory Save Failed");
    else
        resetFactoryButton.setButtonText("Reset Failed");

    triggerResetFactoryButtonReset();
}

void DevPanel::triggerResetFactoryButtonReset()
{
    resetFactoryButtonResetCountdownTicks = 20; // ~2 seconds at 10 Hz timer
}

void DevPanel::applyValueFxPreset(int presetId)
{
    auto& layout = editor.getLayoutTuning();

    if (presetId == 0) // Off
    {
        layout.valueFxEnabled = 1;
        layout.valueGlowAlphaPct = 0;
        layout.valueGlowSpreadPxTimes100 = 0;
        layout.valueFxPerCharOffsetXPxTimes100 = 0;
        layout.valueFxPerCharOffsetYPxTimes100 = 0;
        layout.valueTopReflectAlphaPct = 0;
        layout.valueTopReflectOffsetXPxTimes100 = 0;
        layout.valueTopReflectOffsetYPxTimes100 = 0;
        layout.valueTopReflectShearPct = 0;
        layout.valueTopReflectRotateDeg = 0;
        layout.valueBottomReflectAlphaPct = 0;
        layout.valueBottomReflectOffsetXPxTimes100 = 0;
        layout.valueBottomReflectOffsetYPxTimes100 = 0;
        layout.valueBottomReflectShearPct = 0;
        layout.valueBottomReflectRotateDeg = 0;
        layout.valueReflectBlurPxTimes100 = 0;
        layout.valueReflectSquashPct = 0;
        layout.valueReflectMotionPct = 0;
        layout.colorValueFxEnabled = layout.valueFxEnabled;
        layout.colorValueGlowAlphaPct = layout.valueGlowAlphaPct;
        layout.colorValueGlowSpreadPxTimes100 = layout.valueGlowSpreadPxTimes100;
        layout.colorValueFxPerCharOffsetXPxTimes100 = layout.valueFxPerCharOffsetXPxTimes100;
        layout.colorValueFxPerCharOffsetYPxTimes100 = layout.valueFxPerCharOffsetYPxTimes100;
        layout.colorValueTopReflectAlphaPct = layout.valueTopReflectAlphaPct;
        layout.colorValueTopReflectOffsetXPxTimes100 = layout.valueTopReflectOffsetXPxTimes100;
        layout.colorValueTopReflectOffsetYPxTimes100 = layout.valueTopReflectOffsetYPxTimes100;
        layout.colorValueTopReflectShearPct = layout.valueTopReflectShearPct;
        layout.colorValueTopReflectRotateDeg = layout.valueTopReflectRotateDeg;
        layout.colorValueBottomReflectAlphaPct = layout.valueBottomReflectAlphaPct;
        layout.colorValueBottomReflectOffsetXPxTimes100 = layout.valueBottomReflectOffsetXPxTimes100;
        layout.colorValueBottomReflectOffsetYPxTimes100 = layout.valueBottomReflectOffsetYPxTimes100;
        layout.colorValueBottomReflectShearPct = layout.valueBottomReflectShearPct;
        layout.colorValueBottomReflectRotateDeg = layout.valueBottomReflectRotateDeg;
        layout.colorValueReflectBlurPxTimes100 = layout.valueReflectBlurPxTimes100;
        layout.colorValueReflectSquashPct = layout.valueReflectSquashPct;
        layout.colorValueReflectMotionPct = layout.valueReflectMotionPct;
        layout.mixValueFxEnabled = layout.valueFxEnabled;
        layout.mixValueGlowAlphaPct = layout.valueGlowAlphaPct;
        layout.mixValueGlowSpreadPxTimes100 = layout.valueGlowSpreadPxTimes100;
        layout.mixValueFxPerCharOffsetXPxTimes100 = layout.valueFxPerCharOffsetXPxTimes100;
        layout.mixValueFxPerCharOffsetYPxTimes100 = layout.valueFxPerCharOffsetYPxTimes100;
        layout.mixValueTopReflectAlphaPct = layout.valueTopReflectAlphaPct;
        layout.mixValueTopReflectOffsetXPxTimes100 = layout.valueTopReflectOffsetXPxTimes100;
        layout.mixValueTopReflectOffsetYPxTimes100 = layout.valueTopReflectOffsetYPxTimes100;
        layout.mixValueTopReflectShearPct = layout.valueTopReflectShearPct;
        layout.mixValueTopReflectRotateDeg = layout.valueTopReflectRotateDeg;
        layout.mixValueBottomReflectAlphaPct = layout.valueBottomReflectAlphaPct;
        layout.mixValueBottomReflectOffsetXPxTimes100 = layout.valueBottomReflectOffsetXPxTimes100;
        layout.mixValueBottomReflectOffsetYPxTimes100 = layout.valueBottomReflectOffsetYPxTimes100;
        layout.mixValueBottomReflectShearPct = layout.valueBottomReflectShearPct;
        layout.mixValueBottomReflectRotateDeg = layout.valueBottomReflectRotateDeg;
        layout.mixValueReflectBlurPxTimes100 = layout.valueReflectBlurPxTimes100;
        layout.mixValueReflectSquashPct = layout.valueReflectSquashPct;
        layout.mixValueReflectMotionPct = layout.valueReflectMotionPct;
    }
    else if (presetId == 1) // Subtle
    {
        layout.valueFxEnabled = 1;
        layout.valueGlowAlphaPct = 6;
        layout.valueGlowSpreadPxTimes100 = 55;
        layout.valueFxPerCharOffsetXPxTimes100 = 0;
        layout.valueFxPerCharOffsetYPxTimes100 = 0;
        layout.valueTopReflectAlphaPct = 4;
        layout.valueTopReflectOffsetXPxTimes100 = 0;
        layout.valueTopReflectOffsetYPxTimes100 = -70;
        layout.valueTopReflectShearPct = 7;
        layout.valueTopReflectRotateDeg = 0;
        layout.valueBottomReflectAlphaPct = 6;
        layout.valueBottomReflectOffsetXPxTimes100 = 0;
        layout.valueBottomReflectOffsetYPxTimes100 = 95;
        layout.valueBottomReflectShearPct = -9;
        layout.valueBottomReflectRotateDeg = 0;
        layout.valueReflectBlurPxTimes100 = 55;
        layout.valueReflectSquashPct = 28;
        layout.valueReflectMotionPct = 18;
        layout.colorValueFxEnabled = layout.valueFxEnabled;
        layout.colorValueGlowAlphaPct = layout.valueGlowAlphaPct;
        layout.colorValueGlowSpreadPxTimes100 = layout.valueGlowSpreadPxTimes100;
        layout.colorValueFxPerCharOffsetXPxTimes100 = layout.valueFxPerCharOffsetXPxTimes100;
        layout.colorValueFxPerCharOffsetYPxTimes100 = layout.valueFxPerCharOffsetYPxTimes100;
        layout.colorValueTopReflectAlphaPct = layout.valueTopReflectAlphaPct;
        layout.colorValueTopReflectOffsetXPxTimes100 = layout.valueTopReflectOffsetXPxTimes100;
        layout.colorValueTopReflectOffsetYPxTimes100 = layout.valueTopReflectOffsetYPxTimes100;
        layout.colorValueTopReflectShearPct = layout.valueTopReflectShearPct;
        layout.colorValueTopReflectRotateDeg = layout.valueTopReflectRotateDeg;
        layout.colorValueBottomReflectAlphaPct = layout.valueBottomReflectAlphaPct;
        layout.colorValueBottomReflectOffsetXPxTimes100 = layout.valueBottomReflectOffsetXPxTimes100;
        layout.colorValueBottomReflectOffsetYPxTimes100 = layout.valueBottomReflectOffsetYPxTimes100;
        layout.colorValueBottomReflectShearPct = layout.valueBottomReflectShearPct;
        layout.colorValueBottomReflectRotateDeg = layout.valueBottomReflectRotateDeg;
        layout.colorValueReflectBlurPxTimes100 = layout.valueReflectBlurPxTimes100;
        layout.colorValueReflectSquashPct = layout.valueReflectSquashPct;
        layout.colorValueReflectMotionPct = layout.valueReflectMotionPct;
        layout.mixValueFxEnabled = layout.valueFxEnabled;
        layout.mixValueGlowAlphaPct = layout.valueGlowAlphaPct;
        layout.mixValueGlowSpreadPxTimes100 = layout.valueGlowSpreadPxTimes100;
        layout.mixValueFxPerCharOffsetXPxTimes100 = layout.valueFxPerCharOffsetXPxTimes100;
        layout.mixValueFxPerCharOffsetYPxTimes100 = layout.valueFxPerCharOffsetYPxTimes100;
        layout.mixValueTopReflectAlphaPct = layout.valueTopReflectAlphaPct;
        layout.mixValueTopReflectOffsetXPxTimes100 = layout.valueTopReflectOffsetXPxTimes100;
        layout.mixValueTopReflectOffsetYPxTimes100 = layout.valueTopReflectOffsetYPxTimes100;
        layout.mixValueTopReflectShearPct = layout.valueTopReflectShearPct;
        layout.mixValueTopReflectRotateDeg = layout.valueTopReflectRotateDeg;
        layout.mixValueBottomReflectAlphaPct = layout.valueBottomReflectAlphaPct;
        layout.mixValueBottomReflectOffsetXPxTimes100 = layout.valueBottomReflectOffsetXPxTimes100;
        layout.mixValueBottomReflectOffsetYPxTimes100 = layout.valueBottomReflectOffsetYPxTimes100;
        layout.mixValueBottomReflectShearPct = layout.valueBottomReflectShearPct;
        layout.mixValueBottomReflectRotateDeg = layout.valueBottomReflectRotateDeg;
        layout.mixValueReflectBlurPxTimes100 = layout.valueReflectBlurPxTimes100;
        layout.mixValueReflectSquashPct = layout.valueReflectSquashPct;
        layout.mixValueReflectMotionPct = layout.valueReflectMotionPct;
    }
    else // Medium
    {
        layout.valueFxEnabled = 1;
        layout.valueGlowAlphaPct = 12;
        layout.valueGlowSpreadPxTimes100 = 115;
        layout.valueFxPerCharOffsetXPxTimes100 = 0;
        layout.valueFxPerCharOffsetYPxTimes100 = 0;
        layout.valueTopReflectAlphaPct = 8;
        layout.valueTopReflectOffsetXPxTimes100 = 0;
        layout.valueTopReflectOffsetYPxTimes100 = -125;
        layout.valueTopReflectShearPct = 12;
        layout.valueTopReflectRotateDeg = 0;
        layout.valueBottomReflectAlphaPct = 11;
        layout.valueBottomReflectOffsetXPxTimes100 = 0;
        layout.valueBottomReflectOffsetYPxTimes100 = 165;
        layout.valueBottomReflectShearPct = -14;
        layout.valueBottomReflectRotateDeg = 0;
        layout.valueReflectBlurPxTimes100 = 110;
        layout.valueReflectSquashPct = 45;
        layout.valueReflectMotionPct = 32;
        layout.colorValueFxEnabled = layout.valueFxEnabled;
        layout.colorValueGlowAlphaPct = layout.valueGlowAlphaPct;
        layout.colorValueGlowSpreadPxTimes100 = layout.valueGlowSpreadPxTimes100;
        layout.colorValueFxPerCharOffsetXPxTimes100 = layout.valueFxPerCharOffsetXPxTimes100;
        layout.colorValueFxPerCharOffsetYPxTimes100 = layout.valueFxPerCharOffsetYPxTimes100;
        layout.colorValueTopReflectAlphaPct = layout.valueTopReflectAlphaPct;
        layout.colorValueTopReflectOffsetXPxTimes100 = layout.valueTopReflectOffsetXPxTimes100;
        layout.colorValueTopReflectOffsetYPxTimes100 = layout.valueTopReflectOffsetYPxTimes100;
        layout.colorValueTopReflectShearPct = layout.valueTopReflectShearPct;
        layout.colorValueTopReflectRotateDeg = layout.valueTopReflectRotateDeg;
        layout.colorValueBottomReflectAlphaPct = layout.valueBottomReflectAlphaPct;
        layout.colorValueBottomReflectOffsetXPxTimes100 = layout.valueBottomReflectOffsetXPxTimes100;
        layout.colorValueBottomReflectOffsetYPxTimes100 = layout.valueBottomReflectOffsetYPxTimes100;
        layout.colorValueBottomReflectShearPct = layout.valueBottomReflectShearPct;
        layout.colorValueBottomReflectRotateDeg = layout.valueBottomReflectRotateDeg;
        layout.colorValueReflectBlurPxTimes100 = layout.valueReflectBlurPxTimes100;
        layout.colorValueReflectSquashPct = layout.valueReflectSquashPct;
        layout.colorValueReflectMotionPct = layout.valueReflectMotionPct;
        layout.mixValueFxEnabled = layout.valueFxEnabled;
        layout.mixValueGlowAlphaPct = layout.valueGlowAlphaPct;
        layout.mixValueGlowSpreadPxTimes100 = layout.valueGlowSpreadPxTimes100;
        layout.mixValueFxPerCharOffsetXPxTimes100 = layout.valueFxPerCharOffsetXPxTimes100;
        layout.mixValueFxPerCharOffsetYPxTimes100 = layout.valueFxPerCharOffsetYPxTimes100;
        layout.mixValueTopReflectAlphaPct = layout.valueTopReflectAlphaPct;
        layout.mixValueTopReflectOffsetXPxTimes100 = layout.valueTopReflectOffsetXPxTimes100;
        layout.mixValueTopReflectOffsetYPxTimes100 = layout.valueTopReflectOffsetYPxTimes100;
        layout.mixValueTopReflectShearPct = layout.valueTopReflectShearPct;
        layout.mixValueTopReflectRotateDeg = layout.valueTopReflectRotateDeg;
        layout.mixValueBottomReflectAlphaPct = layout.valueBottomReflectAlphaPct;
        layout.mixValueBottomReflectOffsetXPxTimes100 = layout.valueBottomReflectOffsetXPxTimes100;
        layout.mixValueBottomReflectOffsetYPxTimes100 = layout.valueBottomReflectOffsetYPxTimes100;
        layout.mixValueBottomReflectShearPct = layout.valueBottomReflectShearPct;
        layout.mixValueBottomReflectRotateDeg = layout.valueBottomReflectRotateDeg;
        layout.mixValueReflectBlurPxTimes100 = layout.valueReflectBlurPxTimes100;
        layout.mixValueReflectSquashPct = layout.valueReflectSquashPct;
        layout.mixValueReflectMotionPct = layout.valueReflectMotionPct;
    }

    for (int engineIndex = 0; engineIndex < LayoutTuning::engineCount; ++engineIndex)
    {
        for (int fieldIndex = 0; fieldIndex < LayoutTuning::mainValueFieldCount; ++fieldIndex)
        {
            auto& fx = layout.mainValueAnimationsByEngine[static_cast<std::size_t>(engineIndex)][static_cast<std::size_t>(fieldIndex)].fx;
            fx.enabled = layout.valueFxEnabled;
            fx.glowAlphaPct = layout.valueGlowAlphaPct;
            fx.glowSpreadPxTimes100 = layout.valueGlowSpreadPxTimes100;
            fx.perCharOffsetXPxTimes100 = layout.valueFxPerCharOffsetXPxTimes100;
            fx.perCharOffsetYPxTimes100 = layout.valueFxPerCharOffsetYPxTimes100;
            fx.topReflectAlphaPct = layout.valueTopReflectAlphaPct;
            fx.topReflectOffsetXPxTimes100 = layout.valueTopReflectOffsetXPxTimes100;
            fx.topReflectOffsetYPxTimes100 = layout.valueTopReflectOffsetYPxTimes100;
            fx.topReflectShearPct = layout.valueTopReflectShearPct;
            fx.topReflectRotateDeg = layout.valueTopReflectRotateDeg;
            fx.bottomReflectAlphaPct = layout.valueBottomReflectAlphaPct;
            fx.bottomReflectOffsetXPxTimes100 = layout.valueBottomReflectOffsetXPxTimes100;
            fx.bottomReflectOffsetYPxTimes100 = layout.valueBottomReflectOffsetYPxTimes100;
            fx.bottomReflectShearPct = layout.valueBottomReflectShearPct;
            fx.bottomReflectRotateDeg = layout.valueBottomReflectRotateDeg;
            fx.reflectBlurPxTimes100 = layout.valueReflectBlurPxTimes100;
            fx.reflectSquashPct = layout.valueReflectSquashPct;
            fx.reflectMotionPct = layout.valueReflectMotionPct;
        }
    }

    layoutGlobalPanel.refreshAll();
    layoutTextAnimationPanel.refreshAll();
    layoutGreenPanel.refreshAll();
    layoutBluePanel.refreshAll();
    layoutRedPanel.refreshAll();
    layoutPurplePanel.refreshAll();
    layoutBlackPanel.refreshAll();
    editor.applyLayout();
}
