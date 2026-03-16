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

#include "TopBarDrawer.h"
#include "../Plugin/PluginProcessor.h"
#include "DevPanelSupport.h"
#include <cmath>

//==============================================================================
// Construction
//==============================================================================

TopBarDrawer::TopBarDrawer()
{
    setInterceptsMouseClicks (true, true);

    // Buttons start hidden — revealed by the expand animation
    addChildComponent (devButton);
    addChildComponent (aboutButton);
    addChildComponent (helpButton);
    addChildComponent (feedbackButton);

    // Listen for mouse events on child buttons so we can detect hover
    // even when the mouse is over a button (JUCE routes events to the
    // child, so the parent's mouseMove/mouseEnter never fire).
    devButton.addMouseListener (this, false);
    aboutButton.addMouseListener (this, false);
    helpButton.addMouseListener (this, false);
    feedbackButton.addMouseListener (this, false);

    // Button info: title + description (replaces native tooltips)
    buttonInfos_ = {{
        { &devButton,      "Developer Panel",  {} },
        { &aboutButton,    "About",            {} },
        { &helpButton,     "Help",             {} },
        { &feedbackButton, "Report Bug",       {} }
    }};
}

//==============================================================================
// Setup
//==============================================================================

void TopBarDrawer::setupIcons (const juce::Image& dev,
                               const juce::Image& about,
                               const juce::Image& help,
                               const juce::Image& feedback)
{
    // Store the raw icon images so we can re-tint them later
    iconDev_      = dev;
    iconAbout_    = about;
    iconHelp_     = help;
    iconFeedback_ = feedback;

    applyIconTint (accentColour_);
}

void TopBarDrawer::setAccentColour (juce::Colour newAccent)
{
    if (accentColour_ == newAccent)
        return;

    accentColour_ = newAccent;
    applyIconTint (accentColour_);
    repaint();
}

void TopBarDrawer::applyIconTint (juce::Colour accent)
{
    // false for first param = don't resize button to fit image.
    // This prevents setImages from blowing up button bounds when
    // the engine colour changes while the drawer is open.
    auto setup = [accent] (juce::ImageButton& btn, const juce::Image& icon)
    {
        btn.setImages (false, true, true,
            icon, 0.55f, accent,   // normal: subtle tint
            icon, 1.0f,  accent,   // hover: full tint
            icon, 0.4f,  accent);  // pressed: dim tint

        btn.setTooltip ({});
    };

    setup (devButton,      iconDev_);
    setup (aboutButton,    iconAbout_);
    setup (helpButton,     iconHelp_);
    setup (feedbackButton, iconFeedback_);

    // Re-apply correct bounds (scaled size) in case the drawer is
    // mid-animation or fully expanded when the colour changes.
    if (btnSize_ > 0)
        updateButtonStates();
}

void TopBarDrawer::setupLayout (float uiScale)
{
    auto s = [uiScale] (int v)
    {
        return juce::roundToInt (static_cast<float> (v) * uiScale);
    };

    btnSize_  = s (18);
    btnGap_   = s (5);
    padH_     = s (5);
    padV_     = s (4);
    arrowW_   = s (14);
    arrowGap_ = s (4);

    // Expanded: [padH][arrow][arrowGap][btn][gap][btn][gap][btn][gap][btn][padH]
    expandedW_  = padH_ + arrowW_ + arrowGap_
                + btnSize_ * 4 + btnGap_ * 3
                + padH_;

    // Collapsed: just the arrow area
    collapsedW_ = padH_ + arrowW_ + padH_;

    drawerH_ = padV_ * 2 + btnSize_;

    // Tooltip area: title row only
    tooltipH_ = s (18);

    // Final button positions (relative to component left edge)
    btnY_ = padV_;
    const int x0 = padH_ + arrowW_ + arrowGap_;

    devFinalX_      = x0;
    aboutFinalX_    = x0 + btnSize_ + btnGap_;
    helpFinalX_     = x0 + (btnSize_ + btnGap_) * 2;
    feedbackFinalX_ = x0 + (btnSize_ + btnGap_) * 3;

    devButton.setBounds      (devFinalX_,      btnY_, btnSize_, btnSize_);
    aboutButton.setBounds    (aboutFinalX_,    btnY_, btnSize_, btnSize_);
    helpButton.setBounds     (helpFinalX_,     btnY_, btnSize_, btnSize_);
    feedbackButton.setBounds (feedbackFinalX_, btnY_, btnSize_, btnSize_);
}

int TopBarDrawer::getTotalHeight() const
{
    return drawerH_ + juce::roundToInt (static_cast<float> (tooltipH_) * tooltipProgress_);
}

//==============================================================================
// Painting
//==============================================================================

void TopBarDrawer::paint (juce::Graphics& g)
{
    const float progress = slideProgress();
    const float visW = static_cast<float> (collapsedW_)
                     + static_cast<float> (expandedW_ - collapsedW_) * progress;
    const float visX = static_cast<float> (getWidth()) - visW;
    const float totalH = static_cast<float> (getTotalHeight());
    constexpr float radius = 4.0f;

    // Vertical gradient matching engine-selector combo box style
    const auto bgTop = devpanel::hackerBgElevated().brighter (0.15f);
    const auto bgBot = devpanel::hackerBg().darker (0.32f)
                           .interpolatedWith (devpanel::hackerText(), 0.06f);

    juce::ColourGradient grad (bgTop, visX, 0.0f, bgBot, visX, totalH, false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (visX, 0.0f, visW, totalH, radius);

    // Accent border — uses current engine accent, not hardcoded hacker green.
    g.setColour (accentColour_.withAlpha (0.55f));
    g.drawRoundedRectangle (visX + 0.5f, 0.5f, visW - 1.0f, totalH - 1.0f,
                            radius, 1.05f);

    // Subtle inner glow
    g.setColour (accentColour_.withAlpha (0.04f));
    g.drawRoundedRectangle (visX + 1.5f, 1.5f, visW - 3.0f, totalH - 3.0f,
                            radius - 1.0f, 0.6f);

    paintArrow (g, visX);

    // Draw tooltip text if expanded
    if (tooltipProgress_ > 0.01f && hoveredIndex_ >= 0)
        paintTooltipArea (g, visX, visW);
}

void TopBarDrawer::paintTooltipArea (juce::Graphics& g, float visX, float visW)
{
    const auto& info = buttonInfos_[static_cast<size_t> (hoveredIndex_)];
    const float baseY  = static_cast<float> (drawerH_);
    const float alpha  = tooltipProgress_;
    const float textX  = visX + static_cast<float> (padH_) + 2.0f;
    const float textW  = visW - static_cast<float> (padH_) * 2.0f - 4.0f;

    // Separator line between icons and tooltip
    g.setColour (accentColour_.withAlpha (0.4f * alpha));
    g.drawHorizontalLine (juce::roundToInt (baseY), visX + 4.0f, visX + visW - 4.0f);

    // Title: engine accent colour, bold, vertically centred
    auto titleFont = devpanel::makeLabelFont (11.0f, true);
    g.setFont (titleFont);
    g.setColour (accentColour_.withAlpha (0.95f * alpha));
    g.drawText (info.title,
                juce::Rectangle<float> (textX, baseY + 2.0f,
                                        textW, static_cast<float> (tooltipH_) - 4.0f),
                juce::Justification::centredLeft, true);
}

void TopBarDrawer::paintArrow (juce::Graphics& g, float containerLeft)
{
    const float cx = containerLeft
                   + static_cast<float> (padH_)
                   + static_cast<float> (arrowW_) * 0.5f;
    const float cy = static_cast<float> (drawerH_) * 0.5f;
    const float sz = static_cast<float> (arrowW_) * 0.28f;

    // dir: -1 = left-pointing (collapsed), 0 = line, +1 = right-pointing
    const float dir    = rawProgress_ * 2.0f - 1.0f;
    const float absDir = std::abs (dir);

    juce::Path path;

    if (absDir < 0.12f)
    {
        // Near midpoint - draw a horizontal line
        path.startNewSubPath (cx - sz, cy);
        path.lineTo (cx + sz, cy);
    }
    else
    {
        const float tipX   = cx + sz * dir;
        const float armX   = cx - sz * dir;
        const float spread = sz * juce::jmin (1.0f, absDir * 1.8f);

        path.startNewSubPath (armX, cy - spread);
        path.lineTo (tipX, cy);
        path.lineTo (armX, cy + spread);
    }

    g.setColour (accentColour_.withAlpha (0.90f));
    g.strokePath (path,
        juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
}

//==============================================================================
// Hit testing - only the visible region responds to clicks/hovers
//==============================================================================

bool TopBarDrawer::hitTest (int x, int y)
{
    const float progress = slideProgress();
    const float visW = static_cast<float> (collapsedW_)
                     + static_cast<float> (expandedW_ - collapsedW_) * progress;
    const float visX = static_cast<float> (getWidth()) - visW;
    const float totalH = static_cast<float> (getTotalHeight());

    return static_cast<float> (x) >= visX
        && static_cast<float> (y) < totalH;
}

void TopBarDrawer::mouseDown (const juce::MouseEvent& e)
{
    // Only clicks in the arrow region toggle the drawer
    const float progress = slideProgress();
    const float visW = static_cast<float> (collapsedW_)
                     + static_cast<float> (expandedW_ - collapsedW_) * progress;
    const float visX = static_cast<float> (getWidth()) - visW;
    const float arrowRight = visX
                           + static_cast<float> (padH_ + arrowW_ + arrowGap_);

    if (e.position.x >= visX && e.position.x <= arrowRight)
        toggleDrawer();
}

void TopBarDrawer::mouseMove (const juce::MouseEvent&)
{
    // Continuously check hover even when the timer is idle
    if (expanded_)
        checkButtonHover();
}

void TopBarDrawer::mouseEnter (const juce::MouseEvent&)
{
    // Re-entering the drawer — check hover immediately
    if (expanded_)
        checkButtonHover();
}

void TopBarDrawer::mouseExit (const juce::MouseEvent& e)
{
    // This fires both when the mouse leaves the drawer AND when it
    // exits a child button (via addMouseListener).  Only collapse the
    // tooltip if the mouse has genuinely left the entire drawer area.
    const auto pos = e.getEventRelativeTo (this).position.toInt();
    if (hitTest (pos.x, pos.y))
        return;   // still inside the visible drawer region

    if (hoveredIndex_ >= 0)
    {
        hoveredIndex_ = -1;
        tooltipExpanding_ = false;

        if (! isTimerRunning())
            startTimerHz (kAnimFps);
    }
}

//==============================================================================
// Animation
//==============================================================================

void TopBarDrawer::timerCallback()
{
    bool needsRepaint = false;

    // --- Horizontal slide animation ---
    {
        const float step = 1.0f
                         / (static_cast<float> (kAnimFps) * kAnimDurationSec);

        const float prev = rawProgress_;

        if (expanding_)
            rawProgress_ = juce::jmin (1.0f, rawProgress_ + step);
        else
            rawProgress_ = juce::jmax (0.0f, rawProgress_ - step);

        if (rawProgress_ != prev)
        {
            needsRepaint = true;
            updateButtonStates();
        }

        const bool slideDone = (expanding_  && rawProgress_ >= 1.0f)
                            || (!expanding_ && rawProgress_ <= 0.0f);
        if (slideDone && !expanding_)
            expanded_ = false;

        if (slideDone)
            updateButtonStates();
    }

    // --- Vertical tooltip expansion animation ---
    {
        const float step = 1.0f
                         / (static_cast<float> (kAnimFps) * kTooltipDurationSec);

        const float prev = tooltipProgress_;

        if (tooltipExpanding_)
            tooltipProgress_ = juce::jmin (1.0f, tooltipProgress_ + step);
        else
            tooltipProgress_ = juce::jmax (0.0f, tooltipProgress_ - step);

        if (tooltipProgress_ != prev)
        {
            needsRepaint = true;

            // Resize this component to accommodate the tooltip area
            const int newH = getTotalHeight();
            if (getHeight() != newH)
                setSize (getWidth(), newH);
        }
    }

    if (needsRepaint)
        repaint();

    // Check if we should poll hover state (buttons don't propagate
    // mouseEnter to parent, so we poll while the timer is running)
    if (expanded_)
        checkButtonHover();

    // Stop timer when all animations are done
    const bool slideDone   = (expanding_  && rawProgress_ >= 1.0f)
                          || (!expanding_ && rawProgress_ <= 0.0f);
    const bool tipDone     = (tooltipExpanding_  && tooltipProgress_ >= 1.0f)
                          || (!tooltipExpanding_ && tooltipProgress_ <= 0.0f);

    if (slideDone && tipDone)
        stopTimer();
}

void TopBarDrawer::checkButtonHover()
{
    // Check which button (if any) the mouse is over
    const auto mousePos = getMouseXYRelative();
    int newHovered = -1;

    if (expanded_ && ! isMouseButtonDown())
    {
        for (int i = 0; i < 4; ++i)
        {
            auto* btn = buttonInfos_[static_cast<size_t> (i)].button;
            if (btn->isVisible() && btn->getBounds().contains (mousePos))
            {
                newHovered = i;
                break;
            }
        }
    }

    if (newHovered != hoveredIndex_)
    {
        hoveredIndex_ = newHovered;
        tooltipExpanding_ = (newHovered >= 0);

        if (! isTimerRunning())
            startTimerHz (kAnimFps);

        repaint();
    }
}

void TopBarDrawer::updateButtonStates()
{
    float offsetFrac, fade;

    if (expanding_)
    {
        // Bounce completes at 65 % - ends before the slide finishes
        const float bounceT = juce::jmin (1.0f, rawProgress_ / 0.65f);
        offsetFrac = easeOutBack (bounceT);

        // Fade starts at 8 %, done by 50 %
        fade = juce::jlimit (0.0f, 1.0f, (rawProgress_ - 0.08f) / 0.42f);
    }
    else
    {
        // Collapse: smooth ease-in (no bounce), faster fade-out
        offsetFrac = rawProgress_ * rawProgress_;
        fade = juce::jlimit (0.0f, 1.0f, (rawProgress_ - 0.05f) / 0.45f);
    }

    const float maxSlide = static_cast<float> (expandedW_ - collapsedW_);
    const int   xOff     = juce::roundToInt (maxSlide * (1.0f - offsetFrac));
    const bool  show     = fade > 0.01f;
    const bool  interact = expanded_ && ! (expanding_ ? isTimerRunning() && rawProgress_ < 1.0f : true);

    // Scale icons from 25 % to 100 % of btnSize_ as the drawer opens
    const float scale = 0.25f + 0.75f * fade;
    const int   sz    = juce::roundToInt (static_cast<float> (btnSize_) * scale);

    auto place = [&] (juce::ImageButton& btn, int finalX)
    {
        btn.setVisible (show);
        btn.setAlpha (fade);

        // Centre the scaled button on its final midpoint
        const int cx = finalX + xOff + btnSize_ / 2;
        const int cy = btnY_ + btnSize_ / 2;
        btn.setBounds (cx - sz / 2, cy - sz / 2, sz, sz);

        btn.setInterceptsMouseClicks (interact, false);
    };

    place (devButton,      devFinalX_);
    place (aboutButton,    aboutFinalX_);
    place (helpButton,     helpFinalX_);
    place (feedbackButton, feedbackFinalX_);
}

void TopBarDrawer::toggleDrawer()
{
    // Supports mid-animation reversal
    expanding_ = ! expanding_;
    expanded_  = true;            // keep alive during collapse anim

    // Collapse tooltip when closing the drawer
    if (! expanding_)
    {
        hoveredIndex_ = -1;
        tooltipExpanding_ = false;
    }

    startTimerHz (kAnimFps);
}

//==============================================================================
// Easing curves
//==============================================================================

float TopBarDrawer::slideProgress() const
{
    if (rawProgress_ <= 0.0f) return 0.0f;
    if (rawProgress_ >= 1.0f) return 1.0f;

    // Ease-out cubic
    const float inv = 1.0f - rawProgress_;
    return 1.0f - inv * inv * inv;
}

float TopBarDrawer::easeOutBack (float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    // Subtle back-ease with ~3.7 % overshoot (micro-bounce)
    constexpr float s = 1.0f;
    t -= 1.0f;
    return 1.0f + (s + 1.0f) * t * t * t + s * t * t;
}
