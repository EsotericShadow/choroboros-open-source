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

#include "AnimatedToggleButton.h"
#include "../Assets/AssetRepository.h"

AnimatedToggleButton::AnimatedToggleButton()
{
    setSliderStyle(juce::Slider::LinearVertical);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    // Inverted range: top = 1 (HQ on), bottom = 0 (HQ off) so drag up = ON, drag down = OFF
    setRange(1.0, 0.0, 1.0);
    // Very high sensitivity = effectively disables Slider's built-in drag behavior.
    // All value changes go through commitToggleState() in our custom mouse handlers.
    setMouseDragSensitivity(500);
    setScrollWheelEnabled(false);
    setWantsKeyboardFocus(false);
    setOpaque(false);
    setRepaintsOnMouseActivity(false);
    spritesheetImage = choroboros::assets::AssetRepository::instance()
        .loadImage(choroboros::assets::ids::switchSpriteSheet, true)
        .image;
    onValueChange = [this]
    {
        const bool isOn = getValue() >= 0.5;
        startAnimationToState(isOn);
    };
}

AnimatedToggleButton::~AnimatedToggleButton()
{
    stopTimer();
}

void AnimatedToggleButton::invalidateScaledFrameCache()
{
    cachedFrameWidth = 0;
    cachedFrameHeight = 0;
    scaledFrames.fill({});
}

void AnimatedToggleButton::resized()
{
    invalidateScaledFrameCache();
}

float AnimatedToggleButton::getAnimationProgress() const
{
    // Frame 0 = switch UP = HQ on = light on. Frame 17 = switch DOWN = HQ off = light off.
    const float frame = animationRunning ? animatedFrame : ((getValue() >= 0.5f) ? 0.0f : static_cast<float>(kNumFrames - 1));
    return juce::jlimit(0.0f, 1.0f, 1.0f - frame / static_cast<float>(kNumFrames - 1));
}

void AnimatedToggleButton::paint(juce::Graphics& g)
{
    // Frame 0 = switch UP = HQ on. Frame 17 = switch DOWN = HQ off.
    const float targetFrame = (getValue() >= 0.5) ? 0.0f : static_cast<float>(kNumFrames - 1);
    if (!isTimerRunning() && std::abs(animatedFrame - targetFrame) > 0.01f)
        animatedFrame = targetFrame;

    rebuildScaledFramesIfNeeded();

    const auto& sheet = spritesheetImage;
    if (!sheet.isValid())
        return;

    const float clampedFrame = juce::jlimit(0.0f, static_cast<float>(kNumFrames - 1), animatedFrame);
    const int baseIndex = juce::jlimit(0, kNumFrames - 1, static_cast<int>(std::floor(clampedFrame)));
    const int nextIndex = juce::jlimit(0, kNumFrames - 1, baseIndex + 1);
    const float nextAlpha = (baseIndex == nextIndex) ? 0.0f
                                                     : juce::jlimit(0.0f, 1.0f, clampedFrame - static_cast<float>(baseIndex));

    const auto drawCachedFrame = [&](int frameIndex, float alpha)
    {
        if (alpha <= 0.001f)
            return false;

        const auto& cachedFrame = scaledFrames[static_cast<size_t>(frameIndex)];
        if (!cachedFrame.isValid())
            return false;

        g.saveState();
        g.setOpacity(alpha);
        g.drawImageAt(cachedFrame, 0, 0);
        g.restoreState();
        return true;
    };

    bool drew = drawCachedFrame(baseIndex, 1.0f - nextAlpha);
    if (nextIndex != baseIndex)
        drew = drawCachedFrame(nextIndex, nextAlpha) || drew;

    if (drew)
        return;

    const auto drawFrameFromSheet = [&](int frameIndex, float alpha)
    {
        if (alpha <= 0.001f)
            return false;

        const int row = frameIndex / kCols;
        const int col = frameIndex % kCols;
        const juce::Rectangle<int> src(col * kFramePx, row * kFramePx, kFramePx, kFramePx);
        if (!sheet.getBounds().contains(src))
            return false;

        const juce::Image frame = sheet.getClippedImage(src);
        if (!frame.isValid())
            return false;

        g.saveState();
        g.setOpacity(alpha);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageWithin(frame, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred, false);
        g.restoreState();
        return true;
    };

    drew = drawFrameFromSheet(baseIndex, 1.0f - nextAlpha);
    if (nextIndex != baseIndex)
        drawFrameFromSheet(nextIndex, nextAlpha);
}

void AnimatedToggleButton::rebuildScaledFramesIfNeeded()
{
    const int width = getWidth();
    const int height = getHeight();

    if (width <= 0 || height <= 0 || !spritesheetImage.isValid())
        return;

    if (cachedFrameWidth == width && cachedFrameHeight == height
        && scaledFrames[0].isValid() && scaledFrames[static_cast<size_t>(kNumFrames - 1)].isValid())
        return;

    cachedFrameWidth = width;
    cachedFrameHeight = height;
    scaledFrames.fill({});

    for (int frameIndex = 0; frameIndex < kNumFrames; ++frameIndex)
    {
        const int row = frameIndex / kCols;
        const int col = frameIndex % kCols;
        const juce::Rectangle<int> src(col * kFramePx, row * kFramePx, kFramePx, kFramePx);

        if (!spritesheetImage.getBounds().contains(src))
            continue;

        juce::Image scaledFrame(juce::Image::ARGB, width, height, true);
        juce::Graphics frameGraphics(scaledFrame);
        frameGraphics.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        frameGraphics.drawImage(spritesheetImage,
                                0, 0, width, height,
                                src.getX(), src.getY(), src.getWidth(), src.getHeight());
        scaledFrames[static_cast<size_t>(frameIndex)] = scaledFrame;
    }
}

void AnimatedToggleButton::startAnimationToState(bool on)
{
    // Frame 0 = up = on, frame 17 = down = off
    const float target = on ? 0.0f : static_cast<float>(kNumFrames - 1);

    if (animationRunning && std::abs(animationEndFrame - target) <= 0.01f)
        return;

    if (! animationRunning && std::abs(animatedFrame - target) <= 0.01f)
    {
        animatedFrame = target;
        return;
    }

    animationStartFrame = animatedFrame;
    animationEndFrame = target;
    animationStartMs = juce::Time::getMillisecondCounterHiRes();
    animationDurationMs = on ? kAnimationDurationOnMs : kAnimationDurationOffMs;
    animationRunning = true;
    startTimerHz(kAnimationTimerHz);
}

void AnimatedToggleButton::commitToggleState(bool newState, juce::NotificationType notificationType)
{
    setValue(newState ? 1.0 : 0.0, notificationType);
}

void AnimatedToggleButton::mouseDown(const juce::MouseEvent& e)
{
    // Do NOT forward to Slider::mouseDown — it snaps value based on click Y
    // position, which prevents single-click toggle from HQ→NQ when clicking
    // near the top (Slider resolves to value 1.0 = no change, eating the event).
    // APVTS sync happens through setValue() in commitToggleState().
    juce::ignoreUnused(e);
    juce::Component::beginDragAutoRepeat(16);
    dragStartScreenY = e.getScreenPosition().y;
    dragAnchorScreenY = dragStartScreenY;
    pointerIsDown = true;
    dragToggled = false;
    if (!isTimerRunning())
        startTimerHz(kAnimationTimerHz);
}

void AnimatedToggleButton::mouseDrag(const juce::MouseEvent& e)
{
    tryCommitDragAtScreenY(e.getScreenPosition().y);
}

void AnimatedToggleButton::mouseUp(const juce::MouseEvent& e)
{
    tryCommitDragAtScreenY(e.getScreenPosition().y);

    // Click-to-toggle: if the pointer barely moved and no drag toggle occurred,
    // treat this as a simple click and toggle the state.
    if (!dragToggled)
    {
        const int totalMove = std::abs(e.getScreenPosition().y - dragStartScreenY);
        if (totalMove < kClickMaxMovePx)
            commitToggleState(getValue() < 0.5, juce::sendNotificationSync);
    }

    pointerIsDown = false;
    dragToggled = false;
    // Do NOT forward to Slider::mouseUp — see mouseDown comment.
}

void AnimatedToggleButton::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    commitToggleState(getValue() < 0.5, juce::sendNotificationSync);
}

void AnimatedToggleButton::timerCallback()
{
    if (pointerIsDown)
    {
        const int currentScreenY = juce::Desktop::getInstance().getMousePosition().y;
        tryCommitDragAtScreenY(currentScreenY);
    }

    if (animationRunning)
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const float t = juce::jlimit(0.0f, 1.0f, static_cast<float>((nowMs - animationStartMs) / animationDurationMs));
        const float blend = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
        animatedFrame = animationStartFrame + (animationEndFrame - animationStartFrame) * blend;
        if (t >= 1.0f)
        {
            animatedFrame = animationEndFrame;
            animationRunning = false;
        }
        repaint();
        if (onAnimationTick)
            onAnimationTick();
    }

    if (!pointerIsDown && !animationRunning)
        stopTimer();
}

void AnimatedToggleButton::tryCommitDragAtScreenY(int screenY)
{
    const int deltaY = screenY - dragAnchorScreenY;

    // HQ on = up, HQ off = down. Anchor is reset after each successful flip so
    // a single press-drag can keep flipping as the pointer direction alternates.
    if (deltaY <= -kDragToggleThresholdPx && getValue() < 0.5)
    {
        dragAnchorScreenY = screenY;
        dragToggled = true;
        commitToggleState(true, juce::sendNotificationSync);
        return;
    }

    if (deltaY >= kDragToggleThresholdPx && getValue() >= 0.5)
    {
        dragAnchorScreenY = screenY;
        dragToggled = true;
        commitToggleState(false, juce::sendNotificationSync);
        return;
    }

    if (std::abs(deltaY) >= kDragToggleThresholdPx * 3)
        dragAnchorScreenY = screenY;
}
