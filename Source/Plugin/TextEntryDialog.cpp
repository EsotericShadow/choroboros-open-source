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

#include "TextEntryDialog.h"
#include "ManagedDialogStyle.h"

TextEntryDialog::TextEntryDialog(juce::String title,
                                 juce::String prompt,
                                 juce::String initialText,
                                 juce::String confirmText,
                                 juce::String cancelText,
                                 bool warningTone,
                                 DecisionCallback callback)
    : warning(warningTone),
      decisionCallback(std::move(callback))
{
    setSize(520, 220);

    const auto accent = choroboros::ui::managedDialogAccent(warning);

    titleLabel.setText(std::move(title), juce::dontSendNotification);
    titleLabel.setFont(choroboros::ui::managedDialogTitleFont());
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);

    promptLabel.setText(std::move(prompt), juce::dontSendNotification);
    promptLabel.setFont(choroboros::ui::managedDialogBodyFont(13.5f));
    promptLabel.setJustificationType(juce::Justification::centredLeft);
    promptLabel.setColour(juce::Label::textColourId, choroboros::ui::managedDialogMutedTextColour());
    addAndMakeVisible(promptLabel);

    choroboros::ui::styleManagedDialogEditor(inputEditor, accent);
    inputEditor.setMultiLine(false, false);
    inputEditor.setReturnKeyStartsNewLine(false);
    inputEditor.setText(std::move(initialText), juce::dontSendNotification);
    inputEditor.setSelectAllWhenFocused(true);
    inputEditor.onReturnKey = [this] { finish(true); };
    inputEditor.onEscapeKey = [this] { finish(false); };
    addAndMakeVisible(inputEditor);

    choroboros::ui::styleManagedDialogButton(confirmButton, true, accent);
    confirmButton.setButtonText(std::move(confirmText));
    confirmButton.onClick = [this] { finish(true); };
    addAndMakeVisible(confirmButton);

    choroboros::ui::styleManagedDialogButton(cancelButton, false, accent);
    cancelButton.setButtonText(std::move(cancelText));
    cancelButton.onClick = [this] { finish(false); };
    addAndMakeVisible(cancelButton);
}

void TextEntryDialog::paint(juce::Graphics& g)
{
    choroboros::ui::paintManagedDialogBackground(g,
                                                 getLocalBounds().toFloat(),
                                                 choroboros::ui::managedDialogAccent(warning));
}

void TextEntryDialog::resized()
{
    auto area = getLocalBounds().reduced(20);
    titleLabel.setBounds(area.removeFromTop(28));
    area.removeFromTop(10);
    promptLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(10);
    inputEditor.setBounds(area.removeFromTop(42));
    area.removeFromTop(18);

    auto buttonArea = area.removeFromTop(30);
    const int buttonWidth = 120;
    const int gap = 10;
    const int totalWidth = buttonWidth * 2 + gap;
    const int startX = buttonArea.getCentreX() - totalWidth / 2;
    confirmButton.setBounds(startX, buttonArea.getY(), buttonWidth, 28);
    cancelButton.setBounds(startX + buttonWidth + gap, buttonArea.getY(), buttonWidth, 28);
}

void TextEntryDialog::visibilityChanged()
{
    if (!isVisible())
        return;

    inputEditor.grabKeyboardFocus();
    inputEditor.selectAll();
}

void TextEntryDialog::finish(bool accepted)
{
    auto callback = std::move(decisionCallback);
    decisionCallback = {};
    const auto text = inputEditor.getText();

    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState(accepted ? 1 : 0);
        dw->setVisible(false);
    }
    else
    {
        juce::Component::SafePointer<TextEntryDialog> safeThis(this);
        if (auto* parent = getParentComponent())
            parent->removeChildComponent(this);
        juce::MessageManager::callAsync([safeThis]()
        {
            delete safeThis.getComponent();
        });
    }

    if (callback)
        callback(accepted, text);
}
