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

#include "ChorusDSPPrepare.h"
#include "ChorusDSP.h"
#include <cmath>

void ChorusDSPPrepare::prepareLFOs(ChorusDSP& chorusDSP, const juce::dsp::ProcessSpec& spec)
{
    auto oscFunction = [] (float x) { return std::sin(x); };
    chorusDSP.lfo.initialise(oscFunction);
    chorusDSP.lfo.prepare(spec);
    chorusDSP.lfo.setFrequency(1.0f);

    auto cosFunction = [] (float x) { return std::cos(x); };
    chorusDSP.lfoCos.initialise(cosFunction);
    chorusDSP.lfoCos.prepare(spec);
    chorusDSP.lfoCos.setFrequency(1.0f);

    constexpr float oscVolumeMultiplier = 0.5f;
    chorusDSP.oscVolume.reset(spec.sampleRate, 0.0);
    chorusDSP.oscVolume.setCurrentAndTargetValue(0.25f * oscVolumeMultiplier);

    // Output trim gain (0dB default = gain of 1.0)
    chorusDSP.smoothedOutputTrimGain.reset(spec.sampleRate, 0.01);  // 10ms smoothing
    chorusDSP.smoothedOutputTrimGain.setCurrentAndTargetValue(1.0f);
}

void ChorusDSPPrepare::prepareFilters(ChorusDSP& chorusDSP, const juce::dsp::ProcessSpec& spec)
{
    chorusDSP.hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 30.0f, 0.707f);
    chorusDSP.hpf.coefficients = chorusDSP.hpfCoeffs;
    chorusDSP.hpf.prepare(spec);

    chorusDSP.lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 20000.0f, 0.707f);
    chorusDSP.lpf.coefficients = chorusDSP.lpfCoeffs;
    chorusDSP.lpf.prepare(spec);

    chorusDSP.preEmphasisCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(spec.sampleRate, 3000.0f, 0.707f, 1.2f);
    chorusDSP.preEmphasis.coefficients = chorusDSP.preEmphasisCoeffs;
    chorusDSP.preEmphasis.prepare(spec);
    chorusDSP.previousPreEmphasis.coefficients = chorusDSP.preEmphasisCoeffs;
    chorusDSP.previousPreEmphasis.prepare(spec);
    chorusDSP.pendingPreEmphasis.coefficients = chorusDSP.preEmphasisCoeffs;
    chorusDSP.pendingPreEmphasis.prepare(spec);
    
    chorusDSP.widthMidCoeffs1 = juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 200.0f, 0.707f);
    chorusDSP.widthMidCoeffs2 = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 2000.0f, 0.707f);
    chorusDSP.widthMidFilter1.coefficients = chorusDSP.widthMidCoeffs1;
    chorusDSP.widthMidFilter2.coefficients = chorusDSP.widthMidCoeffs2;
    chorusDSP.widthSideFilter1.coefficients = chorusDSP.widthMidCoeffs1;
    chorusDSP.widthSideFilter2.coefficients = chorusDSP.widthMidCoeffs2;
    chorusDSP.widthMidFilter1.prepare(spec);
    chorusDSP.widthMidFilter2.prepare(spec);
    chorusDSP.widthSideFilter1.prepare(spec);
    chorusDSP.widthSideFilter2.prepare(spec);
    
    chorusDSP.compressor.setAttack(50.0f);
    chorusDSP.compressor.setRelease(200.0f);
    chorusDSP.compressor.setThreshold(-6.0f);
    chorusDSP.compressor.setRatio(4.0f);
    chorusDSP.compressor.prepare(spec);
}

void ChorusDSPPrepare::prepareBuffers(ChorusDSP& chorusDSP, const juce::dsp::ProcessSpec& spec)
{
    chorusDSP.maxBlockSize = static_cast<int>(spec.maximumBlockSize);
    chorusDSP.lfoBuffer.setSize(1, chorusDSP.maxBlockSize);
    chorusDSP.cosBuffer.setSize(1, chorusDSP.maxBlockSize);
    chorusDSP.delaySamplesBuffer.setSize(1, chorusDSP.maxBlockSize);
    chorusDSP.centreDelayPerSampleMsBuffer.setSize(1, chorusDSP.maxBlockSize);
    chorusDSP.colorPerSampleBuffer.setSize(1, chorusDSP.maxBlockSize);
    chorusDSP.rawDepthPerSampleBuffer.setSize(1, chorusDSP.maxBlockSize);
    chorusDSP.modulationDepthPerSampleBuffer.setSize(1, chorusDSP.maxBlockSize);
    chorusDSP.preEmphOriginalBuffer.setSize(spec.numChannels, chorusDSP.maxBlockSize);
    
    chorusDSP.dryWet.setMixingRule(juce::dsp::DryWetMixingRule::balanced);
    chorusDSP.dryWet.prepare(spec);
    chorusDSP.dryWet.setWetMixProportion(0.5f);

    chorusDSP.wetCompressors.resize(static_cast<size_t>(spec.numChannels));
    for (auto& comp : chorusDSP.wetCompressors)
    {
        comp.envelope = 0.0f;
        comp.attackCoeff = std::exp(-1.0f / (ChorusDSP::WetCompressorState::attackMs * 0.001f
                                             * static_cast<float>(spec.sampleRate)));
        comp.releaseCoeff = std::exp(-1.0f / (ChorusDSP::WetCompressorState::releaseMs * 0.001f
                                              * static_cast<float>(spec.sampleRate)));
    }
}
