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
#include "DevPanelBuildContext.h"
#include "../Plugin/PluginEditor.h"
#include "../Plugin/PluginProcessor.h"
#include "../Config/DefaultsPersistence.h"
#include "DevPanelSupport.h"
#include <unordered_set>

using namespace devpanel;

DevPanel::~DevPanel()
{
    if (tooltipWindow != nullptr)
        tooltipWindow->setLookAndFeel(nullptr);
    devEngineModeBox.setLookAndFeel(nullptr);
    content.setLookAndFeel(nullptr);
    viewport.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

DevPanel::DevPanel(ChoroborosPluginEditor& editorRef, ChoroborosAudioProcessor& processorRef)
    : editor(editorRef), processor(processorRef)
{
    setLookAndFeel(&getDevPanelThemeLookAndFeel());
    viewport.setLookAndFeel(&getDevPanelThemeLookAndFeel());
    content.setLookAndFeel(&getDevPanelThemeLookAndFeel());
    setCurrentEngineSkinColour(juce::jlimit(0, 4, processor.getCurrentEngineColorIndex()));
    getDevPanelThemeLookAndFeel().refreshThemeColours();
    getDevPanelSectionLookAndFeel().refreshThemeColours();
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 600);
    tooltipWindow->setLookAndFeel(&getDevPanelThemeLookAndFeel());
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
#if JUCE_WINDOWS
    // Windows GDI doesn't clear before repaint; content fills background to prevent scroll ghosting
    content.setOpaque(true);
#endif

    content.addAndMakeVisible(copyJsonButton);
    copyJsonButton.setButtonText("Copy JSON");
    copyJsonButton.setTooltip("Copies the active user defaults sheet JSON. If none exists yet, copies current in-memory values.");
    copyJsonButton.onClick = [this]
    {
        juce::String loadErr;
        auto json = DefaultsPersistence::loadUser(&loadErr);
        if (json.isEmpty())
            json = buildJson();
        juce::SystemClipboard::copyTextToClipboard(json);
    };

    content.addAndMakeVisible(saveDefaultsButton);
    saveDefaultsButton.setButtonText("Set Current as Defaults");
    saveDefaultsButton.setTooltip("Saves all current tuning, internals, and layout values to the user defaults sheet.");
    saveDefaultsButton.onClick = [this] { saveCurrentAsDefaults(); };

    content.addAndMakeVisible(resetFactoryButton);
    resetFactoryButton.setButtonText("Reset to Factory");
    resetFactoryButton.setTooltip("Resets runtime tuning/layout to factory baseline, writes factory sheet, then replaces user defaults with factory.");
    resetFactoryButton.onClick = [this] { resetToFactoryDefaults(); };

    content.addAndMakeVisible(lockToggleButton);
    lockToggleButton.setTooltip("Locks or unlocks all controls for safe editing.");
    lockToggleButton.onClick = [this] { setEditingLocked(!editingLocked); };
    content.addAndMakeVisible(fxPresetOffButton);
    fxPresetOffButton.setButtonText("FX Off");
    fxPresetOffButton.setTooltip("Applies an off preset to value glow/reflection FX groups.");
    fxPresetOffButton.onClick = [this] { applyValueFxPreset(0); };
    content.addAndMakeVisible(fxPresetSubtleButton);
    fxPresetSubtleButton.setButtonText("FX Subtle");
    fxPresetSubtleButton.setTooltip("Applies a subtle value FX preset.");
    fxPresetSubtleButton.onClick = [this] { applyValueFxPreset(1); };
    content.addAndMakeVisible(fxPresetMediumButton);
    fxPresetMediumButton.setButtonText("FX Medium");
    fxPresetMediumButton.setTooltip("Applies a medium-strength value FX preset.");
    fxPresetMediumButton.onClick = [this] { applyValueFxPreset(2); };

    auto configureTabButton = [this](juce::TextButton& button, const juce::String& name, int tabIndex)
    {
        button.setLookAndFeel(&getDevPanelThemeLookAndFeel());
        button.setButtonText(name);
        button.getProperties().set("devpanelPrimaryTab", true);
        button.setClickingTogglesState(true);
        button.setRadioGroupId(0x445650); // "DVP"
        button.onClick = [this, tabIndex]
        {
            selectedRightTab = tabIndex;
            selectedSubTabs[static_cast<size_t>(tabIndex)] =
                juce::jlimit(0, juce::jmax(0, getSubTabCount(tabIndex) - 1),
                             selectedSubTabs[static_cast<size_t>(tabIndex)]);
            refreshSecondaryTabButtons();
            updateActiveProfileLabel();
            updateRightTabVisibility();
            resized();
        };
        content.addAndMakeVisible(button);
    };
    configureTabButton(tabOverviewButton, "Overview", 0);
    configureTabButton(tabInternalsButton, "Modulation", 1);
    configureTabButton(tabBbdButton, "Tone", 2);
    configureTabButton(tabTapeButton, "Engine", 3);
    configureTabButton(tabLayoutButton, "Look & Feel", 4);
    configureTabButton(tabValidationButton, "Validation", 5);
    configureTabButton(tabSettingsButton, "Settings", 6);
    tabOverviewButton.setTooltip("Overview: signal flow and key diagnostics for the active profile selected above.");
    tabInternalsButton.setTooltip("Modulation: LFO scope and controls; writes to the active profile selected above.");
    tabBbdButton.setTooltip("Tone / Dynamics: spectral and transfer diagnostics for the active profile selected above.");
    tabTapeButton.setTooltip("Engine: engine-specific macros, internals, and advanced sections.");
    tabLayoutButton.setTooltip("Look & Feel: mapping, UI response, and layout tuning controls.");
    tabValidationButton.setTooltip("Validation: wiring trace matrix, telemetry, and live diagnostic log.");
    tabSettingsButton.setTooltip("Dev Panel settings: actions, presets, reset/defaults, and JSON export.");
    tabOverviewButton.setToggleState(true, juce::dontSendNotification);

    auto configureSubTabButton = [this](juce::TextButton& button, int subTabIndex)
    {
        button.setLookAndFeel(&getDevPanelThemeLookAndFeel());
        button.setClickingTogglesState(true);
        button.setRadioGroupId(0x445651); // "DVQ"
        button.onClick = [this, subTabIndex]
        {
            const int maxSubTab = juce::jmax(0, getSubTabCount(selectedRightTab) - 1);
            selectedSubTabs[static_cast<size_t>(selectedRightTab)] = juce::jlimit(0, maxSubTab, subTabIndex);
            refreshSecondaryTabButtons();
            updateActiveProfileLabel();
            updateRightTabVisibility();
            resized();
        };
        content.addAndMakeVisible(button);
    };
    configureSubTabButton(subTabButtonA, 0);
    configureSubTabButton(subTabButtonB, 1);
    configureSubTabButton(subTabButtonC, 2);
    configureSubTabButton(subTabButtonD, 3);

    engineFilterLabel.setText("Engine Sections", juce::dontSendNotification);
    engineFilterLabel.setFont(makeLabelFont(Typography::labelSmall, false));
    engineFilterLabel.setColour(juce::Label::textColourId, hackerTextDim());
    engineFilterLabel.setJustificationType(juce::Justification::centredLeft);
    content.addAndMakeVisible(engineFilterLabel);

    engineFilterEditor.setTooltip("Filter visible engine internals sections by name.");
    engineFilterEditor.setTextToShowWhenEmpty("type to focus engine sections (timing / tone / macro)", hackerTextMuted());
    engineFilterEditor.setFont(makeLabelFont(Typography::labelSmall, false));
    engineFilterEditor.onTextChange = [this]
    {
        updateRightTabVisibility();
        resized();
    };
    content.addAndMakeVisible(engineFilterEditor);

    engineFilterClearButton.setButtonText("Clear");
    engineFilterClearButton.setTooltip("Clear section filter.");
    engineFilterClearButton.onClick = [this]
    {
        engineFilterEditor.clear();
        updateRightTabVisibility();
        resized();
    };
    content.addAndMakeVisible(engineFilterClearButton);

    styleTitle(mappingTitle, "Parameter Mapping");
    styleDescription(mappingDescription, "Map UI values into DSP ranges (min/max/curve).");
    styleTitle(uiTitle, "UI Response");
    styleDescription(uiDescription, "Change slider feel without touching DSP.");
    styleTitle(overviewTitle, "Audio Overview");
    styleDescription(overviewDescription, "Start here: values shown are for the Active Profile above.");
    overviewTitle.setTooltip("Step 1: verify macro values and signal flow.");
    overviewDescription.setTooltip("Overview reflects the same active profile edited in Modulation/Tone/Engine.");
    styleTitle(modulationTitle, "Modulation");
    styleDescription(modulationDescription, "LFO controls here edit the Active Profile shown above.");
    modulationTitle.setTooltip("Step 2: modulation motion diagnostics.");
    modulationDescription.setTooltip("Visual cards, controls, and readouts here all target the active profile.");
    styleTitle(toneTitle, "Tone / Dynamics");
    styleDescription(toneDescription, "Tone analysis and controls here target the Active Profile above.");
    toneTitle.setTooltip("Step 3: spectral and nonlinear behavior.");
    toneDescription.setTooltip("Tone cards/readouts/controls are profile-scoped to the active selection above.");
    styleTitle(engineTitle, "Engine");
    styleDescription(engineDescription, "Engine-specific internals with linked macro readouts and flow card.");
    engineTitle.setTooltip("Step 4: engine-specific internals.");
    engineDescription.setTooltip("Switch engine/HQ here and inspect only relevant internals.");
    styleTitle(validationTitle, "Validation");
    styleDescription(validationDescription, "Telemetry + trace matrix: verify UI raw -> mapped -> snapshot -> DSP effective.");
    validationTitle.setTooltip("Step 5: confirm full wiring integrity.");
    validationDescription.setTooltip("Use telemetry and trace matrix to detect stale/no-op mappings.");
    styleTitle(internalsTitle, "DSP Internals (Per Engine + HQ)");
    styleDescription(internalsDescription, "Independent profiles for each engine in Normal and HQ modes.");
    styleTitle(bbdTitle, "BBD Profiles (Red NQ)");
    styleDescription(bbdDescription, "Only shown for Red Normal, where the BBD core is active.");
    styleTitle(tapeTitle, "Tape Profiles (Red HQ)");
    styleDescription(tapeDescription, "Only shown for Red HQ, where the Tape core is active.");
    styleTitle(layoutTitle, "Visual Layout");
    styleDescription(layoutDescription, "Live placement + sizing of UI elements (grouped by engine color).");
    styleTitle(settingsTitle, "Dev Panel Settings");
    styleDescription(settingsDescription, "Action controls and defaults workflow for this Dev Panel.");
    styleTitle(inspectorTitle, "Inspector");
    styleDescription(inspectorDescription, "Controls and readouts for the selected tab/subview. Expand sections to edit.");
    inspectorTitle.setTooltip("Left column inspector: this is where you edit controls and view linked readouts.");
    inspectorDescription.setTooltip("Inspector follows the selected tab/subview on the right.");
    styleDescription(activeProfileLabel, "Active Profile: Green NQ");
    activeProfileLabel.setColour(juce::Label::textColourId, hackerText());
    activeProfileLabel.setJustificationType(juce::Justification::centredLeft);
    activeProfileLabel.setTooltip("Current target profile. Overview/Modulation/Tone/Engine controls all edit this profile.");
    styleDescription(activeScopeHintLabel, "Scope: Overview/Modulation/Tone/Engine all edit this active profile.");
    activeScopeHintLabel.setColour(juce::Label::textColourId, hackerTextDim());
    activeScopeHintLabel.setJustificationType(juce::Justification::centredLeft);
    activeScopeHintLabel.setTooltip("Changing Engine/HQ changes which profile is being edited.");
    devEngineModeLabel.setText("Profile", juce::dontSendNotification);
    devEngineModeLabel.setFont(makeLabelFont(Typography::labelSmall, false));
    devEngineModeLabel.setColour(juce::Label::textColourId, hackerTextDim());
    devEngineModeLabel.setJustificationType(juce::Justification::centredLeft);

    devEngineModeBox.addItem("Green", 1);
    devEngineModeBox.addItem("Blue", 2);
    devEngineModeBox.addItem("Red", 3);
    devEngineModeBox.addItem("Purple", 4);
    devEngineModeBox.addItem("Black", 5);
    devEngineModeBox.setTooltip("Switch engine directly from the Dev Panel.");
    devEngineModeBox.onChange = [this]
    {
        if (suppressDevModeControlCallbacks)
            return;
        const int selectedEngine = juce::jlimit(0, 4, devEngineModeBox.getSelectedId() - 1);
        setCurrentEngineSkinColour(selectedEngine);
        getDevPanelThemeLookAndFeel().refreshThemeColours();
        getDevPanelSectionLookAndFeel().refreshThemeColours();
        sendLookAndFeelChange();
        styleProfileSelectorComboBox(devEngineModeBox, profileSelectorColourForEngineIndex(selectedEngine));
        if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
        {
            const float normalized = processor.getValueTreeState().getParameterRange(ChoroborosAudioProcessor::ENGINE_COLOR_ID)
                                         .convertTo0to1(static_cast<float>(selectedEngine));
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalized);
            param->endChangeGesture();
        }
        updateActiveProfileLabel();
        updateRightTabVisibility();
        resized();
        repaint();
    };

    devHqModeToggle.setButtonText("HQ");
    devHqModeToggle.setTooltip("Toggle NQ/HQ directly from the Dev Panel.");
    devHqModeToggle.onClick = [this]
    {
        if (suppressDevModeControlCallbacks)
            return;
        if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::HQ_ID))
        {
            const float normalized = devHqModeToggle.getToggleState() ? 1.0f : 0.0f;
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalized);
            param->endChangeGesture();
        }
        updateActiveProfileLabel();
        updateRightTabVisibility();
        resized();
        repaint();
    };

    content.addAndMakeVisible(mappingTitle);
    content.addAndMakeVisible(mappingDescription);
    content.addAndMakeVisible(uiTitle);
    content.addAndMakeVisible(uiDescription);
    content.addAndMakeVisible(overviewTitle);
    content.addAndMakeVisible(overviewDescription);
    content.addAndMakeVisible(modulationTitle);
    content.addAndMakeVisible(modulationDescription);
    content.addAndMakeVisible(toneTitle);
    content.addAndMakeVisible(toneDescription);
    content.addAndMakeVisible(engineTitle);
    content.addAndMakeVisible(engineDescription);
    content.addAndMakeVisible(validationTitle);
    content.addAndMakeVisible(validationDescription);
    content.addAndMakeVisible(internalsTitle);
    content.addAndMakeVisible(internalsDescription);
    content.addAndMakeVisible(bbdTitle);
    content.addAndMakeVisible(bbdDescription);
    content.addAndMakeVisible(tapeTitle);
    content.addAndMakeVisible(tapeDescription);
    content.addAndMakeVisible(layoutTitle);
    content.addAndMakeVisible(layoutDescription);
    content.addAndMakeVisible(settingsTitle);
    content.addAndMakeVisible(settingsDescription);
    content.addAndMakeVisible(inspectorTitle);
    content.addAndMakeVisible(inspectorDescription);
    content.addAndMakeVisible(activeProfileLabel);
    content.addAndMakeVisible(activeScopeHintLabel);
    content.addAndMakeVisible(devEngineModeLabel);
    content.addAndMakeVisible(devEngineModeBox);
    content.addAndMakeVisible(devHqModeToggle);
    content.addAndMakeVisible(overviewVisualDeck);
    content.addAndMakeVisible(modulationVisualDeck);
    content.addAndMakeVisible(toneVisualDeck);
    content.addAndMakeVisible(engineVisualDeck);
    content.addAndMakeVisible(lookFeelVisualDeck);
    content.addAndMakeVisible(validationVisualDeck);

    overviewVisualDeck.setAccentColour(visualOverview());
    modulationVisualDeck.setAccentColour(visualModulation());
    toneVisualDeck.setAccentColour(visualTone());
    engineVisualDeck.setAccentColour(visualEngine());
    lookFeelVisualDeck.setAccentColour(visualLayout());
    validationVisualDeck.setAccentColour(visualValidation());
    overviewVisualDeck.setTooltip("Overview visualizer area.");
    modulationVisualDeck.setTooltip("Start with motion cards here; linked controls/readouts below all write to the active profile.");
    toneVisualDeck.setTooltip("Start with spectrum and transfer cards here, then use linked tone readouts below.");
    engineVisualDeck.setTooltip("Start with engine signal flow here, then use linked engine readouts below.");
    lookFeelVisualDeck.setTooltip("Live UI preview with grid and coordinates for Look & Feel tuning.");
    validationVisualDeck.setTooltip("Validation visuals: trace matrix and live log.");

    content.addAndMakeVisible(mappingPanel);
    content.addAndMakeVisible(uiPanel);
    content.addAndMakeVisible(overviewPanel);
    content.addAndMakeVisible(modulationPanel);
    content.addAndMakeVisible(tonePanel);
    content.addAndMakeVisible(enginePanel);
    content.addAndMakeVisible(validationPanel);
    content.addAndMakeVisible(internalsGreenNqPanel);
    content.addAndMakeVisible(internalsGreenHqPanel);
    content.addAndMakeVisible(internalsBlueNqPanel);
    content.addAndMakeVisible(internalsBlueHqPanel);
    content.addAndMakeVisible(internalsRedNqPanel);
    content.addAndMakeVisible(internalsRedHqPanel);
    content.addAndMakeVisible(internalsPurpleNqPanel);
    content.addAndMakeVisible(internalsPurpleHqPanel);
    content.addAndMakeVisible(internalsBlackNqPanel);
    content.addAndMakeVisible(internalsBlackHqPanel);
    content.addAndMakeVisible(bbdPanel);
    content.addAndMakeVisible(tapePanel);
    content.addAndMakeVisible(layoutGlobalPanel);
    content.addAndMakeVisible(layoutGreenPanel);
    content.addAndMakeVisible(layoutBluePanel);
    content.addAndMakeVisible(layoutRedPanel);
    content.addAndMakeVisible(layoutPurplePanel);
    content.addAndMakeVisible(layoutBlackPanel);

    auto styleActionButton = [](juce::TextButton& button)
    {
        button.setLookAndFeel(&getDevPanelThemeLookAndFeel());
        styleHackerTextButton(button, false);
    };
    styleActionButton(copyJsonButton);
    styleActionButton(saveDefaultsButton);
    styleActionButton(resetFactoryButton);
    styleActionButton(lockToggleButton);
    styleActionButton(fxPresetOffButton);
    styleActionButton(fxPresetSubtleButton);
    styleActionButton(fxPresetMediumButton);
    styleActionButton(engineFilterClearButton);

    styleProfileSelectorComboBox(devEngineModeBox, profileSelectorColourForEngineIndex(processor.getCurrentEngineColorIndex()));
    styleHackerToggleButton(devHqModeToggle);
    styleHackerEditor(engineFilterEditor);


    auto& tuning = processor.getTuningState();

    const auto makeTooltipForControl = [](const juce::String& name) -> juce::String
    {
        const juce::String n = name.toLowerCase();

        if (n.contains("bbd filter max ratio"))
            return "Caps BBD filter cutoff as a sample-rate ratio (0.1-0.5). Lower values are darker/safer; higher values are brighter.";
        if (n.contains("bbd stages") || n.contains("stages"))
            return "Number of BBD delay stages (256-2048). More stages increase max delay and change clock behavior.";
        if (n.contains("bbd"))
            return "Used only in Red NQ (BBD core). BBD emulates analog bucket-brigade delay behavior. Slower clock and lower cutoff sound darker and grainier; faster clock and higher cutoff sound cleaner and brighter.";
        if (n.contains("tape"))
            return "Used only in Red HQ (Tape core). Tape controls shape wow/flutter pitch drift, tone rolloff, and nonlinear drive. Small moves can be musical; big moves can sound unstable or lo-fi.";
        if (n.contains("bloom"))
            return "Green engine wet-character macro. Controls body, softness, and modulation bloom as Color increases.";
        if (n.contains("focus") || n.contains("presence"))
            return "Blue engine wet-character macro. Controls clarity shaping, presence lift, and transient focus as Color increases.";
        if (n.contains("warp"))
            return "Purple NQ shape parameters for phase-warp motion. Higher values create more complex, nonlinear modulation paths.";
        if (n.contains("orbit"))
            return "Purple HQ orbit parameters for dual-tap spatial motion. Affects eccentricity, rotation rates, and tap balance.";
        if (n.contains("ensemble"))
            return "Black HQ ensemble spread controls for second-tap blend, depth, and spacing.";

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
        if (n.contains("lpf"))
            return "Low-pass filter rolls off high frequencies. Use to tame BBD aliasing, clock buzz, or harsh highs. Lower cutoff = darker.";
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

    auto makeLiveMappedControl = [&](const juce::String& name, const char* paramId,
                                     float minDisplay, float maxDisplay,
                                     float stepDisplay, float skew,
                                     float displayScale = 1.0f) -> juce::PropertyComponent*
    {
        const float safeScale = juce::jmax(0.0001f, displayScale);
        return makeLockable(
            makeFloatValue(
                [this, paramId, safeScale]
                {
                    if (auto* raw = processor.getValueTreeState().getRawParameterValue(paramId))
                        return processor.mapParameterValue(paramId, raw->load()) * safeScale;
                    return 0.0f;
                },
                [this, paramId, minDisplay, maxDisplay, safeScale](float displayValue)
                {
                    if (auto* param = processor.getValueTreeState().getParameter(paramId))
                    {
                        const float clampedDisplay = juce::jlimit(minDisplay, maxDisplay, displayValue);
                        const float mappedValue = clampedDisplay / safeScale;
                        const float rawValue = processor.unmapParameterValue(paramId, mappedValue);
                        const auto& range = processor.getValueTreeState().getParameterRange(paramId);
                        const float normalized = juce::jlimit(0.0f, 1.0f, range.convertTo0to1(rawValue));
                        param->setValueNotifyingHost(normalized);
                    }
                }),
            name,
            static_cast<double>(minDisplay),
            static_cast<double>(maxDisplay),
            static_cast<double>(stepDisplay),
            static_cast<double>(skew));
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

    auto readRawParam = [this](const char* paramId) -> float
    {
        if (auto* p = processor.getValueTreeState().getRawParameterValue(paramId))
            return p->load();
        return 0.0f;
    };
    auto getActiveProfileRaw = [this, readRawParam](const char* paramId, bool& profileValid) -> float
    {
        const auto& profiles = processor.getEngineParamProfiles();
        const int engineIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const auto& profile = profiles[static_cast<size_t>(engineIndex)];
        if (!profile.valid)
        {
            profileValid = false;
            return readRawParam(paramId);
        }

        profileValid = true;
        if (juce::String(paramId) == ChoroborosAudioProcessor::RATE_ID)
            return profile.rate;
        if (juce::String(paramId) == ChoroborosAudioProcessor::DEPTH_ID)
            return profile.depth;
        if (juce::String(paramId) == ChoroborosAudioProcessor::OFFSET_ID)
            return profile.offset;
        if (juce::String(paramId) == ChoroborosAudioProcessor::WIDTH_ID)
            return profile.width;
        if (juce::String(paramId) == ChoroborosAudioProcessor::COLOR_ID)
            return profile.color;
        if (juce::String(paramId) == ChoroborosAudioProcessor::MIX_ID)
            return profile.mix;
        return readRawParam(paramId);
    };
    auto readAnalyzerSnapshot = [this]() -> ChoroborosAudioProcessor::AnalyzerSnapshot
    {
        ChoroborosAudioProcessor::AnalyzerSnapshot snapshot;
        processor.getAnalyzerSnapshot(snapshot);
        return snapshot;
    };
    std::vector<ControlMetadata> metadataRegistry;
    metadataRegistry.reserve(512);
    std::unordered_set<std::string> metadataIds;

    auto inferStage = [](const juce::String& label) -> juce::String
    {
        const auto n = label.toLowerCase();
        if (n.contains("layout") || n.contains("flip") || n.contains("button")
            || n.contains("selector") || n.contains("font") || n.contains("knob sweep"))
            return "layout";
        if (n.contains("trace") || n.contains("validation") || n.contains("sync")
            || n.contains("wiring") || n.contains("telemetry") || n.contains("recent"))
            return "validation";
        if (n.contains("lfo") || n.contains("delay") || n.contains("rate")
            || n.contains("depth") || n.contains("offset") || n.contains("width")
            || n.contains("trajectory"))
            return "modulation";
        if (n.contains("hpf") || n.contains("lpf") || n.contains("pre")
            || n.contains("tone") || n.contains("compressor") || n.contains("saturation")
            || n.contains("spectrum") || n.contains("transfer"))
            return "tone_dynamics";
        if (n.contains("engine") || n.contains("bloom") || n.contains("focus")
            || n.contains("warp") || n.contains("orbit") || n.contains("bbd")
            || n.contains("tape") || n.contains("ensemble") || n.contains("intensity"))
            return "engine";
        return "general";
    };

    auto inferAppliesTo = [](const juce::String& label) -> std::uint16_t
    {
        constexpr std::uint16_t allModes = 0x03ffu;
        const auto n = label.toLowerCase();
        auto maskForEngine = [](int engineIndex) -> std::uint16_t
        {
            const int bit = juce::jlimit(0, 4, engineIndex) * 2;
            return static_cast<std::uint16_t>((1u << bit) | (1u << (bit + 1)));
        };
        if (n.contains("red nq")) return static_cast<std::uint16_t>(1u << (2 * 2));
        if (n.contains("red hq")) return static_cast<std::uint16_t>(1u << (2 * 2 + 1));
        if (n.contains("green")) return maskForEngine(0);
        if (n.contains("blue")) return maskForEngine(1);
        if (n.contains("red")) return maskForEngine(2);
        if (n.contains("purple")) return maskForEngine(3);
        if (n.contains("black")) return maskForEngine(4);
        return allModes;
    };

    auto registerControlMetadata = [&](const juce::String& label,
                                       const juce::String& visualCardId,
                                       const juce::String& rawToMappedHook,
                                       const juce::String& effectiveProbeHook,
                                       const juce::String& noVisualReason)
    {
        juce::String baseId = label.toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
        if (baseId.isEmpty())
            baseId = "control";
        juce::String controlId = baseId;
        int suffix = 2;
        while (metadataIds.count(controlId.toStdString()) > 0)
        {
            controlId = baseId + juce::String(suffix);
            ++suffix;
        }
        metadataIds.insert(controlId.toStdString());

        ControlMetadata metadata;
        metadata.id = controlId;
        metadata.label = label;
        metadata.stage = inferStage(label);
        metadata.appliesToMask = inferAppliesTo(label);
        metadata.rawToMappedHook = rawToMappedHook;
        metadata.effectiveProbeHook = effectiveProbeHook;
        metadata.visualCardId = visualCardId;
        metadata.noVisualReason = noVisualReason;
        metadataRegistry.push_back(std::move(metadata));
    };

    auto makeReadOnly = [this, &makeTooltipForControl, &registerControlMetadata](const juce::String& name, std::function<juce::String()> valueProvider)
    {
        auto* prop = new ReadOnlyDiagnosticPropertyComponent(name, std::move(valueProvider), makeTooltipForControl(name));
        liveReadoutProperties.add(prop);
        registerControlMetadata(name, {}, "read_only", "derived_probe", "readout_only");
        return prop;
    };
    auto makeMultiLineReadOnly = [this, &makeTooltipForControl, &registerControlMetadata](const juce::String& name, std::function<juce::String()> valueProvider)
    {
        auto* prop = new MultiLineReadOnlyPropertyComponent(name, std::move(valueProvider), makeTooltipForControl(name));
        liveReadoutProperties.add(prop);
        registerControlMetadata(name, {}, "read_only", "derived_probe", "readout_only");
        return prop;
    };

    auto makeSparkline = [this, &makeTooltipForControl, &registerControlMetadata](const juce::String& name, std::function<std::vector<float>()> valueProvider, LinkGroup linkGroup = LinkGroup::none)
    {
        auto* prop = new SparklinePropertyComponent(name, std::move(valueProvider), makeTooltipForControl(name),
                                                    [this, linkGroup](bool active)
                                                    {
                                                        if (linkGroup != LinkGroup::none)
                                                        {
                                                            setLinkedGroup(active ? linkGroup : LinkGroup::none);
                                                            overviewPanel.repaint();
                                                            modulationPanel.repaint();
                                                            tonePanel.repaint();
                                                            enginePanel.repaint();
                                                            validationPanel.repaint();
                                                        }
                                                    });
        liveReadoutProperties.add(prop);
        modulationVisualizerProperties.add(prop);
        const juce::String card = name.containsIgnoreCase("lfo") ? "lfo_scope"
                                : (name.containsIgnoreCase("delay") ? "delay_trajectory" : "sparkline");
        registerControlMetadata(name, card, "analyzer_snapshot", "visualized", {});
        return prop;
    };
    auto makeTransferCurve = [this, &makeTooltipForControl, &registerControlMetadata](const juce::String& name,
                                                             std::function<TransferCurvePropertyComponent::State()> stateProvider,
                                                             LinkGroup linkGroup = LinkGroup::none)
    {
        auto* prop = new TransferCurvePropertyComponent(name, std::move(stateProvider), makeTooltipForControl(name),
                                                        [this, linkGroup](bool active)
                                                        {
                                                            if (linkGroup != LinkGroup::none)
                                                            {
                                                                setLinkedGroup(active ? linkGroup : LinkGroup::none);
                                                                overviewPanel.repaint();
                                                                modulationPanel.repaint();
                                                                tonePanel.repaint();
                                                                enginePanel.repaint();
                                                                validationPanel.repaint();
                                                            }
                                                        });
        liveReadoutProperties.add(prop);
        transferVisualizerProperties.add(prop);
        registerControlMetadata(name, "transfer_curve", "raw->mapped->measured", "transfer_probe", {});
        return prop;
    };
    auto makeSpectrumOverlay = [this, &makeTooltipForControl, &registerControlMetadata](const juce::String& name,
                                                              std::function<SpectrumOverlayPropertyComponent::State()> stateProvider,
                                                              LinkGroup linkGroup = LinkGroup::none)
    {
        auto* prop = new SpectrumOverlayPropertyComponent(name, std::move(stateProvider), makeTooltipForControl(name),
                                                          [this, linkGroup](bool active)
                                                          {
                                                              if (linkGroup != LinkGroup::none)
                                                              {
                                                                  setLinkedGroup(active ? linkGroup : LinkGroup::none);
                                                                  overviewPanel.repaint();
                                                                  modulationPanel.repaint();
                                                                  tonePanel.repaint();
                                                                  enginePanel.repaint();
                                                                  validationPanel.repaint();
                                                              }
                                                          });
        liveReadoutProperties.add(prop);
        spectrumVisualizerProperties.add(prop);
        registerControlMetadata(name, "spectrum_overlay", "raw->mapped", "spectrum_probe", {});
        return prop;
    };
    auto makeSignalFlow = [this, &makeTooltipForControl, &registerControlMetadata](const juce::String& name,
                                                         std::function<SignalFlowPropertyComponent::State()> stateProvider,
                                                         LinkGroup linkGroup = LinkGroup::none)
    {
        auto* prop = new SignalFlowPropertyComponent(name, std::move(stateProvider), makeTooltipForControl(name),
                                                     [this, linkGroup](bool active)
                                                     {
                                                         if (linkGroup != LinkGroup::none)
                                                         {
                                                             setLinkedGroup(active ? linkGroup : LinkGroup::none);
                                                             overviewPanel.repaint();
                                                             modulationPanel.repaint();
                                                             tonePanel.repaint();
                                                             enginePanel.repaint();
                                                             validationPanel.repaint();
                                                         }
                                                     });
        liveReadoutProperties.add(prop);
        modulationVisualizerProperties.add(prop);
        analyzerTelemetryProperties.add(prop);
        registerControlMetadata(name, "signal_flow", "runtime", "signal_path_probe", {});
        return prop;
    };
    auto makeTraceMatrix = [this, &makeTooltipForControl, &registerControlMetadata](const juce::String& name,
                                                          std::function<ValidationTraceMatrixPropertyComponent::State()> stateProvider)
    {
        auto* prop = new ValidationTraceMatrixPropertyComponent(name, std::move(stateProvider), makeTooltipForControl(name));
        liveReadoutProperties.add(prop);
        registerControlMetadata(name, "trace_matrix", "ui->mapped->snapshot", "effective_probe", {});
        return prop;
    };

    DevPanelBuildContext ctx;
    ctx.makeLockable = makeLockable;
    ctx.makeLiveMappedControl = makeLiveMappedControl;
    ctx.makeReadOnly = makeReadOnly;
    ctx.makeMultiLineReadOnly = makeMultiLineReadOnly;
    ctx.makeSparkline = makeSparkline;
    ctx.makeTransferCurve = makeTransferCurve;
    ctx.makeSpectrumOverlay = makeSpectrumOverlay;
    ctx.makeSignalFlow = makeSignalFlow;
    ctx.makeTraceMatrix = makeTraceMatrix;
    ctx.readRawParam = readRawParam;
    ctx.getActiveProfileRaw = getActiveProfileRaw;
    ctx.readAnalyzerSnapshot = readAnalyzerSnapshot;
    ctx.registerControlMetadata = registerControlMetadata;

    buildOverviewTab(ctx);
    buildModulationTab(ctx);
    buildToneTab(ctx);
    buildEngineTab(ctx);
    buildValidationTab(ctx);
    buildInternalsTab(ctx);
    buildLayoutTab(ctx);

    for (auto* property : lockableProperties)
    {
        if (property == nullptr)
            continue;
        registerControlMetadata(property->getName(), {}, "raw->mapped", "effective_runtime", "control_only");
    }
    registerControlMetadata("Dev Engine Mode", {}, "meta_choice", "engine_switch", "control_only");
    registerControlMetadata("Dev HQ Mode", {}, "meta_toggle", "hq_toggle", "control_only");
    registerControlMetadata("Engine Section Filter", {}, "ui_filter", "visibility_only", "control_only");

    metadataControlCount = static_cast<int>(metadataRegistry.size());
    metadataVisualMappedCount = 0;
    for (const auto& metadata : metadataRegistry)
    {
        if (metadata.visualCardId.isNotEmpty())
            ++metadataVisualMappedCount;
    }
    metadataNoVisualCount = metadataControlCount - metadataVisualMappedCount;

    const bool hasDuplicateIds = static_cast<int>(metadataIds.size()) != metadataControlCount;
    if (metadataControlCount <= 0)
        metadataValidationText = "WARN: no controls registered.";
    else if (hasDuplicateIds)
        metadataValidationText = "WARN: duplicate control IDs detected.";
    else
        metadataValidationText = "OK: registry is stable; each control maps to visual or explicit no-visual reason.";

    setEditingLocked(true);
    refreshSecondaryTabButtons();
    updateActiveProfileLabel();
    updateRightTabVisibility();
    lastKnownEngineIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    lastKnownHqState = processor.isHqEnabled();
    lastKnownSectionFilter = engineFilterEditor.getText().trim();
    startTimerHz(kDevPanelTimerHz);
}
