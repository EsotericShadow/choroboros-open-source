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
}

//==============================================================================
// Setup
//==============================================================================

void TopBarDrawer::setupIcons (const juce::Image& dev,
                               const juce::Image& about,
                               const juce::Image& help,
                               const juce::Image& feedback)
{
    auto setup = [] (juce::ImageButton& btn, const juce::Image& icon)
    {
        btn.setImages (true, true, true,
            icon, 0.55f, {},    // normal: subtle
            icon, 1.0f,  {},    // hover: full
            icon, 0.4f,  {});   // pressed: dim
    };

    setup (devButton,      dev);
    setup (aboutButton,    about);
    setup (helpButton,     help);
    setup (feedbackButton, feedback);
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

//==============================================================================
// Painting
//==============================================================================

void TopBarDrawer::paint (juce::Graphics& g)
{
    const float progress = slideProgress();
    const float visW = static_cast<float> (collapsedW_)
                     + static_cast<float> (expandedW_ - collapsedW_) * progress;
    const float visX = static_cast<float> (getWidth()) - visW;

    // Dark container background
    g.setColour (juce::Colour (0x99000000));   // 60 % black
    g.fillRoundedRectangle (visX, 0.0f, visW,
                            static_cast<float> (getHeight()), 3.0f);

    paintArrow (g, visX);
}

void TopBarDrawer::paintArrow (juce::Graphics& g, float containerLeft)
{
    const float cx = containerLeft
                   + static_cast<float> (padH_)
                   + static_cast<float> (arrowW_) * 0.5f;
    const float cy = static_cast<float> (getHeight()) * 0.5f;
    const float sz = static_cast<float> (arrowW_) * 0.28f;

    // dir: –1 = left-pointing (collapsed), 0 = line, +1 = right-pointing
    const float dir    = rawProgress_ * 2.0f - 1.0f;
    const float absDir = std::abs (dir);

    juce::Path path;

    if (absDir < 0.12f)
    {
        // Near midpoint — draw a horizontal line
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

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.strokePath (path,
        juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
}

//==============================================================================
// Hit testing — only the visible region responds to clicks
//==============================================================================

bool TopBarDrawer::hitTest (int x, int /*y*/)
{
    const float progress = slideProgress();
    const float visW = static_cast<float> (collapsedW_)
                     + static_cast<float> (expandedW_ - collapsedW_) * progress;
    const float visX = static_cast<float> (getWidth()) - visW;
    return static_cast<float> (x) >= visX;
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

//==============================================================================
// Animation
//==============================================================================

void TopBarDrawer::timerCallback()
{
    const float step = 1.0f
                     / (static_cast<float> (kAnimFps) * kAnimDurationSec);

    if (expanding_)
        rawProgress_ = juce::jmin (1.0f, rawProgress_ + step);
    else
        rawProgress_ = juce::jmax (0.0f, rawProgress_ - step);

    updateButtonStates();
    repaint();

    const bool done = (expanding_  && rawProgress_ >= 1.0f)
                   || (!expanding_ && rawProgress_ <= 0.0f);
    if (done)
    {
        stopTimer();
        if (! expanding_)
            expanded_ = false;
    }
}

void TopBarDrawer::updateButtonStates()
{
    float offsetFrac, fade;

    if (expanding_)
    {
        // Bounce completes at 65 % — ends before the slide finishes
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
    const bool  interact = expanded_ && ! isTimerRunning();

    auto place = [&] (juce::ImageButton& btn, int finalX)
    {
        btn.setVisible (show);
        btn.setAlpha (fade);
        btn.setTopLeftPosition (finalX + xOff, btnY_);
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
