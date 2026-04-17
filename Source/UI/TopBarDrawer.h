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

//==============================================================================
/**
    Animated sliding drawer that groups the top-bar icon buttons.

    Default state is collapsed — only a small tab with a left-pointing
    chevron is visible in the top-right corner.  Clicking the chevron
    expands the drawer to the left, revealing all four buttons with a
    smooth slide, micro-bounce, and fade-in.  The chevron morphs through
    a horizontal line to a right-pointing arrow, which collapses the
    drawer on click.

    Hovering a button expands the drawer downward to show the button's
    title (green) and description (white italic) in two extra rows.
    Leaving the drawer collapses the tooltip area back up.
*/
class TopBarDrawer : public juce::Component,
                     private juce::Timer
{
public:
    // Public so the editor can wire up onClick callbacks
    juce::ImageButton devButton      { "dev" };
    juce::ImageButton aboutButton    { "about" };
    juce::ImageButton helpButton     { "help" };
    juce::ImageButton feedbackButton { "feedback" };

    TopBarDrawer();
    ~TopBarDrawer() override;

    /** Load icon images into the four buttons. */
    void setupIcons (const juce::Image& dev,
                     const juce::Image& about,
                     const juce::Image& help,
                     const juce::Image& feedback);

    /** Compute all internal layout values from the global UI scale. */
    void setupLayout (float uiScale);
    void setUiScale (float uiScale);

    /** Update the accent colour used for icon tinting, arrow, and tooltip title.
        Call this whenever the engine colour changes. */
    void setAccentColour (juce::Colour newAccent);

    int  getExpandedWidth()  const { return expandedW_; }
    int  getDrawerHeight()   const { return drawerH_; }
    bool isDrawerExpanded()  const { return expanded_; }

    /** Total height including any tooltip expansion. */
    int  getTotalHeight()    const;

private:
    //==========================================================================
    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void timerCallback() override;

    //==========================================================================
    void  toggleDrawer();
    void  updateButtonStates();
    void  paintArrow (juce::Graphics& g, float containerLeft);
    void  paintTooltipArea (juce::Graphics& g, float visX, float visW);
    float slideProgress() const;
    static float easeOutBack (float t);

    //==========================================================================
    // Hover tooltip expansion
    struct ButtonInfo
    {
        juce::ImageButton* button;
        juce::String       title;
        juce::String       description;
    };

    std::array<ButtonInfo, 4> buttonInfos_;
    int    hoveredIndex_     = -1;     // which button is hovered (-1 = none)
    float  tooltipProgress_  = 0.0f;  // 0 = collapsed, 1 = fully expanded
    bool   tooltipExpanding_ = false;
    int    tooltipH_         = 0;     // pixel height of the tooltip area

    void checkButtonHover();

    //==========================================================================
    bool  expanded_   = false;
    bool  expanding_  = false;
    float rawProgress_ = 0.0f;

    int btnSize_ = 0, btnGap_ = 0, padH_ = 0, padV_ = 0;
    int arrowW_ = 0, arrowGap_ = 0;
    int expandedW_ = 0, collapsedW_ = 0, drawerH_ = 0;
    int devFinalX_ = 0, aboutFinalX_ = 0;
    int helpFinalX_ = 0, feedbackFinalX_ = 0;
    int btnY_ = 0;

    //==========================================================================
    // Accent colour + stored icon images for re-tinting
    juce::Colour accentColour_ { 0xff80ef80 };  // default: hacker green
    juce::Image  iconDev_, iconAbout_, iconHelp_, iconFeedback_;
    void applyIconTint (juce::Colour accent);

    static constexpr int   kAnimFps         = 60;
    static constexpr float kAnimDurationSec = 0.35f;
    static constexpr float kTooltipDurationSec = 0.18f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBarDrawer)
};
