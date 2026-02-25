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
#include "BinaryData.h"

AnimatedToggleButton::AnimatedToggleButton()
{
    setSliderStyle(juce::Slider::LinearVertical);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    // Inverted range: top = 1 (HQ on), bottom = 0 (HQ off) so drag up = ON, drag down = OFF
    setRange(1.0, 0.0, 1.0);
    setMouseDragSensitivity(2);
    setScrollWheelEnabled(false);
    setWantsKeyboardFocus(false);
    onValueChange = [this]
    {
        const bool isOn = getValue() >= 0.5;
        startAnimationToState(isOn);
    };
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

    static const juce::Image sheet = juce::ImageCache::getFromMemory(BinaryData::switch_a_spritesheet_png, BinaryData::switch_a_spritesheet_pngSize);
    if (!sheet.isValid())
        return;

    const int frameIndex = juce::jlimit(0, kNumFrames - 1, juce::roundToInt(animatedFrame));
    const int row = frameIndex / kCols;
    const int col = frameIndex % kCols;
    const juce::Rectangle<int> src(col * kFramePx, row * kFramePx, kFramePx, kFramePx);
    if (!sheet.getBounds().contains(src))
        return;

    const juce::Image frame = sheet.getClippedImage(src);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImageWithin(frame, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred, false);
}

void AnimatedToggleButton::startAnimationToState(bool on)
{
    // Frame 0 = up = on, frame 17 = down = off
    const float target = on ? 0.0f : static_cast<float>(kNumFrames - 1);
    animationStartFrame = animatedFrame;
    animationEndFrame = target;
    animationStartMs = juce::Time::getMillisecondCounterHiRes();
    animationRunning = true;
    startTimerHz(120);
}

void AnimatedToggleButton::commitToggleState(bool newState, juce::NotificationType notificationType)
{
    setValue(newState ? 1.0 : 0.0, notificationType);
}

void AnimatedToggleButton::mouseDown(const juce::MouseEvent& e)
{
    juce::Slider::mouseDown(e); // keep APVTS SliderAttachment gesture behavior
    juce::Component::beginDragAutoRepeat(16);
    dragStartScreenY = e.getScreenPosition().y;
    dragTriggered = false;
    pointerIsDown = true;
    if (!isTimerRunning())
        startTimerHz(120);
}

void AnimatedToggleButton::mouseDrag(const juce::MouseEvent& e)
{
    if (dragTriggered)
        return;

    const float dragDeltaY = static_cast<float>(e.getScreenPosition().y - dragStartScreenY);
    // HQ on = up, HQ off = down: drag up -> ON, drag down -> OFF
    if (dragDeltaY < 0.0f && getValue() < 0.5)
    {
        dragTriggered = true;
        commitToggleState(true, juce::sendNotificationSync);
    }
    else if (dragDeltaY > 0.0f && getValue() >= 0.5)
    {
        dragTriggered = true;
        commitToggleState(false, juce::sendNotificationSync);
    }
}

void AnimatedToggleButton::mouseUp(const juce::MouseEvent& e)
{
    const float dragDeltaY = static_cast<float>(e.getScreenPosition().y - dragStartScreenY);
    // HQ on = up, HQ off = down: drag up -> ON, drag down -> OFF
    if (!dragTriggered && dragDeltaY < 0.0f && getValue() < 0.5)
    {
        dragTriggered = true;
        commitToggleState(true, juce::sendNotificationSync);
    }
    else if (!dragTriggered && dragDeltaY > 0.0f && getValue() >= 0.5)
    {
        dragTriggered = true;
        commitToggleState(false, juce::sendNotificationSync);
    }
    pointerIsDown = false;
    juce::Slider::mouseUp(e);
}

void AnimatedToggleButton::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    commitToggleState(getValue() < 0.5, juce::sendNotificationSync);
}

void AnimatedToggleButton::timerCallback()
{
    if (pointerIsDown && !dragTriggered)
    {
        const int currentScreenY = juce::Desktop::getInstance().getMousePosition().y;
        // HQ on = up, HQ off = down
        if (currentScreenY < dragStartScreenY && getValue() < 0.5)
        {
            dragTriggered = true;
            commitToggleState(true, juce::sendNotificationSync);
        }
        else if (currentScreenY > dragStartScreenY && getValue() >= 0.5)
        {
            dragTriggered = true;
            commitToggleState(false, juce::sendNotificationSync);
        }
    }

    if (animationRunning)
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const float t = juce::jlimit(0.0f, 1.0f, static_cast<float>((nowMs - animationStartMs) / animationDurationMs));
        const float eased = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::pi * t);
        const float snapped = (t > 0.88f) ? (0.88f + (t - 0.88f) * 1.8f) : t;
        const float blend = juce::jlimit(0.0f, 1.0f, 0.7f * eased + 0.3f * snapped);
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
