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

class ChoroborosPluginEditor;
class ChoroborosAudioProcessor;
struct DevPanelBuildContext;

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
    juce::ComboBox devEngineModeBox;
    juce::ToggleButton devHqModeToggle;
    juce::TextEditor engineFilterEditor;
    juce::TextButton engineFilterClearButton;
    juce::ToggleButton engineShowAdvancedToggle;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    bool editingLocked = true;
    bool suppressDevModeControlCallbacks = false;
    int saveButtonResetCountdownTicks = 0;
    int selectedRightTab = 0; // 0=Overview, 1=Modulation, 2=Tone, 3=Engine, 4=Look&Feel, 5=Validation, 6=Settings
    std::array<int, 7> selectedSubTabs { { 0, 0, 0, 0, 0, 0, 0 } };
    bool engineShowAdvanced = false;
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
