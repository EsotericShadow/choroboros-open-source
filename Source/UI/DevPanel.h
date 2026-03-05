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

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class ChoroborosPluginEditor;
class ChoroborosAudioProcessor;
struct DevPanelBuildContext;
namespace devpanel
{
struct ConsoleCommandResult;
class LockableFloatPropertyComponent;
}

/** Fills background on paint; prevents Windows GDI scroll ghosting. */
class DevPanelContent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        juce::ColourGradient bg(juce::Colour(0xff040704),
                                area.getX(), area.getY(),
                                juce::Colour(0xff0a120d),
                                area.getRight(), area.getBottom(),
                                false);
        g.setGradientFill(bg);
        g.fillAll();
    }
};

class VisualDeckContent : public juce::Component,
                          public juce::SettableTooltipClient
{
public:
    void setAccentColour(juce::Colour c) { accentColour = c; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        juce::ColourGradient bg(juce::Colour(0xff081008).withAlpha(0.95f),
                                area.getX(), area.getY(),
                                juce::Colour(0xff0d170f).withAlpha(0.88f),
                                area.getRight(), area.getBottom(),
                                false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(area, 6.0f);

        juce::ColourGradient accentWash(accentColour.withAlpha(0.22f),
                                        area.getX(), area.getY() + 2.0f,
                                        accentColour.withAlpha(0.06f),
                                        area.getX(), area.getBottom(),
                                        false);
        g.setGradientFill(accentWash);
        g.fillRoundedRectangle(area.reduced(1.0f), 6.0f);

        g.setColour(accentColour.withAlpha(0.62f));
        g.drawRoundedRectangle(area.reduced(0.5f), 6.0f, 1.0f);
    }

private:
    juce::Colour accentColour { juce::Colour(0xff80ef80) };
};

class TutorialOverlayContainer final : public juce::Component
{
public:
    void setAccentColour(juce::Colour accent)
    {
        accentColour = accent;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        juce::ColourGradient bg(juce::Colour(0xee060b08),
                                area.getX(), area.getY(),
                                juce::Colour(0xee0d1711),
                                area.getRight(), area.getBottom(),
                                false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(area, 10.0f);

        juce::ColourGradient wash(accentColour.withAlpha(0.24f),
                                  area.getX(), area.getY(),
                                  accentColour.withAlpha(0.06f),
                                  area.getX(), area.getBottom(),
                                  false);
        g.setGradientFill(wash);
        g.fillRoundedRectangle(area.reduced(1.0f), 10.0f);

        g.setColour(accentColour.withAlpha(0.8f));
        g.drawRoundedRectangle(area.reduced(0.5f), 10.0f, 1.2f);
    }

private:
    juce::Colour accentColour { juce::Colour(0xff80ef80) };
};

class TutorialFocusHighlight final : public juce::Component
{
public:
    void setAccentColour(juce::Colour accent)
    {
        accentColour = accent;
        repaint();
    }

    void setPulseAlpha(float alpha)
    {
        pulseAlpha = juce::jlimit(0.05f, 1.0f, alpha);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(accentColour.withAlpha(0.1f * pulseAlpha));
        g.fillRoundedRectangle(area, 9.0f);
        g.setColour(accentColour.withAlpha(0.88f * pulseAlpha));
        g.drawRoundedRectangle(area, 9.0f, 2.0f);
        g.setColour(accentColour.withAlpha(0.45f * pulseAlpha));
        g.drawRoundedRectangle(area.reduced(4.0f), 7.0f, 1.0f);
    }

private:
    juce::Colour accentColour { juce::Colour(0xff80ef80) };
    float pulseAlpha = 1.0f;
};

class DevPanel : public juce::Component,
                 private juce::Timer
{
public:
    ~DevPanel() override;
    DevPanel(ChoroborosPluginEditor& editorRef, ChoroborosAudioProcessor& processorRef);
    void resized() override;

private:
    ChoroborosPluginEditor& editor;
    ChoroborosAudioProcessor& processor;

    juce::Viewport viewport;
    DevPanelContent content;
    juce::Label mappingTitle;
    juce::Label mappingDescription;
    juce::Label uiTitle;
    juce::Label uiDescription;
    juce::Label overviewTitle;
    juce::Label overviewDescription;
    juce::Label modulationTitle;
    juce::Label modulationDescription;
    juce::Label toneTitle;
    juce::Label toneDescription;
    juce::Label engineTitle;
    juce::Label engineDescription;
    juce::Label validationTitle;
    juce::Label validationDescription;
    juce::Label internalsTitle;
    juce::Label internalsDescription;
    juce::Label bbdTitle;
    juce::Label bbdDescription;
    juce::Label tapeTitle;
    juce::Label tapeDescription;
    juce::Label layoutTitle;
    juce::Label layoutDescription;
    juce::Label settingsTitle;
    juce::Label settingsDescription;
    juce::Label activeProfileLabel;
    juce::Label activeScopeHintLabel;
    juce::Label devEngineModeLabel;
    juce::Label engineFilterLabel;
    juce::Label inspectorTitle;
    juce::Label inspectorDescription;
    VisualDeckContent overviewVisualDeck;
    VisualDeckContent modulationVisualDeck;
    VisualDeckContent toneVisualDeck;
    VisualDeckContent engineVisualDeck;
    VisualDeckContent lookFeelVisualDeck;
    VisualDeckContent validationVisualDeck;
    TutorialOverlayContainer tutorialOverlay;
    TutorialFocusHighlight tutorialFocusHighlight;
    juce::Label tutorialStepLabel;
    juce::Label tutorialTitleLabel;
    juce::TextEditor tutorialBodyText;
    juce::Label tutorialFocusHintLabel;
    juce::TextButton tutorialNextButton;
    juce::TextButton tutorialNextSectionButton;
    juce::TextButton tutorialSkipButton;
    juce::PropertyPanel mappingPanel;
    juce::PropertyPanel uiPanel;
    juce::PropertyPanel overviewPanel;
    juce::PropertyPanel modulationPanel;
    juce::PropertyPanel tonePanel;
    juce::PropertyPanel enginePanel;
    juce::PropertyPanel validationPanel;
    juce::PropertyPanel internalsGreenNqPanel;
    juce::PropertyPanel internalsGreenHqPanel;
    juce::PropertyPanel internalsBlueNqPanel;
    juce::PropertyPanel internalsBlueHqPanel;
    juce::PropertyPanel internalsRedNqPanel;
    juce::PropertyPanel internalsRedHqPanel;
    juce::PropertyPanel internalsPurpleNqPanel;
    juce::PropertyPanel internalsPurpleHqPanel;
    juce::PropertyPanel internalsBlackNqPanel;
    juce::PropertyPanel internalsBlackHqPanel;
    juce::PropertyPanel bbdPanel;
    juce::PropertyPanel tapePanel;
    juce::PropertyPanel layoutGlobalPanel;
    juce::PropertyPanel layoutTextAnimationPanel;
    juce::PropertyPanel layoutGreenPanel;
    juce::PropertyPanel layoutBluePanel;
    juce::PropertyPanel layoutRedPanel;
    juce::PropertyPanel layoutPurplePanel;
    juce::PropertyPanel layoutBlackPanel;
    juce::Array<juce::PropertyComponent*> lockableProperties;
    juce::Array<juce::PropertyComponent*> liveReadoutProperties;
    juce::Array<juce::PropertyComponent*> overviewVisualDeckCards;
    juce::Array<juce::PropertyComponent*> modulationVisualDeckCards;
    juce::Array<juce::PropertyComponent*> toneVisualDeckCards;
    juce::Array<juce::PropertyComponent*> engineVisualDeckCards;
    juce::Array<juce::PropertyComponent*> lookFeelVisualDeckCards;
    juce::Array<juce::PropertyComponent*> validationVisualDeckCards;
    juce::Array<juce::PropertyComponent*> modulationVisualizerProperties;
    juce::Array<juce::PropertyComponent*> spectrumVisualizerProperties;
    juce::Array<juce::PropertyComponent*> transferVisualizerProperties;
    juce::Array<juce::PropertyComponent*> analyzerTelemetryProperties;
    juce::TextButton copyJsonButton;
    juce::TextButton saveDefaultsButton;
    juce::TextButton resetFactoryButton;
    juce::TextButton lockToggleButton;
    juce::TextButton fxPresetOffButton;
    juce::TextButton fxPresetSubtleButton;
    juce::TextButton fxPresetMediumButton;
    juce::TextButton tabOverviewButton;
    juce::TextButton tabInternalsButton;
    juce::TextButton tabBbdButton;
    juce::TextButton tabTapeButton;
    juce::TextButton tabLayoutButton;
    juce::TextButton tabValidationButton;
    juce::TextButton tabSettingsButton;
    juce::TextButton subTabButtonA;
    juce::TextButton subTabButtonB;
    juce::TextButton subTabButtonC;
    juce::TextButton subTabButtonD;
    juce::TextButton subTabButtonE;
    juce::ComboBox devEngineModeBox;
    juce::ToggleButton devHqModeToggle;
    juce::TextEditor engineFilterEditor;
    juce::TextButton engineFilterClearButton;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    bool editingLocked = true;
    bool suppressDevModeControlCallbacks = false;
    int saveButtonResetCountdownTicks = 0;
    int selectedRightTab = 0; // 0=Overview, 1=Modulation, 2=Tone, 3=Engine, 4=Look&Feel, 5=Validation, 6=Settings
    std::array<int, 7> selectedSubTabs { { 0, 0, 0, 0, 0, 0, 0 } };
    bool macroTouchHistoryPrimed = false;
    float lastRateRaw = 0.0f;
    float lastDepthRaw = 0.0f;
    float lastOffsetRaw = 0.0f;
    float lastWidthRaw = 0.0f;
    float lastColorRaw = 0.0f;
    float lastMixRaw = 0.0f;
    int lastKnownEngineIndex = -1;
    bool lastKnownHqState = false;
    juce::String lastKnownSectionFilter;
    juce::StringArray recentTouchHistory;
    struct ConsoleTargetBinding
    {
        juce::String slug;
        juce::String displayName;
        juce::PropertyComponent* property = nullptr;
        double baselineValue = 0.0;
        double lastKnownValue = 0.0;
        double previousValue = 0.0;
        bool hasPreviousValue = false;
        bool engineSpecific = false;
        int engineScope = -1; // -1 = global, 0..4 = engine-specific
    };
    struct ConsoleAction
    {
        juce::String label;
        std::function<void()> undo;
        std::function<void()> redo;
    };
    struct ConsoleSweepState
    {
        devpanel::LockableFloatPropertyComponent* lockable = nullptr;
        juce::String slug;
        double startValue = 0.0;
        double endValue = 0.0;
        double preSweepValue = 0.0;
        int totalTicks = 1;
        int tick = 0;
    };
    struct TutorialStep
    {
        juce::String title;
        juce::String body;
        int rightTab = -1;
        int subTab = -1;
        int forceEngine = -1;
        int forceHq = -1; // -1 keeps current mode, otherwise 0/1.
        juce::String focusTarget;
        juce::String focusHint;
    };
    std::vector<ConsoleTargetBinding> consoleTargets;
    std::vector<ConsoleAction> consoleUndoStack;
    std::vector<ConsoleAction> consoleRedoStack;
    std::vector<ConsoleSweepState> consoleSweeps;
    std::unordered_map<std::string, size_t> consoleTargetIndexBySlug;
    std::unordered_map<std::string, juce::String> consoleListOutputCache;
    std::unordered_map<std::string, double> consoleFactoryValues;
    std::unordered_map<std::string, juce::String> consoleAliases;
    bool consoleFactoryValuesReady = false;
    juce::StringArray consoleWatchSlugs;
    juce::StringArray consoleCommandHistory;
    bool consoleBypassActive = false;
    float consoleBypassStoredMixRaw = 0.0f;
    bool consoleSoloActive = false;
    juce::String consoleSoloNode;
    float consoleSoloStoredMixRaw = 0.0f;
    bool tutorialActive = false;
    juce::String tutorialTopicKey;
    juce::String tutorialTopicTitle;
    std::vector<TutorialStep> tutorialSteps;
    int tutorialStepIndex = -1;
    juce::String tutorialFocusTarget;
    int tutorialPulseTick = 0;
    juce::File recentTouchesLogFile;
    int analyzerRefreshTickCounter = 0;
    bool lastModulationDemand = true;
    bool lastSpectrumDemand = true;
    bool lastTransferDemand = true;
    bool lastTelemetryDemand = true;
    int metadataControlCount = 0;
    int metadataVisualMappedCount = 0;
    int metadataNoVisualCount = 0;
    juce::String metadataValidationText;
    int resetFactoryButtonResetCountdownTicks = 0;

    juce::String buildJson() const;
    void setEditingLocked(bool shouldLock);
    void saveCurrentAsDefaults();
    void resetToFactoryDefaults();
    void applyValueFxPreset(int presetId);
    void updateActiveProfileLabel();
    int getSelectedSubTab() const;
    int getSubTabCount(int mainTab) const;
    juce::String getSubTabName(int mainTab, int subTab) const;
    juce::PropertyPanel* getActiveEngineLayoutPanel();
    juce::PropertyPanel* getActiveEngineInternalsPanel();
    void refreshSecondaryTabButtons();
    void updateEngineSectionVisibility();
    void updateRightTabVisibility();
    bool isPropertyVisibleInViewport(const juce::PropertyComponent* property) const;
    void updateAnalyzerDemandFromVisibility();
    int refreshVisibleLiveReadouts();
    void registerConsoleTarget(juce::PropertyComponent* property, const juce::String& name);
    devpanel::ConsoleCommandResult executeConsoleCommand(const juce::String& command);
    void appendRecentTouchLogLine(const juce::String& line) const;
    juce::String buildConsoleWatchHudText() const;
    void updateConsoleSweeps();
    void cancelConsoleSweeps();
    bool startTutorial(const juce::String& requestedTopic, juce::String& statusMessage);
    bool stopTutorial(bool skipped, juce::String& statusMessage);
    void advanceTutorialStep();
    void advanceTutorialSection();
    void applyTutorialStep();
    void layoutTutorialOverlay();
    void updateTutorialHighlight();
    void expandTutorialFocusSections();
    void revealTutorialFocusTarget();
    juce::Component* resolveTutorialFocusComponent(const juce::String& focusTarget) const;
    std::vector<TutorialStep> buildTutorialScript(const juce::String& requestedTopic,
                                                  juce::String& resolvedTopic,
                                                  juce::String& resolvedTitle) const;
    void triggerSaveButtonReset();
    void triggerResetFactoryButtonReset();
    void buildOverviewTab(DevPanelBuildContext& ctx);
    void buildModulationTab(DevPanelBuildContext& ctx);
    void buildToneTab(DevPanelBuildContext& ctx);
    void buildEngineTab(DevPanelBuildContext& ctx);
    void buildValidationTab(DevPanelBuildContext& ctx);
    void buildInternalsTab(DevPanelBuildContext& ctx);
    void buildLayoutTab(DevPanelBuildContext& ctx);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DevPanel)
};
