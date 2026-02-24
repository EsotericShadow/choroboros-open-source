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

#include "LabelWithContainer.h"
#include <cmath>

namespace
{
bool tryParseNumericValue(const juce::String& text, double& out)
{
    auto numeric = text.retainCharacters("0123456789.-");
    if (numeric.isEmpty() || numeric == "-" || numeric == "." || numeric == "-.")
        return false;
    out = numeric.getDoubleValue();
    return true;
}
}

void LabelWithContainer::setAnimatedValueText(const juce::String& text)
{
    if (!isValueLabelStyle || isEditing || !flipAnimationEnabled || !flipHasVisualEffect)
    {
        stopTimer();
        isAnimatingFlip = false;
        juce::Label::setText(text, juce::dontSendNotification);
        return;
    }

    const juce::String current = getText();
    if (current == text)
        return;

    flipFromText = current;
    flipToText = text;
    flipFromMappedText = flipToText;
    flipDirection = 0;
    double oldValue = 0.0;
    double newValue = 0.0;
    if (tryParseNumericValue(flipFromText, oldValue) && tryParseNumericValue(flipToText, newValue))
    {
        if (newValue > oldValue) flipDirection = +1;
        else if (newValue < oldValue) flipDirection = -1;
    }
    flippingCharIndices.clearQuick();
    const auto isDigit = [](juce::juce_wchar c) { return c >= '0' && c <= '9'; };
    juce::Array<int> fromDigitIndices;
    juce::Array<int> toDigitIndices;
    fromDigitIndices.ensureStorageAllocated(flipFromText.length());
    toDigitIndices.ensureStorageAllocated(flipToText.length());
    for (int i = 0; i < flipFromText.length(); ++i)
        if (isDigit(flipFromText[i]))
            fromDigitIndices.add(i);
    for (int i = 0; i < flipToText.length(); ++i)
        if (isDigit(flipToText[i]))
            toDigitIndices.add(i);

    juce::Array<juce::juce_wchar> mappedFromChars;
    mappedFromChars.ensureStorageAllocated(flipToText.length());
    for (int i = 0; i < flipToText.length(); ++i)
        mappedFromChars.add(flipToText[i]);

    int fromIdx = fromDigitIndices.size() - 1;
    int toIdx = toDigitIndices.size() - 1;
    while (toIdx >= 0)
    {
        const int toCharIndex = toDigitIndices[toIdx];
        const juce::juce_wchar toDigit = flipToText[toCharIndex];
        const bool hasFromDigit = fromIdx >= 0;
        const juce::juce_wchar fromDigit = hasFromDigit ? flipFromText[fromDigitIndices[fromIdx]] : ' ';
        mappedFromChars.set(toCharIndex, fromDigit);
        if (fromDigit != toDigit)
            flippingCharIndices.add(toCharIndex);
        --toIdx;
        if (hasFromDigit)
            --fromIdx;
    }

    flipFromMappedText.clear();
    for (int i = 0; i < mappedFromChars.size(); ++i)
        flipFromMappedText += juce::String::charToString(mappedFromChars[i]);

    juce::Label::setText(text, juce::dontSendNotification);

    if (flippingCharIndices.isEmpty())
    {
        stopTimer();
        isAnimatingFlip = false;
        repaint();
        return;
    }

    isAnimatingFlip = true;
    flipProgress = 0.0f;
    flipStartTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
    repaint();
}

void LabelWithContainer::setFlipAnimationParams(bool enabled, int durationMs, float travelUpPx, float travelDownPx,
                                                float travelOutScale, float travelInScale, float shearAmount, float minScale)
{
    flipAnimationEnabled = enabled;
    flipDurationMs = juce::jlimit(20, 1000, durationMs);
    flipTravelUpPx = juce::jlimit(0.0f, 200.0f, travelUpPx);
    flipTravelDownPx = juce::jlimit(0.0f, 200.0f, travelDownPx);
    flipTravelOutScale = juce::jlimit(0.0f, 8.0f, travelOutScale);
    flipTravelInScale = juce::jlimit(0.0f, 8.0f, travelInScale);
    flipShearAmount = juce::jlimit(0.0f, 1.0f, shearAmount);
    flipMinScale = juce::jlimit(0.0f, 0.95f, minScale);
    flipHasVisualEffect = (flipTravelUpPx > 0.0f || flipTravelDownPx > 0.0f ||
                           flipTravelOutScale > 0.0f || flipTravelInScale > 0.0f ||
                           flipShearAmount > 0.0f || flipMinScale < 0.999f);

    if (!flipAnimationEnabled || !flipHasVisualEffect)
    {
        stopTimer();
        isAnimatingFlip = false;
        repaint();
    }
}

void LabelWithContainer::setValueFxParams(bool enabled,
                                          float glowAlpha,
                                          float glowSpreadPx,
                                          float perCharOffsetX,
                                          float perCharOffsetY,
                                          float topAlpha,
                                          float topOffsetX,
                                          float topOffsetY,
                                          float topShear,
                                          float topRotateDeg,
                                          float bottomAlpha,
                                          float bottomOffsetX,
                                          float bottomOffsetY,
                                          float bottomShear,
                                          float bottomRotateDeg,
                                          float reflectBlurPx,
                                          float reflectSquash,
                                          float reflectMotionAmount)
{
    valueFxEnabled = enabled;
    valueGlowAlpha = juce::jlimit(0.0f, 1.0f, glowAlpha);
    valueGlowSpreadPx = juce::jlimit(0.0f, 8.0f, glowSpreadPx);
    valueFxPerCharOffsetX = juce::jlimit(-8.0f, 8.0f, perCharOffsetX);
    valueFxPerCharOffsetY = juce::jlimit(-8.0f, 8.0f, perCharOffsetY);
    valueTopReflectAlpha = juce::jlimit(0.0f, 1.0f, topAlpha);
    valueTopReflectOffsetX = juce::jlimit(-12.0f, 12.0f, topOffsetX);
    valueTopReflectOffsetY = juce::jlimit(-12.0f, 12.0f, topOffsetY);
    valueTopReflectShear = juce::jlimit(-1.0f, 1.0f, topShear);
    valueTopReflectRotateDeg = juce::jlimit(-180.0f, 180.0f, topRotateDeg);
    valueBottomReflectAlpha = juce::jlimit(0.0f, 1.0f, bottomAlpha);
    valueBottomReflectOffsetX = juce::jlimit(-12.0f, 12.0f, bottomOffsetX);
    valueBottomReflectOffsetY = juce::jlimit(-12.0f, 12.0f, bottomOffsetY);
    valueBottomReflectShear = juce::jlimit(-1.0f, 1.0f, bottomShear);
    valueBottomReflectRotateDeg = juce::jlimit(-180.0f, 180.0f, bottomRotateDeg);
    valueReflectBlurPx = juce::jlimit(0.0f, 8.0f, reflectBlurPx);
    valueReflectSquash = juce::jlimit(0.0f, 0.95f, reflectSquash);
    valueReflectMotionAmount = juce::jlimit(0.0f, 2.0f, reflectMotionAmount);
}

void LabelWithContainer::setValueLabelStyle(bool isValueLabel)
{
    isValueLabelStyle = isValueLabel;
    if (isValueLabel)
    {
        setEditable(true, false, false);  // Editable on double-click, not single-click
    }
}

void LabelWithContainer::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (isValueLabelStyle && onValueEdited)
    {
        showEditor();
    }
    else
    {
        juce::Label::mouseDoubleClick(e);
    }
}

void LabelWithContainer::setEditorTextColor(juce::Colour color)
{
    editorTextColor = color;
}

juce::TextEditor* LabelWithContainer::createEditorComponent()
{
    if (!isValueLabelStyle)
        return juce::Label::createEditorComponent();
    
    auto* editor = new juce::TextEditor();
    editor->setJustification(juce::Justification::centred);
    editor->setColour(juce::TextEditor::textColourId, editorTextColor);
    editor->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff000000));
    editor->setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff404040));
    editor->setColour(juce::TextEditor::focusedOutlineColourId, editorTextColor);
    editor->setBorder(juce::BorderSize<int>(0));
    editor->setFont(getFont());
    editor->setText(getText(), false);
    editor->setSelectAllWhenFocused(true);
    editor->setReturnKeyStartsNewLine(false);
    
    return editor;
}

void LabelWithContainer::editorShown(juce::TextEditor* editor)
{
    if (editor && isValueLabelStyle)
    {
        isEditing = true;
        stopTimer();
        isAnimatingFlip = false;
        repaint();  // Repaint to hide underlying text
        
        // Select all text for easy replacement
        editor->selectAll();
        
        // Store original text in case we need to restore it
        juce::String originalText = getText();
        
        // Set up callback for when editing finishes
        editor->onReturnKey = [this, editor, originalText]()
        {
            bool valueApplied = false;
            pendingFormattedText = originalText;  // Default to original
            
            if (onValueEdited)
            {
                valueApplied = onValueEdited(editor->getText());
                // If value was applied, get the formatted text that was set
                if (valueApplied)
                {
                    pendingFormattedText = getText();  // Get the formatted text that was set
                }
            }
            
            isEditing = false;
            hideEditor(true);  // Discard editor contents - we'll set text in editorAboutToBeHidden
        };
        
        editor->onEscapeKey = [this, originalText]()
        {
            pendingFormattedText = originalText;  // Restore original
            isEditing = false;
            hideEditor(true);  // Discard editor contents
        };
        
        editor->onFocusLost = [this, editor, originalText]()
        {
            bool valueApplied = false;
            pendingFormattedText = originalText;  // Default to original
            
            if (onValueEdited)
            {
                valueApplied = onValueEdited(editor->getText());
                // If value was applied, get the formatted text that was set
                if (valueApplied)
                {
                    pendingFormattedText = getText();  // Get the formatted text that was set
                }
            }
            
            isEditing = false;
            hideEditor(true);  // Discard editor contents - we'll set text in editorAboutToBeHidden
        };
    }
    else
    {
        juce::Label::editorShown(editor);
    }
}

void LabelWithContainer::editorAboutToBeHidden(juce::TextEditor* editor)
{
    if (editor && isValueLabelStyle)
    {
        // Set the formatted text AFTER editor is about to be hidden but BEFORE it's actually hidden
        // This ensures the text is set correctly and won't be overwritten
        if (!pendingFormattedText.isEmpty())
        {
            setText(pendingFormattedText, juce::dontSendNotification);
            pendingFormattedText.clear();
        }
    }
    
    // Call base class to maintain JUCE's normal behavior
    juce::Label::editorAboutToBeHidden(editor);
}

void LabelWithContainer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float cornerRadius = 4.0f;
    const float borderWidth = 1.0f;
    
    if (isValueLabelStyle)
    {
        // Value label style: text only (no background)
        const auto innerBounds = bounds.reduced(borderWidth);
        // Only draw text if not editing (editor will show its own text)
        if (!isEditing)
        {
            g.setColour(findColour(juce::Label::textColourId));
            g.setFont(getFont());
            const bool isFlipActive = (isAnimatingFlip && flipProgress < 1.0f && !flippingCharIndices.isEmpty());
            const juce::String& newText = isFlipActive ? flipToText : getText();
            const juce::String& oldText = isFlipActive ? flipFromMappedText : newText;
            const auto justification = getJustificationType();
            const float textHeight = innerBounds.getHeight();
            const float motionStrength = isFlipActive ? (1.0f - std::abs(2.0f * flipProgress - 1.0f)) : 0.0f;

            const int maxLen = newText.length();
            juce::Array<float> charWidths;
            charWidths.ensureStorageAllocated(maxLen);
            float digitSlotWidth = 0.0f;
            for (juce::juce_wchar d = '0'; d <= '9'; ++d)
                digitSlotWidth = juce::jmax(digitSlotWidth, juce::GlyphArrangement::getStringWidth(getFont(), juce::String::charToString(d)));
            digitSlotWidth = juce::jmax(digitSlotWidth, textHeight * 0.26f);
            const float symbolMinWidth = juce::jmax(textHeight * 0.16f, digitSlotWidth * 0.38f);
            const float slotPad = juce::jmax(0.8f, textHeight * 0.03f);
            const auto slotWidthFor = [symbolMinWidth, digitSlotWidth, slotPad](juce::juce_wchar ch, float measured) -> float
            {
                const bool isDigit = (ch >= '0' && ch <= '9');
                if (ch == ' ')
                    return symbolMinWidth * 0.9f;
                if (isDigit)
                    return juce::jmax(digitSlotWidth, measured * 1.05f) + slotPad;
                // Punctuation/units can report tiny widths in this typeface; enforce a practical slot.
                return juce::jmax(symbolMinWidth, measured * 1.3f) + slotPad;
            };
            float totalWidth = 0.0f;
            for (int i = 0; i < maxLen; ++i)
            {
                const juce::String newCh = newText.substring(i, i + 1);
                const juce::String oldCh = i < oldText.length() ? oldText.substring(i, i + 1) : newCh;
                const float wNew = juce::GlyphArrangement::getStringWidth(getFont(), newCh);
                const float wOld = juce::GlyphArrangement::getStringWidth(getFont(), oldCh);
                const juce::juce_wchar cNew = newText[i];
                const juce::juce_wchar cOld = i < oldText.length() ? oldText[i] : cNew;
                const float w = juce::jmax(slotWidthFor(cNew, wNew), slotWidthFor(cOld, wOld));
                charWidths.add(w);
                totalWidth += w;
            }

            float startX = innerBounds.getX();
            if (justification.testFlags(juce::Justification::horizontallyCentred))
                startX = innerBounds.getCentreX() - (totalWidth * 0.5f);
            else if (justification.testFlags(juce::Justification::right))
                startX = innerBounds.getRight() - totalWidth;

            const auto drawGlyph = [this, &g, motionStrength](juce::juce_wchar glyph,
                                                               const juce::Rectangle<float>& area,
                                                               float alpha,
                                                               float scaleY,
                                                               bool pivotBottom,
                                                               float shearSign,
                                                               float translateY)
            {
                if (alpha <= 0.0f || scaleY <= 0.001f)
                    return;

                const auto translatedArea = area.translated(0.0f, translateY);
                const float clampedScale = juce::jmax(0.001f, scaleY);

                if (valueFxEnabled)
                {
                    const float phaseX = valueFxPerCharOffsetX;
                    const float phaseY = valueFxPerCharOffsetY;
                    const float dyn = 1.0f + (valueReflectMotionAmount * motionStrength);
                    const float reflectScaleY = juce::jlimit(0.08f, 1.0f, 1.0f - (valueReflectSquash * dyn));
                    const float blurRadius = valueReflectBlurPx * dyn;

                    if (valueGlowAlpha > 0.0f && valueGlowSpreadPx > 0.0f)
                    {
                        const float glowAlpha = alpha * valueGlowAlpha;
                        const float spread = valueGlowSpreadPx;
                        constexpr float dirs[8][2] = {
                            { -1.0f,  0.0f }, { 1.0f,  0.0f }, { 0.0f, -1.0f }, { 0.0f,  1.0f },
                            { -0.7f, -0.7f }, { 0.7f, -0.7f }, { -0.7f,  0.7f }, { 0.7f,  0.7f }
                        };
                        for (const auto& d : dirs)
                        {
                            juce::Graphics::ScopedSaveState glowState(g);
                            g.setOpacity(glowAlpha * 0.14f);
                            g.drawText(juce::String::charToString(glyph),
                                       translatedArea.translated(d[0] * spread, d[1] * spread).toNearestInt(),
                                       juce::Justification::centred, false);
                        }
                    }

                    if (valueTopReflectAlpha > 0.0f)
                    {
                        const auto topArea = translatedArea.translated(valueTopReflectOffsetX + phaseX, valueTopReflectOffsetY + phaseY);
                        constexpr float blurDirs[4][2] = { { -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, -1.0f }, { 0.0f, 1.0f } };
                        if (blurRadius > 0.0f)
                        {
                            for (const auto& d : blurDirs)
                            {
                                juce::Graphics::ScopedSaveState topBlur(g);
                                g.addTransform(juce::AffineTransform().sheared(0.0f, valueTopReflectShear * dyn));
                                g.addTransform(juce::AffineTransform::scale(1.0f, reflectScaleY, topArea.getCentreX(), topArea.getBottom()));
                                g.addTransform(juce::AffineTransform::rotation(juce::degreesToRadians(valueTopReflectRotateDeg), topArea.getCentreX(), topArea.getBottom()));
                                g.setOpacity(alpha * valueTopReflectAlpha * 0.28f);
                                g.drawText(juce::String::charToString(glyph), topArea.translated(d[0] * blurRadius, d[1] * blurRadius).toNearestInt(), juce::Justification::centred, false);
                            }
                        }
                        juce::Graphics::ScopedSaveState topState(g);
                        g.addTransform(juce::AffineTransform().sheared(0.0f, valueTopReflectShear * dyn));
                        g.addTransform(juce::AffineTransform::scale(1.0f, reflectScaleY, topArea.getCentreX(), topArea.getBottom()));
                        g.addTransform(juce::AffineTransform::rotation(juce::degreesToRadians(valueTopReflectRotateDeg), topArea.getCentreX(), topArea.getBottom()));
                        g.setOpacity(alpha * valueTopReflectAlpha);
                        g.drawText(juce::String::charToString(glyph), topArea.toNearestInt(), juce::Justification::centred, false);
                    }

                    if (valueBottomReflectAlpha > 0.0f)
                    {
                        const auto bottomArea = translatedArea.translated(valueBottomReflectOffsetX + phaseX, valueBottomReflectOffsetY + phaseY);
                        constexpr float blurDirs[4][2] = { { -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, -1.0f }, { 0.0f, 1.0f } };
                        if (blurRadius > 0.0f)
                        {
                            for (const auto& d : blurDirs)
                            {
                                juce::Graphics::ScopedSaveState bottomBlur(g);
                                g.addTransform(juce::AffineTransform().sheared(0.0f, valueBottomReflectShear * dyn));
                                g.addTransform(juce::AffineTransform::scale(1.0f, reflectScaleY, bottomArea.getCentreX(), bottomArea.getY()));
                                g.addTransform(juce::AffineTransform::rotation(juce::degreesToRadians(valueBottomReflectRotateDeg), bottomArea.getCentreX(), bottomArea.getY()));
                                g.setOpacity(alpha * valueBottomReflectAlpha * 0.28f);
                                g.drawText(juce::String::charToString(glyph), bottomArea.translated(d[0] * blurRadius, d[1] * blurRadius).toNearestInt(), juce::Justification::centred, false);
                            }
                        }
                        juce::Graphics::ScopedSaveState bottomState(g);
                        g.addTransform(juce::AffineTransform().sheared(0.0f, valueBottomReflectShear * dyn));
                        g.addTransform(juce::AffineTransform::scale(1.0f, reflectScaleY, bottomArea.getCentreX(), bottomArea.getY()));
                        g.addTransform(juce::AffineTransform::rotation(juce::degreesToRadians(valueBottomReflectRotateDeg), bottomArea.getCentreX(), bottomArea.getY()));
                        g.setOpacity(alpha * valueBottomReflectAlpha);
                        g.drawText(juce::String::charToString(glyph), bottomArea.toNearestInt(), juce::Justification::centred, false);
                    }
                }

                {
                    juce::Graphics::ScopedSaveState mainState(g);
                    const float pivotY = pivotBottom ? translatedArea.getBottom() : translatedArea.getY();
                    const float shear = (1.0f - clampedScale) * flipShearAmount * shearSign;
                    g.addTransform(juce::AffineTransform().sheared(0.0f, shear));
                    g.addTransform(juce::AffineTransform::scale(1.0f, clampedScale, translatedArea.getCentreX(), pivotY));
                    g.setOpacity(alpha);
                    g.drawText(juce::String::charToString(glyph), translatedArea.toNearestInt(), juce::Justification::centred, false);
                }
            };

            float x = startX;
            for (int i = 0; i < maxLen; ++i)
            {
                const float w = charWidths[i];
                const juce::Rectangle<float> charArea(x, innerBounds.getY(), w, textHeight);
                const bool shouldFlip = isFlipActive && flippingCharIndices.contains(i);
                const juce::juce_wchar toGlyph = newText[i];
                const juce::juce_wchar fromGlyph = i < oldText.length() ? oldText[i] : toGlyph;
                if (!shouldFlip)
                {
                    drawGlyph(toGlyph, charArea, 1.0f, 1.0f, true, 0.0f, 0.0f);
                }
                else
                {
                    const bool valueDown = (flipDirection < 0);
                    const bool outgoingPivotBottom = !valueDown;
                    const bool incomingPivotBottom = valueDown;
                    const float outgoingShearSign = valueDown ? -1.0f : 1.0f;
                    const float incomingShearSign = -outgoingShearSign;
                    const float baseTravelPx = valueDown ? flipTravelDownPx : flipTravelUpPx;
                    const float outgoingDir = valueDown ? -1.0f : 1.0f;
                    const float incomingDir = -outgoingDir;
                    const float outgoingTravelPx = baseTravelPx * flipTravelOutScale;
                    const float incomingTravelPx = baseTravelPx * flipTravelInScale;
                    const float clipTravelPx = juce::jmax(outgoingTravelPx, incomingTravelPx);
                    juce::Graphics::ScopedSaveState charState(g);
                    g.reduceClipRegion(charArea.expanded(0.0f, clipTravelPx + 2.0f).toNearestInt());
                    const float outgoingY = outgoingDir * outgoingTravelPx * flipProgress;
                    const float incomingY = incomingDir * incomingTravelPx * (1.0f - flipProgress);
                    const float outgoingScale = flipMinScale + (1.0f - flipMinScale) * (1.0f - flipProgress);
                    const float incomingScale = flipMinScale + (1.0f - flipMinScale) * flipProgress;
                    drawGlyph(fromGlyph, charArea, 1.0f - flipProgress, outgoingScale, outgoingPivotBottom, outgoingShearSign, outgoingY);
                    drawGlyph(toGlyph, charArea, flipProgress, incomingScale, incomingPivotBottom, incomingShearSign, incomingY);
                }

                x += w;
            }
        }
    }
    else
    {
        // Name label style: dark background with green gradient border (original style)
        // Create gradient for border (subtle gradient from lighter to darker green)
        juce::ColourGradient gradient(
            juce::Colour(0xff4a6b5a), bounds.getX(), bounds.getY(),  // Top-left: lighter green
            juce::Colour(0xff2a3b32), bounds.getX(), bounds.getBottom(), // Bottom: darker green
            false
        );
        
        // Draw border with gradient
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerRadius);
        
        // Draw inner background (slightly inset for border effect)
        const auto innerBounds = bounds.reduced(borderWidth);
        g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f)); // Dark semi-transparent background
        g.fillRoundedRectangle(innerBounds, cornerRadius - borderWidth);
        
        // Draw text
        g.setColour(findColour(juce::Label::textColourId));
        g.setFont(getFont());
        g.drawText(getText(), innerBounds, getJustificationType(), false);
    }
}

void LabelWithContainer::timerCallback()
{
    if (!isAnimatingFlip)
    {
        stopTimer();
        return;
    }

    const double elapsedMs = juce::Time::getMillisecondCounterHiRes() - flipStartTimeMs;
    flipProgress = juce::jlimit(0.0f, 1.0f, static_cast<float>(elapsedMs / static_cast<double>(flipDurationMs)));
    repaint();

    if (flipProgress >= 1.0f)
    {
        isAnimatingFlip = false;
        stopTimer();
    }
}
