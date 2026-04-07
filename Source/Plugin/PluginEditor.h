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

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"
#include "../UI/CustomLookAndFeel.h"
#include "../UI/LabelWithContainer.h"
#include "../UI/SmoothedSlider.h"
#include "../UI/AnimatedToggleButton.h"
#include "../UI/PluginEditorSetup.h"
#include "../UI/TopBarDrawer.h"
#include "../UI/TopHeaderBar.h"
#include "../UI/RateSyncOverlay.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

/** Background images for one engine theme (editor decode / prewarm). */
struct BackgroundAssetPack
{
    juce::Image off;
    juce::Image lit;
};

//==============================================================================
/**
*/
class ChoroborosPluginEditor  : public juce::AudioProcessorEditor,
                                private juce::AudioProcessorValueTreeState::Listener,
                                public juce::FileDragAndDropTarget
{
public:
    ChoroborosPluginEditor (ChoroborosAudioProcessor&);
    ~ChoroborosPluginEditor() override;
    static constexpr float kUiScale = 0.91f;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void applyLayout();
    void applyTuningToUI();
    void refreshValueLabels();
    void resetLayoutToFactoryDefaults();
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    float getUiScale() const { return kUiScale; }
    int getHeaderBarHeight() const { return topHeaderBar_ ? topHeaderBar_->getBarHeight() : 0; }
    const LayoutTuning& getLayoutTuning() const { return layoutTuning; }
    LayoutTuning& getLayoutTuning() { return layoutTuning; }
    juce::Font makeValueLabelFont(float heightPx, bool bold = true) const;
    juce::Font makeUiTextFont(float heightPx, bool bold = true) const;
    void setTooltipsEnabled(bool enabled);

private:
    class HQLitOverlay;

    ChoroborosAudioProcessor& audioProcessor;
    LayoutTuning layoutTuning;
    
    CustomLookAndFeel customLookAndFeel;
    juce::Image backgroundImage;
    juce::Image backgroundImageLit;  // Light-on overlay (opacity synced to HQ switch) for all themes
    std::unique_ptr<HQLitOverlay> hqLitOverlay_;
    
    // Sliders (with visual smoothing for natural feel)
    SmoothedSlider rateSlider;
    SmoothedSlider depthSlider;  // Will use 50ms to match audio smoothing
    SmoothedSlider offsetSlider;
    SmoothedSlider widthSlider;
    SmoothedSlider colorSlider;
    SmoothedSlider mixSlider;
    
    // Labels (with containers)
    LabelWithContainer rateLabel;
    LabelWithContainer depthLabel;
    LabelWithContainer offsetLabel;
    LabelWithContainer widthLabel;
    LabelWithContainer colorLabel;
    LabelWithContainer mixLabel;
    
    // Value labels (with containers)
    LabelWithContainer rateValueLabel;
    LabelWithContainer depthValueLabel;
    LabelWithContainer offsetValueLabel;
    LabelWithContainer widthValueLabel;
    LabelWithContainer colorValueLabel;
    LabelWithContainer mixValueLabel;
    
    // Engine Color selector
    juce::ComboBox engineColorBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineColorAttachment;
    
    // HQ toggle button (with animation)
    AnimatedToggleButton hqButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hqAttachment;
    LabelWithContainer hqLabel;
    
    // Branded top header bar with logo + preset browser
    std::unique_ptr<TopHeaderBar> topHeaderBar_;

    // Top-bar sliding icon-button drawer
    TopBarDrawer topBarDrawer;
    std::unique_ptr<juce::DocumentWindow> devWindow;
    bool devPanelPrewarmScheduled = false;
    bool devPanelPrewarmComplete = false;
    
    // Tooltip window
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    juce::Typeface::Ptr valueLabelTypeface;
    juce::Typeface::Ptr uiTextTypeface;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> offsetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    void loadBackgroundImage(int colorIndex = 0);
    void loadValueLabelTypeface();
    void loadUiTextTypeface();
    void updateValueLabelColors(int colorIndex);
    void repaintHQLitOverlay();
    void invalidateHQLitOverlayCache();
    void setupSlider(juce::Slider& slider, LabelWithContainer& label, LabelWithContainer& valueLabel,
                     const juce::String& name, const juce::String& paramId);
    void updateValueLabel(LabelWithContainer& label, float value, const juce::String& paramId);
    void setupValueLabelEditing(LabelWithContainer& label, juce::Slider& slider, const juce::String& paramId);
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    float parseValueFromText(const juce::String& text, const juce::String& paramId);
    
    // Helper to calculate label width with 8px padding on each side
    int calculateLabelWidth(const juce::String& text, const juce::Font& font) const;
    
    // Constructor helper methods
    void setupEngineColorSelector();
    void rebuildEngineSelectorItems();
    void setupSliderAttachments();
    void setupSliderValueChangeListeners();
    void applyEngineVisual(const choroboros::CustomEngineVisual& visual);
    void applyCurrentEngineVisual();
    void startDeferredThemePrewarm(int activeColorIndex);
    void stopDeferredThemePrewarm();
    void scheduleDeferredDevPanelPrewarm();
    void ensureDevPanelWindowCreated(bool triggeredByUser);
    void forceSoftwareRenderingForPeer();
    static void forceSoftwareRenderingForWindow(juce::DocumentWindow* window);
    double getHostBpm() const;
    void showRateSyncMenu(juce::Slider& rateControl);
    std::unique_ptr<RateSyncOverlay> rateSyncOverlay_;
    
    // parseValueFromText helper methods
    float parseRateValue(const juce::String& trimmed);
    float parseDepthValue(const juce::String& trimmed);
    float parseOffsetValue(const juce::String& trimmed);
    float parseWidthValue(const juce::String& trimmed);
    float parseColorValue(const juce::String& trimmed);
    float parseMixValue(const juce::String& trimmed);

    std::thread themePrewarmThread;
    std::shared_ptr<std::atomic<bool>> themePrewarmStopFlag = std::make_shared<std::atomic<bool>>(false);

    // Prewarm results queue: worker pushes decoded packs, message-thread paint installs them.
    struct PrewarmedTheme
    {
        int colorIndex = 0;
        int activeColorIndex = 0;
        CustomLookAndFeel::ThemeAssetPack pack;
        BackgroundAssetPack backgroundPack;
    };

    std::mutex prewarmQueueMutex;
    std::vector<PrewarmedTheme> prewarmQueue;
    std::future<CustomLookAndFeel::ThemeAssetPack> activeThemeDecodeFuture;
    int activeThemeDecodeColorIndex = 0;
    bool activeThemeInstalled = false;
    double editorCtorStartMs = 0.0;
    bool firstPaintTimingLogged = false;
    bool themePrewarmStarted = false;
    bool engineSwitchInProgress = false;
    
    // Make members accessible to setup helper
    friend class PluginEditorSetup;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoroborosPluginEditor)
};
