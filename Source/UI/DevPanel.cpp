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
#include "BinaryData.h"
#include <cmath>

namespace
{
class DevPanelSectionLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawPropertyPanelSectionHeader(juce::Graphics& g, const juce::String& name,
                                        bool isOpen, int width, int height) override
    {
        if (!isOpen)
        {
            const auto oldLabelColour = findColour(juce::Label::textColourId);
            const auto oldPropertyLabelColour = findColour(juce::PropertyComponent::labelTextColourId);
            setColour(juce::Label::textColourId, juce::Colours::black);
            setColour(juce::PropertyComponent::labelTextColourId, juce::Colours::black);
            juce::LookAndFeel_V4::drawPropertyPanelSectionHeader(g, name, isOpen, width, height);
            setColour(juce::Label::textColourId, oldLabelColour);
            setColour(juce::PropertyComponent::labelTextColourId, oldPropertyLabelColour);
            return;
        }

        juce::LookAndFeel_V4::drawPropertyPanelSectionHeader(g, name, isOpen, width, height);
    }
};

DevPanelSectionLookAndFeel& getDevPanelSectionLookAndFeel()
{
    static DevPanelSectionLookAndFeel lf;
    return lf;
}

class FloatValueSource : public juce::Value::ValueSource
{
public:
    FloatValueSource(std::function<float()> getterIn, std::function<void(float)> setterIn)
        : getter(std::move(getterIn)), setter(std::move(setterIn)) {}

    juce::var getValue() const override { return getter(); }

    void setValue(const juce::var& newValue) override
    {
        const float value = static_cast<float>(newValue);
        setter(value);
    }

private:
    std::function<float()> getter;
    std::function<void(float)> setter;
};

juce::Value makeFloatValue(std::function<float()> getter, std::function<void(float)> setter)
{
    return juce::Value(new FloatValueSource(std::move(getter), std::move(setter)));
}

juce::String formatFloat(float value)
{
    return juce::String(value, 4);
}

bool varsEquivalent(const juce::var& a, const juce::var& b, double numberTolerance = 1.0e-6)
{
    if (a.isVoid() || b.isVoid())
        return a.isVoid() == b.isVoid();

    if ((a.isInt() || a.isInt64() || a.isDouble()) && (b.isInt() || b.isInt64() || b.isDouble()))
        return std::abs(static_cast<double>(a) - static_cast<double>(b)) <= numberTolerance;

    if (a.isBool() || b.isBool())
        return static_cast<bool>(a) == static_cast<bool>(b);

    if (a.isString() || b.isString())
        return a.toString() == b.toString();

    if (const auto* arrA = a.getArray())
    {
        const auto* arrB = b.getArray();
        if (arrB == nullptr || arrA->size() != arrB->size())
            return false;
        for (int i = 0; i < arrA->size(); ++i)
        {
            if (!varsEquivalent((*arrA)[i], (*arrB)[i], numberTolerance))
                return false;
        }
        return true;
    }

    const auto* objA = a.getDynamicObject();
    const auto* objB = b.getDynamicObject();
    if (objA != nullptr || objB != nullptr)
    {
        if (objA == nullptr || objB == nullptr)
            return false;
        const auto propsA = objA->getProperties();
        const auto propsB = objB->getProperties();
        if (propsA.size() != propsB.size())
            return false;
        for (int i = 0; i < propsA.size(); ++i)
        {
            const auto key = propsA.getName(i);
            if (!objB->hasProperty(key))
                return false;
            if (!varsEquivalent(propsA.getValueAt(i), objB->getProperty(key), numberTolerance))
                return false;
        }
        return true;
    }

    return a == b;
}

juce::Font makeRetroFont(float height, bool bold)
{
    juce::Font font { juce::FontOptions { height, bold ? juce::Font::bold : juce::Font::plain } };
    if (BinaryData::Retroica_ttfSize > 0)
    {
        static juce::Typeface::Ptr retroTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Retroica_ttf,
            static_cast<size_t>(BinaryData::Retroica_ttfSize));
        if (retroTypeface != nullptr)
            font = juce::Font { juce::FontOptions { retroTypeface }.withHeight(height) };
    }
    if (bold)
        font.setBold(true);
    return font;
}

class LockableFloatPropertyComponent : public juce::PropertyComponent
{
public:
    LockableFloatPropertyComponent(const juce::Value& valueToControl, const juce::String& name,
                                   double min, double max, double step, double skew,
                                   const juce::String& tooltipText)
        : juce::PropertyComponent(name),
          controlledValue(valueToControl)
    {
        setTooltip(tooltipText);
        slider.setRange(min, max, step);
        slider.setSkewFactor(skew);
        slider.setScrollWheelEnabled(true);
        slider.setTooltip(tooltipText);
        slider.onValueChange = [this]
        {
            if (!locked)
                controlledValue = static_cast<float>(slider.getValue());
        };
        addAndMakeVisible(slider);

        lockButton.setClickingTogglesState(true);
        lockButton.setToggleState(true, juce::dontSendNotification);
        lockButton.onClick = [this]
        {
            setLocked(lockButton.getToggleState());
        };
        addAndMakeVisible(lockButton);

        setLocked(true);
        refresh();
    }

    void refresh() override
    {
        const auto valueAsDouble = static_cast<double>(controlledValue.getValue());
        slider.setValue(valueAsDouble, juce::dontSendNotification);
        slider.setEnabled(!locked);
    }

    void resized() override
    {
        auto content = getLookAndFeel().getPropertyComponentContentPosition(*this);
        auto lockBounds = content.removeFromRight(30).reduced(2, 2);
        lockButton.setBounds(lockBounds);
        slider.setBounds(content.reduced(0, 2));
    }

    void setLocked(bool shouldLock)
    {
        locked = shouldLock;
        slider.setEnabled(!locked);
        lockButton.setButtonText(locked ? "L" : "U");
        lockButton.setTooltip(locked ? "Locked: click to unlock this control"
                                     : "Unlocked: click to lock this control");
        lockButton.setToggleState(locked, juce::dontSendNotification);
    }

    bool isLocked() const { return locked; }

private:
    juce::Value controlledValue;
    juce::Slider slider;
    juce::TextButton lockButton;
    bool locked = true;
};

void styleTitle(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(makeRetroFont(15.0f, true));
    label.setJustificationType(juce::Justification::left);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
}

void styleDescription(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(makeRetroFont(12.5f, false));
    label.setJustificationType(juce::Justification::left);
    label.setColour(juce::Label::textColourId, juce::Colour(0xff9f9f9f));
}

void stylePanel(juce::PropertyPanel& panel)
{
    panel.setOpaque(true);
    panel.setLookAndFeel(&getDevPanelSectionLookAndFeel());
}

void addPanelSection(juce::PropertyPanel& panel, const juce::String& name,
                     juce::Array<juce::PropertyComponent*>& props, bool shouldBeOpen)
{
    for (auto* prop : props)
        prop->setPreferredHeight(30);
    panel.addSection(name, props, shouldBeOpen);
    stylePanel(panel);
}
} // namespace

DevPanel::DevPanel(ChoroborosPluginEditor& editorRef, ChoroborosAudioProcessor& processorRef)
    : editor(editorRef), processor(processorRef)
{
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 600);
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);

    addAndMakeVisible(copyJsonButton);
    copyJsonButton.setButtonText("Copy JSON");
    copyJsonButton.setTooltip("Copies all current Dev panel values as JSON.");
    copyJsonButton.onClick = [this]
    {
        juce::SystemClipboard::copyTextToClipboard(buildJson());
    };

    addAndMakeVisible(saveDefaultsButton);
    saveDefaultsButton.setButtonText("Set Current as Defaults");
    saveDefaultsButton.setTooltip("Saves all current tuning, internals, and layout values as startup defaults.");
    saveDefaultsButton.onClick = [this] { saveCurrentAsDefaults(); };

    addAndMakeVisible(lockToggleButton);
    lockToggleButton.setTooltip("Locks or unlocks all controls for safe editing.");
    lockToggleButton.onClick = [this] { setEditingLocked(!editingLocked); };
    addAndMakeVisible(fxPresetOffButton);
    fxPresetOffButton.setButtonText("FX Off");
    fxPresetOffButton.setTooltip("Applies an off preset to value glow/reflection FX groups.");
    fxPresetOffButton.onClick = [this] { applyValueFxPreset(0); };
    addAndMakeVisible(fxPresetSubtleButton);
    fxPresetSubtleButton.setButtonText("FX Subtle");
    fxPresetSubtleButton.setTooltip("Applies a subtle value FX preset.");
    fxPresetSubtleButton.onClick = [this] { applyValueFxPreset(1); };
    addAndMakeVisible(fxPresetMediumButton);
    fxPresetMediumButton.setButtonText("FX Medium");
    fxPresetMediumButton.setTooltip("Applies a medium-strength value FX preset.");
    fxPresetMediumButton.onClick = [this] { applyValueFxPreset(2); };

    styleTitle(mappingTitle, "Parameter Mapping");
    styleDescription(mappingDescription, "Map UI values into DSP ranges (min/max/curve).");
    styleTitle(uiTitle, "UI Response");
    styleDescription(uiDescription, "Change slider feel without touching DSP.");
    styleTitle(internalsTitle, "DSP Internals (Per Engine + HQ)");
    styleDescription(internalsDescription, "Independent profiles for each engine in Normal and HQ modes.");
    styleTitle(bbdTitle, "BBD Profiles (Red NQ)");
    styleDescription(bbdDescription, "Only shown for Red Normal, where the BBD core is active.");
    styleTitle(tapeTitle, "Tape Profiles (Red HQ)");
    styleDescription(tapeDescription, "Only shown for Red HQ, where the Tape core is active.");
    styleTitle(layoutTitle, "Layout");
    styleDescription(layoutDescription, "Live placement + sizing of UI elements (grouped by engine color).");
    styleDescription(activeProfileLabel, "Active Profile: Green NQ");
    activeProfileLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd8d8d8));
    activeProfileLabel.setJustificationType(juce::Justification::centredLeft);

    content.addAndMakeVisible(mappingTitle);
    content.addAndMakeVisible(mappingDescription);
    content.addAndMakeVisible(uiTitle);
    content.addAndMakeVisible(uiDescription);
    content.addAndMakeVisible(internalsTitle);
    content.addAndMakeVisible(internalsDescription);
    content.addAndMakeVisible(bbdTitle);
    content.addAndMakeVisible(bbdDescription);
    content.addAndMakeVisible(tapeTitle);
    content.addAndMakeVisible(tapeDescription);
    content.addAndMakeVisible(layoutTitle);
    content.addAndMakeVisible(layoutDescription);
    content.addAndMakeVisible(activeProfileLabel);

    content.addAndMakeVisible(mappingPanel);
    content.addAndMakeVisible(uiPanel);
    content.addAndMakeVisible(internalsPanel);
    content.addAndMakeVisible(bbdPanel);
    content.addAndMakeVisible(tapePanel);
    content.addAndMakeVisible(layoutPanel);

    auto& tuning = processor.getTuningState();
    auto& layout = editor.getLayoutTuning();

    const auto makeTooltipForControl = [](const juce::String& name) -> juce::String
    {
        const juce::String n = name.toLowerCase();

        if (n.contains("bbd"))
            return "Used only in Red NQ (BBD core). BBD emulates analog bucket-brigade delay behavior. Slower clock and lower cutoff sound darker and grainier; faster clock and higher cutoff sound cleaner and brighter.";
        if (n.contains("tape"))
            return "Used only in Red HQ (Tape core). Tape controls shape wow/flutter pitch drift, tone rolloff, and nonlinear drive. Small moves can be musical; big moves can sound unstable or lo-fi.";

        if (n.contains(" min"))
            return "Sets the lowest allowed value. Think of this as the floor for this control's mapped range.";
        if (n.contains(" max"))
            return "Sets the highest allowed value. Think of this as the ceiling for this control's mapped range.";
        if (n.contains(" curve"))
            return "Shapes the knob response curve. Higher curve values give finer control near low settings; lower curve values feel more linear across the whole travel.";
        if (n.contains("ui skew"))
            return "Changes only how the UI control responds under your mouse. DSP behavior does not change.";
        if (n.contains("enabled"))
            return "Turns this effect block on or off.";
        if (n.contains("duration"))
            return "Controls animation time. Longer duration feels smoother/slower; shorter duration feels snappier.";
        if (n.contains("travel"))
            return "Sets animation movement distance in pixels. Lower values are subtler.";
        if (n.contains("shear"))
            return "Controls tilt/skew amount for the text effect.";
        if (n.contains("scale amount"))
            return "Controls how much characters shrink during the flip effect.";
        if (n.contains("alpha"))
            return "Controls transparency. 0 is invisible, 100 is fully opaque.";
        if (n.contains("blur"))
            return "Softens edges. Higher values create a blurrier, more diffuse look.";
        if (n.contains("reflect"))
            return "Adjusts the reflection layer for value text (position, intensity, shape, and motion).";
        if (n.contains("glow"))
            return "Adjusts glow around value text. Keep this subtle for readability.";
        if (n.contains("offset x"))
            return "Moves this element left/right (X axis).";
        if (n.contains("offset y"))
            return "Moves this element up/down (Y axis).";
        if (n.contains(" center x"))
            return "Sets horizontal center position.";
        if (n.contains(" top y"))
            return "Sets vertical top position.";
        if (n.contains(" size"))
            return "Controls visual size.";
        if (n.contains(" width"))
            return "Controls visual width.";
        if (n.contains(" height"))
            return "Controls visual height.";
        if (n.contains("font"))
            return "Controls text size for this UI element.";
        if (n.contains("colour") || n.contains("color"))
            return "Controls colour channels (A,R,G,B) for this UI element.";
        if (n.contains("smooth"))
            return "Smoothing time. Higher values reduce zipper noise but react slower; lower values feel tighter and more immediate.";
        if (n.contains("cutoff"))
            return "Filter cutoff frequency. Lower cutoff sounds darker; higher cutoff sounds brighter.";
        if (n.contains(" q"))
            return "Filter resonance (Q). Higher Q emphasizes frequencies around the cutoff point.";
        if (n.contains("attack"))
            return "Compressor attack time. Lower attack clamps peaks faster; higher attack lets transients through.";
        if (n.contains("release"))
            return "Compressor release time. Lower release recovers quickly; higher release is smoother but can pump.";
        if (n.contains("threshold"))
            return "Compressor threshold. Lower threshold means compression engages more often.";
        if (n.contains("ratio"))
            return "Ratio/intensity control. Higher values increase effect strength or compression amount.";
        if (n.contains("drive"))
            return "Drive/saturation amount. More drive adds harmonics and thickness, but too much can harshen.";
        if (n.contains("wow") || n.contains("flutter"))
            return "Tape-style pitch modulation: wow is slower drift, flutter is faster shimmer. Too high can sound seasick.";
        if (n.contains("clock"))
            return "BBD clock controls delay quality. Slower clock is darker/noisier; faster clock is cleaner.";
        if (n.contains("delay"))
            return "Controls base delay timing, which sets the chorus center point and movement range.";
        if (n.contains("phase"))
            return "Controls phase behavior/stability. Can affect stereo image and motion smoothness.";
        if (n.contains("depth rate limit"))
            return "Limits how fast depth can change over time. Prevents zipper/crackle during aggressive automation.";
        if (n.contains("centre base"))
            return "Base chorus delay before modulation. Lower values feel tighter; higher values feel wider/slower.";
        if (n.contains("centre scale"))
            return "How strongly depth pushes delay away from the center base value.";
        if (n.contains("preemph"))
            return "Pre-emphasis boosts selected high frequencies before chorus/saturation. Useful for recovering clarity.";
        if (n.contains("hpf"))
            return "High-pass filter removes low end before modulation to reduce mud/rumble.";
        if (n.contains("knob sweep"))
            return "Sets the rotary animation start/end angle.";
        if (n.contains("frame count"))
            return "Sets how many sprite frames are used for knob animation.";
        if (n.contains("visual response"))
            return "UI-only smoothing for knob movement. Changes feel, not DSP audio.";
        if (n.contains("mix"))
            return "Dry/wet balance behavior. Lower values keep more source signal; higher values emphasize chorus.";
        if (n.contains("rate"))
            return "LFO speed behavior. Slower rates sound lush/swimmy; faster rates move toward vibrato-like motion.";
        if (n.contains("depth"))
            return "Modulation amount. More depth means stronger pitch/time motion.";
        if (n.contains("width"))
            return "Stereo spread behavior. Wider settings increase side information and perceived space.";
        if (n.contains("offset"))
            return "Stereo phase offset behavior between modulation channels, shaping width and motion character.";

        return "Live DSP/UI tuning control. Use small moves, listen for tone/motion changes, then refine with A/B comparisons.";
    };

    auto makeLockable = [this, &makeTooltipForControl](const juce::Value& valueToControl, const juce::String& name,
                               double min, double max, double step, double skew) -> juce::PropertyComponent*
    {
        auto* prop = new LockableFloatPropertyComponent(valueToControl, name, min, max, step, skew, makeTooltipForControl(name));
        lockableProperties.add(prop);
        return prop;
    };

    auto addParamMapping = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& name,
                               ChoroborosAudioProcessor::ParamTuning& param, float baseMin, float baseMax)
    {
        props.add(makeLockable(
            makeFloatValue([&] { return param.min.load(); },
                           [&](float v) { param.min.store(v); editor.refreshValueLabels(); }),
            name + " Min", baseMin * 0.5f, baseMax * 2.0f, 0.0001f, 1.0f));
        props.add(makeLockable(
            makeFloatValue([&] { return param.max.load(); },
                           [&](float v) { param.max.store(v); editor.refreshValueLabels(); }),
            name + " Max", baseMin * 0.5f, baseMax * 2.0f, 0.0001f, 1.0f));
        props.add(makeLockable(
            makeFloatValue([&] { return param.curve.load(); },
                           [&](float v) { param.curve.store(v); editor.refreshValueLabels(); }),
            name + " Curve", 0.2f, 5.0f, 0.001f, 1.0f));
    };

    juce::Array<juce::PropertyComponent*> mappingRate;
    juce::Array<juce::PropertyComponent*> mappingDepth;
    juce::Array<juce::PropertyComponent*> mappingOffset;
    juce::Array<juce::PropertyComponent*> mappingWidth;
    juce::Array<juce::PropertyComponent*> mappingColor;
    juce::Array<juce::PropertyComponent*> mappingMix;
    addParamMapping(mappingRate, "Rate", tuning.rate, ChoroborosAudioProcessor::RATE_MIN, ChoroborosAudioProcessor::RATE_MAX);
    addParamMapping(mappingDepth, "Depth", tuning.depth, ChoroborosAudioProcessor::DEPTH_MIN, ChoroborosAudioProcessor::DEPTH_MAX);
    addParamMapping(mappingOffset, "Offset", tuning.offset, ChoroborosAudioProcessor::OFFSET_MIN, ChoroborosAudioProcessor::OFFSET_MAX);
    addParamMapping(mappingWidth, "Width", tuning.width, ChoroborosAudioProcessor::WIDTH_MIN, ChoroborosAudioProcessor::WIDTH_MAX);
    addParamMapping(mappingColor, "Color", tuning.color, ChoroborosAudioProcessor::COLOR_MIN, ChoroborosAudioProcessor::COLOR_MAX);
    addParamMapping(mappingMix, "Mix", tuning.mix, ChoroborosAudioProcessor::MIX_MIN, ChoroborosAudioProcessor::MIX_MAX);
    addPanelSection(mappingPanel, "Rate", mappingRate, true);
    addPanelSection(mappingPanel, "Depth", mappingDepth, false);
    addPanelSection(mappingPanel, "Offset", mappingOffset, false);
    addPanelSection(mappingPanel, "Width", mappingWidth, false);
    addPanelSection(mappingPanel, "Color", mappingColor, false);
    addPanelSection(mappingPanel, "Mix", mappingMix, false);

    auto addUiSkew = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& name, ChoroborosAudioProcessor::ParamTuning& param)
    {
        props.add(makeLockable(
            makeFloatValue([&] { return param.uiSkew.load(); },
                           [&](float v) { param.uiSkew.store(v); editor.applyTuningToUI(); }),
            name + " UI Skew", 0.2f, 5.0f, 0.001f, 1.0f));
    };
    juce::Array<juce::PropertyComponent*> uiResponseRate;
    juce::Array<juce::PropertyComponent*> uiResponseDepth;
    juce::Array<juce::PropertyComponent*> uiResponseOffset;
    juce::Array<juce::PropertyComponent*> uiResponseWidth;
    juce::Array<juce::PropertyComponent*> uiResponseColor;
    juce::Array<juce::PropertyComponent*> uiResponseMix;
    addUiSkew(uiResponseRate, "Rate", tuning.rate);
    addUiSkew(uiResponseDepth, "Depth", tuning.depth);
    addUiSkew(uiResponseOffset, "Offset", tuning.offset);
    addUiSkew(uiResponseWidth, "Width", tuning.width);
    addUiSkew(uiResponseColor, "Color", tuning.color);
    addUiSkew(uiResponseMix, "Mix", tuning.mix);
    addPanelSection(uiPanel, "Rate", uiResponseRate, true);
    addPanelSection(uiPanel, "Depth", uiResponseDepth, false);
    addPanelSection(uiPanel, "Offset", uiResponseOffset, false);
    addPanelSection(uiPanel, "Width", uiResponseWidth, false);
    addPanelSection(uiPanel, "Color", uiResponseColor, false);
    addPanelSection(uiPanel, "Mix", uiResponseMix, false);

    auto addInternal = [&](int engineIndex, bool hqEnabled, const juce::String& name, std::atomic<float>& target,
                           double min, double max, double step, double skew = 1.0)
    {
        return makeLockable(
            makeFloatValue([&target] { return target.load(); },
                           [&, engineIndex, hqEnabled](float v)
                           {
                               target.store(v);
                               processor.syncEngineInternalsToActiveDsp(engineIndex, hqEnabled);
                           }),
            name, min, max, step, skew);
    };

    auto addInternalsSectionsForEngine = [&](const juce::String& engineName, int engineIndex, bool hqEnabled,
                                             ChorusDSP::RuntimeTuning& engineTuning,
                                             bool firstOpen)
    {
        const juce::String profileName = engineName + (hqEnabled ? " HQ" : " NQ");
        juce::Array<juce::PropertyComponent*> dspTimingAndMotion;
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Rate Smooth (ms)", engineTuning.rateSmoothingMs, 0.0, 200.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Depth Smooth (ms)", engineTuning.depthSmoothingMs, 0.0, 500.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Depth Rate Limit", engineTuning.depthRateLimit, 0.01, 5.0, 0.01, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Centre Smooth (ms)", engineTuning.centreDelaySmoothingMs, 0.0, 500.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Centre Base (ms)", engineTuning.centreDelayBaseMs, 0.0, 30.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Centre Scale", engineTuning.centreDelayScale, 0.0, 30.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Color Smooth (ms)", engineTuning.colorSmoothingMs, 0.0, 200.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Width Smooth (ms)", engineTuning.widthSmoothingMs, 0.0, 200.0, 0.1, 1.0));

        juce::Array<juce::PropertyComponent*> dspFiltering;
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "HPF Cutoff (Hz)", engineTuning.hpfCutoffHz, 5.0, 200.0, 0.1, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "HPF Q", engineTuning.hpfQ, 0.1, 2.0, 0.001, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Freq (Hz)", engineTuning.preEmphasisFreqHz, 200.0, 12000.0, 1.0, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Q", engineTuning.preEmphasisQ, 0.1, 4.0, 0.001, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Gain", engineTuning.preEmphasisGain, 0.1, 4.0, 0.001, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Level Smooth", engineTuning.preEmphasisLevelSmoothing, 0.0, 1.0, 0.001, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Quiet Thresh", engineTuning.preEmphasisQuietThreshold, 0.0, 1.0, 0.001, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Max Amount", engineTuning.preEmphasisMaxAmount, 0.0, 1.0, 0.001, 1.0));

        juce::Array<juce::PropertyComponent*> dspCompressor;
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Attack (ms)", engineTuning.compressorAttackMs, 0.1, 200.0, 0.1, 1.0));
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Release (ms)", engineTuning.compressorReleaseMs, 1.0, 500.0, 0.1, 1.0));
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Threshold (dB)", engineTuning.compressorThresholdDb, -30.0, 6.0, 0.1, 1.0));
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Ratio", engineTuning.compressorRatio, 1.0, 12.0, 0.01, 1.0));

        juce::Array<juce::PropertyComponent*> dspSaturation;
        dspSaturation.add(addInternal(engineIndex, hqEnabled, "Saturation Drive Scale", engineTuning.saturationDriveScale, 0.0, 6.0, 0.01, 1.0));

        addPanelSection(internalsPanel, profileName + " - Timing + Motion", dspTimingAndMotion, firstOpen);
        addPanelSection(internalsPanel, profileName + " - Filtering + Emphasis", dspFiltering, false);
        addPanelSection(internalsPanel, profileName + " - Compressor", dspCompressor, false);
        addPanelSection(internalsPanel, profileName + " - Saturation", dspSaturation, false);

        // BBD internals are only used by Red NQ (engine 2, HQ off).
        if (engineIndex == 2 && !hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> bbdDelayAndDepth;
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Delay Smooth (ms)", engineTuning.bbdDelaySmoothingMs, 0.0, 500.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Delay Min (ms)", engineTuning.bbdDelayMinMs, 1.0, 50.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Delay Max (ms)", engineTuning.bbdDelayMaxMs, 10.0, 200.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Centre Base (ms)", engineTuning.bbdCentreBaseMs, 0.0, 50.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Centre Scale", engineTuning.bbdCentreScale, 0.0, 4.0, 0.01, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Depth (ms)", engineTuning.bbdDepthMs, 0.0, 40.0, 0.1, 1.0));

            juce::Array<juce::PropertyComponent*> bbdFilter;
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Smooth (ms)", engineTuning.bbdFilterSmoothingMs, 0.0, 200.0, 0.1, 1.0));
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Min (Hz)", engineTuning.bbdFilterCutoffMinHz, 200.0, 8000.0, 1.0, 1.0));
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Max (Hz)", engineTuning.bbdFilterCutoffMaxHz, 1000.0, 20000.0, 1.0, 1.0));
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Scale", engineTuning.bbdFilterCutoffScale, 0.05, 1.0, 0.001, 1.0));

            juce::Array<juce::PropertyComponent*> bbdClock;
            bbdClock.add(addInternal(engineIndex, hqEnabled, "BBD Clock Smooth (ms)", engineTuning.bbdClockSmoothingMs, 0.0, 200.0, 0.1, 1.0));
            bbdClock.add(addInternal(engineIndex, hqEnabled, "BBD Clock Min (Hz)", engineTuning.bbdClockMinHz, 200.0, 10000.0, 1.0, 1.0));
            bbdClock.add(addInternal(engineIndex, hqEnabled, "BBD Clock Max Ratio", engineTuning.bbdClockMaxRatio, 0.1, 1.0, 0.001, 1.0));

            addPanelSection(bbdPanel, profileName + " - Delay + Depth", bbdDelayAndDepth, firstOpen);
            addPanelSection(bbdPanel, profileName + " - Filter", bbdFilter, false);
            addPanelSection(bbdPanel, profileName + " - Clock", bbdClock, false);
        }

        // Tape internals are only used by Red HQ (engine 2, HQ on).
        if (engineIndex == 2 && hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> tapeDelayAndTone;
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Delay Smooth (ms)", engineTuning.tapeDelaySmoothingMs, 0.0, 800.0, 0.1, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Centre Base (ms)", engineTuning.tapeCentreBaseMs, 0.0, 50.0, 0.1, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Centre Scale", engineTuning.tapeCentreScale, 0.0, 4.0, 0.01, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Tone Max (Hz)", engineTuning.tapeToneMaxHz, 1000.0, 20000.0, 1.0, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Tone Min (Hz)", engineTuning.tapeToneMinHz, 1000.0, 20000.0, 1.0, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Tone Smooth", engineTuning.tapeToneSmoothingCoeff, 0.0, 1.0, 0.0001, 1.0));

            juce::Array<juce::PropertyComponent*> tapeMotionAndModulation;
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape LFO Ratio", engineTuning.tapeLfoRatioScale, 0.0, 0.2, 0.0001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape LFO Smooth", engineTuning.tapeLfoModSmoothingCoeff, 0.0, 0.05, 0.0001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Ratio Smooth", engineTuning.tapeRatioSmoothingCoeff, 0.0, 0.05, 0.0001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Phase Damp", engineTuning.tapePhaseDamping, 0.9, 1.0, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Freq Base", engineTuning.tapeWowFreqBase, 0.0, 2.0, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Freq Spread", engineTuning.tapeWowFreqSpread, 0.0, 0.5, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Freq Base", engineTuning.tapeFlutterFreqBase, 0.0, 12.0, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Freq Spread", engineTuning.tapeFlutterFreqSpread, 0.0, 2.0, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Depth Base", engineTuning.tapeWowDepthBase, 0.0, 0.01, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Depth Spread", engineTuning.tapeWowDepthSpread, 0.0, 0.002, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Depth Base", engineTuning.tapeFlutterDepthBase, 0.0, 0.01, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Depth Spread", engineTuning.tapeFlutterDepthSpread, 0.0, 0.002, 0.00001, 1.0));

            juce::Array<juce::PropertyComponent*> tapeDriveAndOutput;
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Drive Scale", engineTuning.tapeDriveScale, 0.0, 2.0, 0.001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Ratio Min", engineTuning.tapeRatioMin, 0.9, 1.0, 0.0001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Ratio Max", engineTuning.tapeRatioMax, 1.0, 1.1, 0.0001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Wet Gain", engineTuning.tapeWetGain, 0.5, 2.0, 0.001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Hermite Tension", engineTuning.tapeHermiteTension, 0.3, 1.2, 0.001, 1.0));

            addPanelSection(tapePanel, profileName + " - Delay + Tone", tapeDelayAndTone, firstOpen);
            addPanelSection(tapePanel, profileName + " - Motion + Modulation", tapeMotionAndModulation, false);
            addPanelSection(tapePanel, profileName + " - Drive + Output", tapeDriveAndOutput, false);
        }
    };

    addInternalsSectionsForEngine("Green", 0, false, processor.getEngineDspInternals(0, false), true);
    addInternalsSectionsForEngine("Green", 0, true, processor.getEngineDspInternals(0, true), false);
    addInternalsSectionsForEngine("Blue", 1, false, processor.getEngineDspInternals(1, false), false);
    addInternalsSectionsForEngine("Blue", 1, true, processor.getEngineDspInternals(1, true), false);
    addInternalsSectionsForEngine("Red", 2, false, processor.getEngineDspInternals(2, false), false);
    addInternalsSectionsForEngine("Red", 2, true, processor.getEngineDspInternals(2, true), false);
    addInternalsSectionsForEngine("Purple", 3, false, processor.getEngineDspInternals(3, false), false);
    addInternalsSectionsForEngine("Purple", 3, true, processor.getEngineDspInternals(3, true), false);
    addInternalsSectionsForEngine("Black", 4, false, processor.getEngineDspInternals(4, false), false);
    addInternalsSectionsForEngine("Black", 4, true, processor.getEngineDspInternals(4, true), false);

    juce::Array<juce::PropertyComponent*> layoutSliderProps;
    juce::Array<juce::PropertyComponent*> layoutGreenProps;
    juce::Array<juce::PropertyComponent*> layoutBlueProps;
    juce::Array<juce::PropertyComponent*> layoutRedProps;
    juce::Array<juce::PropertyComponent*> layoutPurpleProps;
    juce::Array<juce::PropertyComponent*> layoutBlackProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalValueTypographyProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalValueFxProps;
    juce::Array<juce::PropertyComponent*> layoutColorValueFxProps;
    juce::Array<juce::PropertyComponent*> layoutMixValueFxProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalMainValueFlipProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalColorValueFlipProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalMixValueFlipProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalTopButtonsProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalEngineSelectorProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalHqSwitchProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalKnobResponseProps;
    auto addLayout = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& name, int& target, int min, int max)
    {
        props.add(makeLockable(
            makeFloatValue([&] { return static_cast<float>(target); },
                           [&](float v)
                           {
                               target = static_cast<int>(std::round(v));
                               editor.applyLayout();
                           }),
            name, static_cast<double>(min), static_cast<double>(max), 1.0, 1.0));
    };
    auto addLayoutToGroup = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& name, int& target, int min, int max)
    {
        addLayout(props, name, target, min, max);
    };
    auto addLayoutGlobal = [&](const juce::String& name, int& target, int min, int max)
    {
        addLayoutToGroup(layoutGlobalValueTypographyProps, name, target, min, max);
    };
    auto addColourChannels = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& baseName, int& target)
    {
        auto addChannel = [&](const juce::String& channelName, int shift)
        {
            props.add(makeLockable(
                makeFloatValue([&target, shift]
                               {
                                   return static_cast<float>((target >> shift) & 0xff);
                               },
                               [this, &target, shift](float v)
                               {
                                   const int clamped = juce::jlimit(0, 255, static_cast<int>(std::round(v)));
                                   target = (target & ~(0xff << shift)) | (clamped << shift);
                                   this->editor.applyLayout();
                               }),
                baseName + " " + channelName, 0.0, 255.0, 1.0, 1.0));
        };
        addChannel("A", 24);
        addChannel("R", 16);
        addChannel("G", 8);
        addChannel("B", 0);
    };
    auto addLayoutByColor = [&](const juce::String& baseName,
                                int& green, int& blue, int& red, int& purple, int& black,
                                int min, int max)
    {
        addLayout(layoutGreenProps, baseName, green, min, max);
        addLayout(layoutBlueProps, baseName, blue, min, max);
        addLayout(layoutRedProps, baseName, red, min, max);
        addLayout(layoutPurpleProps, baseName, purple, min, max);
        addLayout(layoutBlackProps, baseName, black, min, max);
    };

    addLayoutByColor("Main Knob Size", layout.mainKnobSizeGreen, layout.mainKnobSizeBlue, layout.mainKnobSizeRed, layout.mainKnobSizePurple, layout.mainKnobSizeBlack, 40, 260);
    addLayoutByColor("Knob Top Y", layout.knobTopYGreen, layout.knobTopYBlue, layout.knobTopYRed, layout.knobTopYPurple, layout.knobTopYBlack, 0, 300);
    addLayoutByColor("Rate Center X", layout.rateCenterXGreen, layout.rateCenterXBlue, layout.rateCenterXRed, layout.rateCenterXPurple, layout.rateCenterXBlack, 0, 800);
    addLayoutByColor("Depth Center X", layout.depthCenterXGreen, layout.depthCenterXBlue, layout.depthCenterXRed, layout.depthCenterXPurple, layout.depthCenterXBlack, 0, 800);
    addLayoutByColor("Offset Center X", layout.offsetCenterXGreen, layout.offsetCenterXBlue, layout.offsetCenterXRed, layout.offsetCenterXPurple, layout.offsetCenterXBlack, 0, 800);
    addLayoutByColor("Width Center X", layout.widthCenterXGreen, layout.widthCenterXBlue, layout.widthCenterXRed, layout.widthCenterXPurple, layout.widthCenterXBlack, 0, 800);
    addLayoutByColor("Slider Track Start X", layout.sliderTrackStartXGreen, layout.sliderTrackStartXBlue, layout.sliderTrackStartXRed, layout.sliderTrackStartXPurple, layout.sliderTrackStartXBlack, 0, 800);
    addLayoutByColor("Slider Track Start Y", layout.sliderTrackStartYGreen, layout.sliderTrackStartYBlue, layout.sliderTrackStartYRed, layout.sliderTrackStartYPurple, layout.sliderTrackStartYBlack, 0, 500);
    addLayoutByColor("Slider Track End X", layout.sliderTrackEndXGreen, layout.sliderTrackEndXBlue, layout.sliderTrackEndXRed, layout.sliderTrackEndXPurple, layout.sliderTrackEndXBlack, 0, 800);
    addLayoutByColor("Slider Track End Y", layout.sliderTrackEndYGreen, layout.sliderTrackEndYBlue, layout.sliderTrackEndYRed, layout.sliderTrackEndYPurple, layout.sliderTrackEndYBlack, 0, 500);
    addLayoutByColor("Slider Size (%)", layout.sliderSizeGreen, layout.sliderSizeBlue, layout.sliderSizeRed, layout.sliderSizePurple, layout.sliderSizeBlack, 10, 500);
    addLayout(layoutSliderProps, "Mix Knob Y", layout.mixKnobY, 0, 500);
    addLayout(layoutSliderProps, "Color Value Center X", layout.colorValueCenterX, 0, 800);
    addLayoutByColor("Mix Knob Size", layout.mixKnobSizeGreen, layout.mixKnobSizeBlue, layout.mixKnobSizeRed, layout.mixKnobSizePurple, layout.mixKnobSizeBlack, 10, 260);
    addLayoutByColor("Mix Center X", layout.mixCenterXGreen, layout.mixCenterXBlue, layout.mixCenterXRed, layout.mixCenterXPurple, layout.mixCenterXBlack, 0, 800);
    addLayoutByColor("Value Label Y", layout.valueLabelYGreen, layout.valueLabelYBlue, layout.valueLabelYRed, layout.valueLabelYPurple, layout.valueLabelYBlack, 0, 500);
    addLayoutByColor("Rate Value X Offset", layout.rateValueOffsetXGreen, layout.rateValueOffsetXBlue, layout.rateValueOffsetXRed, layout.rateValueOffsetXPurple, layout.rateValueOffsetXBlack, -200, 200);
    addLayoutByColor("Depth Value X Offset", layout.depthValueOffsetXGreen, layout.depthValueOffsetXBlue, layout.depthValueOffsetXRed, layout.depthValueOffsetXPurple, layout.depthValueOffsetXBlack, -200, 200);
    addLayoutByColor("Offset Value X Offset", layout.offsetValueOffsetXGreen, layout.offsetValueOffsetXBlue, layout.offsetValueOffsetXRed, layout.offsetValueOffsetXPurple, layout.offsetValueOffsetXBlack, -200, 200);
    addLayoutByColor("Width Value X Offset", layout.widthValueOffsetXGreen, layout.widthValueOffsetXBlue, layout.widthValueOffsetXRed, layout.widthValueOffsetXPurple, layout.widthValueOffsetXBlack, -200, 200);
    addLayoutByColor("Color Value Y", layout.colorValueYGreen, layout.colorValueYBlue, layout.colorValueYRed, layout.colorValueYPurple, layout.colorValueYBlack, 0, 500);
    addLayoutByColor("Color Value X Offset", layout.colorValueXOffsetGreen, layout.colorValueXOffsetBlue, layout.colorValueXOffsetRed, layout.colorValueXOffsetPurple, layout.colorValueXOffsetBlack, -200, 200);
    addLayoutByColor("Mix Value Y", layout.mixValueYGreen, layout.mixValueYBlue, layout.mixValueYRed, layout.mixValueYPurple, layout.mixValueYBlack, 0, 500);
    addLayoutByColor("Mix Value X Offset", layout.mixValueOffsetXGreen, layout.mixValueOffsetXBlue, layout.mixValueOffsetXRed, layout.mixValueOffsetXPurple, layout.mixValueOffsetXBlack, -200, 200);

    addLayoutGlobal("Rate Value Y Offset", layout.rateValueOffsetY, -200, 200);
    addLayoutGlobal("Depth Value Y Offset", layout.depthValueOffsetY, -200, 200);
    addLayoutGlobal("Offset Value Y Offset", layout.offsetValueOffsetY, -200, 200);
    addLayoutGlobal("Width Value Y Offset", layout.widthValueOffsetY, -200, 200);
    addLayoutGlobal("Main Knobs Value Font Size", layout.knobValueFontSize, 8, 48);
    addLayoutGlobal("Color Value Font Size", layout.colorValueFontSize, 8, 48);
    addLayoutGlobal("Mix Value Font Size", layout.mixValueFontSize, 8, 48);
    addLayoutGlobal("Value Text Alpha (%)", layout.valueTextAlphaPct, 0, 100);

    addLayoutToGroup(layoutGlobalValueFxProps, "FX Enabled (0/1)", layout.valueFxEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalValueFxProps, "Glow Alpha (%)", layout.valueGlowAlphaPct, 0, 40);
    addLayoutToGroup(layoutGlobalValueFxProps, "Glow Spread (x0.01 px)", layout.valueGlowSpreadPxTimes100, 0, 800);
    addLayoutToGroup(layoutGlobalValueFxProps, "Per-Char Offset X (x0.01 px)", layout.valueFxPerCharOffsetXPxTimes100, -600, 600);
    addLayoutToGroup(layoutGlobalValueFxProps, "Per-Char Offset Y (x0.01 px)", layout.valueFxPerCharOffsetYPxTimes100, -600, 600);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Alpha (%)", layout.valueTopReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Offset X (x0.01 px)", layout.valueTopReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Offset Y (x0.01 px)", layout.valueTopReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Shear (%)", layout.valueTopReflectShearPct, -100, 100);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Rotate (deg)", layout.valueTopReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Alpha (%)", layout.valueBottomReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Offset X (x0.01 px)", layout.valueBottomReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Offset Y (x0.01 px)", layout.valueBottomReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Shear (%)", layout.valueBottomReflectShearPct, -100, 100);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Rotate (deg)", layout.valueBottomReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutGlobalValueFxProps, "Reflect Blur (x0.01 px)", layout.valueReflectBlurPxTimes100, 0, 800);
    addLayoutToGroup(layoutGlobalValueFxProps, "Reflect Squash (%)", layout.valueReflectSquashPct, 0, 95);
    addLayoutToGroup(layoutGlobalValueFxProps, "Reflect Motion (%)", layout.valueReflectMotionPct, 0, 200);

    addLayoutToGroup(layoutColorValueFxProps, "FX Enabled (0/1)", layout.colorValueFxEnabled, 0, 1);
    addLayoutToGroup(layoutColorValueFxProps, "Glow Alpha (%)", layout.colorValueGlowAlphaPct, 0, 40);
    addLayoutToGroup(layoutColorValueFxProps, "Glow Spread (x0.01 px)", layout.colorValueGlowSpreadPxTimes100, 0, 800);
    addLayoutToGroup(layoutColorValueFxProps, "Per-Char Offset X (x0.01 px)", layout.colorValueFxPerCharOffsetXPxTimes100, -600, 600);
    addLayoutToGroup(layoutColorValueFxProps, "Per-Char Offset Y (x0.01 px)", layout.colorValueFxPerCharOffsetYPxTimes100, -600, 600);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Alpha (%)", layout.colorValueTopReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Offset X (x0.01 px)", layout.colorValueTopReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Offset Y (x0.01 px)", layout.colorValueTopReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Shear (%)", layout.colorValueTopReflectShearPct, -100, 100);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Rotate (deg)", layout.colorValueTopReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Alpha (%)", layout.colorValueBottomReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Offset X (x0.01 px)", layout.colorValueBottomReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Offset Y (x0.01 px)", layout.colorValueBottomReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Shear (%)", layout.colorValueBottomReflectShearPct, -100, 100);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Rotate (deg)", layout.colorValueBottomReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutColorValueFxProps, "Reflect Blur (x0.01 px)", layout.colorValueReflectBlurPxTimes100, 0, 800);
    addLayoutToGroup(layoutColorValueFxProps, "Reflect Squash (%)", layout.colorValueReflectSquashPct, 0, 95);
    addLayoutToGroup(layoutColorValueFxProps, "Reflect Motion (%)", layout.colorValueReflectMotionPct, 0, 200);

    addLayoutToGroup(layoutMixValueFxProps, "FX Enabled (0/1)", layout.mixValueFxEnabled, 0, 1);
    addLayoutToGroup(layoutMixValueFxProps, "Glow Alpha (%)", layout.mixValueGlowAlphaPct, 0, 40);
    addLayoutToGroup(layoutMixValueFxProps, "Glow Spread (x0.01 px)", layout.mixValueGlowSpreadPxTimes100, 0, 800);
    addLayoutToGroup(layoutMixValueFxProps, "Per-Char Offset X (x0.01 px)", layout.mixValueFxPerCharOffsetXPxTimes100, -600, 600);
    addLayoutToGroup(layoutMixValueFxProps, "Per-Char Offset Y (x0.01 px)", layout.mixValueFxPerCharOffsetYPxTimes100, -600, 600);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Alpha (%)", layout.mixValueTopReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Offset X (x0.01 px)", layout.mixValueTopReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Offset Y (x0.01 px)", layout.mixValueTopReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Shear (%)", layout.mixValueTopReflectShearPct, -100, 100);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Rotate (deg)", layout.mixValueTopReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Alpha (%)", layout.mixValueBottomReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Offset X (x0.01 px)", layout.mixValueBottomReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Offset Y (x0.01 px)", layout.mixValueBottomReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Shear (%)", layout.mixValueBottomReflectShearPct, -100, 100);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Rotate (deg)", layout.mixValueBottomReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutMixValueFxProps, "Reflect Blur (x0.01 px)", layout.mixValueReflectBlurPxTimes100, 0, 800);
    addLayoutToGroup(layoutMixValueFxProps, "Reflect Squash (%)", layout.mixValueReflectSquashPct, 0, 95);
    addLayoutToGroup(layoutMixValueFxProps, "Reflect Motion (%)", layout.mixValueReflectMotionPct, 0, 200);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Enabled (0/1)", layout.mainValueFlipEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Duration (ms)", layout.mainValueFlipDurationMs, 20, 1000);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel Up (x0.01 px)", layout.mainValueFlipTravelUpPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel Down (x0.01 px)", layout.mainValueFlipTravelDownPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel Out (%)", layout.mainValueFlipTravelOutPct, 0, 400);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel In (%)", layout.mainValueFlipTravelInPct, 0, 400);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Shear (%)", layout.mainValueFlipShearPct, 0, 100);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Scale Amount (%)", layout.mainValueFlipMinScalePct, 0, 95);

    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Enabled (0/1)", layout.colorValueFlipEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Duration (ms)", layout.colorValueFlipDurationMs, 20, 1000);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel Up (x0.01 px)", layout.colorValueFlipTravelUpPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel Down (x0.01 px)", layout.colorValueFlipTravelDownPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel Out (%)", layout.colorValueFlipTravelOutPct, 0, 400);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel In (%)", layout.colorValueFlipTravelInPct, 0, 400);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Shear (%)", layout.colorValueFlipShearPct, 0, 100);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Scale Amount (%)", layout.colorValueFlipMinScalePct, 0, 95);

    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Enabled (0/1)", layout.mixValueFlipEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Duration (ms)", layout.mixValueFlipDurationMs, 20, 1000);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel Up (x0.01 px)", layout.mixValueFlipTravelUpPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel Down (x0.01 px)", layout.mixValueFlipTravelDownPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel Out (%)", layout.mixValueFlipTravelOutPct, 0, 400);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel In (%)", layout.mixValueFlipTravelInPct, 0, 400);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Shear (%)", layout.mixValueFlipShearPct, 0, 100);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Scale Amount (%)", layout.mixValueFlipMinScalePct, 0, 95);

    addLayout(layoutGlobalTopButtonsProps, "Buttons Width", layout.topButtonsWidth, 20, 220);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Height", layout.topButtonsHeight, 8, 80);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Gap", layout.topButtonsGap, 0, 40);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Right Margin", layout.topButtonsRightMargin, 0, 120);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Top Y", layout.topButtonsTopY, 0, 80);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Font Size", layout.topButtonsFontSize, 6, 48);
    addColourChannels(layoutGlobalTopButtonsProps, "Buttons Text Colour", layout.topButtonsTextColour);
    addColourChannels(layoutGlobalTopButtonsProps, "Buttons Bg Colour", layout.topButtonsBackgroundColour);
    addColourChannels(layoutGlobalTopButtonsProps, "Buttons Bg On Colour", layout.topButtonsOnBackgroundColour);

    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector X", layout.engineSelectorX, 0, 680);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector Y", layout.engineSelectorY, 0, 360);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector W", layout.engineSelectorW, 40, 260);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector H", layout.engineSelectorH, 10, 80);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector Font Size", layout.engineSelectorFontSize, 6, 48);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Text", layout.engineSelectorTextColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Bg", layout.engineSelectorBackgroundColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Outline", layout.engineSelectorOutlineColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Arrow", layout.engineSelectorArrowColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Bg", layout.engineSelectorPopupBackgroundColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Text", layout.engineSelectorPopupTextColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Highlight Bg", layout.engineSelectorPopupHighlightedBackgroundColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Highlight Text", layout.engineSelectorPopupHighlightedTextColour);

    addLayout(layoutGlobalHqSwitchProps, "Flip Switch Size", layout.hqSwitchSize, 20, 180);
    addLayout(layoutGlobalHqSwitchProps, "Flip Switch Offset X", layout.hqSwitchOffsetX, -300, 300);
    addLayout(layoutGlobalHqSwitchProps, "Flip Switch Offset Y", layout.hqSwitchOffsetY, -300, 300);
    addLayout(layoutGlobalKnobResponseProps, "Rate Visual Response (ms)", layout.rateKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Depth Visual Response (ms)", layout.depthKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Offset Visual Response (ms)", layout.offsetKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Width Visual Response (ms)", layout.widthKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Mix Visual Response (ms)", layout.mixKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Knob Sweep Start (deg)", layout.knobSweepStartDeg, 0, 360);
    addLayout(layoutGlobalKnobResponseProps, "Knob Sweep End (deg)", layout.knobSweepEndDeg, 0, 360);
    addLayout(layoutGlobalKnobResponseProps, "Knob Frame Count", layout.knobFrameCount, 2, 156);

    addPanelSection(layoutPanel, "Global (Mix Knob Y, Color Value X)", layoutSliderProps, false);
    addPanelSection(layoutPanel, "Green Engine", layoutGreenProps, false);
    addPanelSection(layoutPanel, "Blue Engine", layoutBlueProps, false);
    addPanelSection(layoutPanel, "Red Engine", layoutRedProps, false);
    addPanelSection(layoutPanel, "Purple Engine", layoutPurpleProps, false);
    addPanelSection(layoutPanel, "Black Engine", layoutBlackProps, false);
    addPanelSection(layoutPanel, "Global Value Typography", layoutGlobalValueTypographyProps, false);
    addPanelSection(layoutPanel, "Global Value FX - Main Knobs", layoutGlobalValueFxProps, false);
    addPanelSection(layoutPanel, "Global Value FX - Color", layoutColorValueFxProps, false);
    addPanelSection(layoutPanel, "Global Value FX - Mix", layoutMixValueFxProps, false);
    addPanelSection(layoutPanel, "Value Flip - Main Knobs", layoutGlobalMainValueFlipProps, false);
    addPanelSection(layoutPanel, "Value Flip - Color", layoutGlobalColorValueFlipProps, false);
    addPanelSection(layoutPanel, "Value Flip - Mix", layoutGlobalMixValueFlipProps, false);
    addPanelSection(layoutPanel, "Top Buttons (About/Help/Feedback)", layoutGlobalTopButtonsProps, false);
    addPanelSection(layoutPanel, "Engine Selector (Combo + Popup)", layoutGlobalEngineSelectorProps, false);
    addPanelSection(layoutPanel, "HQ Flip Switch", layoutGlobalHqSwitchProps, false);
    addPanelSection(layoutPanel, "Global Knob Response", layoutGlobalKnobResponseProps, false);
    setEditingLocked(true);
    updateActiveProfileLabel();
    startTimerHz(10);
}

void DevPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    auto footer = bounds.removeFromBottom(36);
    fxPresetOffButton.setBounds(footer.removeFromLeft(84).reduced(0, 0));
    footer.removeFromLeft(6);
    fxPresetSubtleButton.setBounds(footer.removeFromLeft(92).reduced(0, 0));
    footer.removeFromLeft(6);
    fxPresetMediumButton.setBounds(footer.removeFromLeft(102).reduced(0, 0));
    copyJsonButton.setBounds(footer.removeFromRight(120));
    saveDefaultsButton.setBounds(footer.removeFromRight(190).reduced(6, 0));
    lockToggleButton.setBounds(footer.removeFromRight(130).reduced(6, 0));
    viewport.setBounds(bounds);

    const int columnGap = 24;
    const int contentWidth = bounds.getWidth();
    const int columnWidth = (contentWidth - columnGap) / 2;
    const int leftX = 0;
    const int rightX = columnWidth + columnGap;

    auto layoutSection = [&](int x, int& y, juce::Label& title, juce::Label& description, juce::PropertyPanel& panel)
    {
        const int titleH = 22;
        const int descH = 18;
        const int panelPadding = 8;

        title.setBounds(x, y, columnWidth, titleH);
        y += titleH;
        description.setBounds(x, y, columnWidth, descH);
        y += descH;

        const int panelHeight = panel.getTotalContentHeight() + panelPadding;
        panel.setBounds(x, y, columnWidth, panelHeight);
        y += panelHeight + 18;
    };

    int leftY = 0;
    int rightY = 0;

    layoutSection(leftX, leftY, mappingTitle, mappingDescription, mappingPanel);
    layoutSection(leftX, leftY, uiTitle, uiDescription, uiPanel);
    layoutSection(leftX, leftY, layoutTitle, layoutDescription, layoutPanel);

    const int activeProfileH = 18;
    activeProfileLabel.setBounds(rightX, rightY, columnWidth, activeProfileH);
    rightY += activeProfileH + 8;
    layoutSection(rightX, rightY, internalsTitle, internalsDescription, internalsPanel);
    layoutSection(rightX, rightY, bbdTitle, bbdDescription, bbdPanel);
    layoutSection(rightX, rightY, tapeTitle, tapeDescription, tapePanel);

    const int contentHeight = std::max(leftY, rightY);
    content.setBounds(0, 0, contentWidth, contentHeight);
}

void DevPanel::updateActiveProfileLabel()
{
    static const juce::String engineNames[] { "Green", "Blue", "Red", "Purple", "Black" };
    const int colorIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    const bool hqEnabled = processor.isHqEnabled();
    activeProfileLabel.setText("Active Profile: " + engineNames[colorIndex] + (hqEnabled ? " HQ" : " NQ"),
                               juce::dontSendNotification);
}

void DevPanel::timerCallback()
{
    updateActiveProfileLabel();
    if (saveButtonResetCountdownTicks > 0)
    {
        --saveButtonResetCountdownTicks;
        if (saveButtonResetCountdownTicks == 0)
        {
            saveDefaultsButton.setButtonText("Set Current as Defaults");
            saveDefaultsButton.setEnabled(true);
        }
    }
}

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
    json << "    \"mixKnobYOffset\": " << layout.mixKnobYOffsetGreen << ",\n";
    json << "    \"mixKnobYOffsetGreen\": " << layout.mixKnobYOffsetGreen << ",\n";
    json << "    \"mixKnobYOffsetBlue\": " << layout.mixKnobYOffsetBlue << ",\n";
    json << "    \"mixKnobYOffsetRed\": " << layout.mixKnobYOffsetRed << ",\n";
    json << "    \"mixKnobYOffsetPurple\": " << layout.mixKnobYOffsetPurple << ",\n";
    json << "    \"mixKnobYOffsetBlack\": " << layout.mixKnobYOffsetBlack << ",\n";
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
    json << "    \"mixValueFlipMinScalePct\": " << layout.mixValueFlipMinScalePct << "\n";
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
    const bool ok = DefaultsPersistence::save(json, &err);

    if (ok)
    {
        const auto roundTrip = juce::JSON::parse(DefaultsPersistence::load());
        const auto generated = juce::JSON::parse(json);
        const auto* root = roundTrip.getDynamicObject();
        const bool semanticMatch = !generated.isVoid() && varsEquivalent(generated, roundTrip);
        const bool hasCoreSections = (root != nullptr
            && root->hasProperty("tuning") && root->hasProperty("layout")
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

    layoutPanel.refreshAll();
    editor.applyLayout();
}
