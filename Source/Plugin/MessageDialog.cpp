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

#include "MessageDialog.h"
#include "ManagedDialogStyle.h"

MessageDialog::MessageDialog(juce::String title, juce::String message, bool warning)
    : warningTone(warning)
{
    setSize(520, 320);

    const auto accent = choroboros::ui::managedDialogAccent(warningTone);

    titleLabel.setText(std::move(title), juce::dontSendNotification);
    titleLabel.setFont(choroboros::ui::managedDialogTitleFont());
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);

    choroboros::ui::styleManagedDialogEditor(messageEditor, accent);
    messageEditor.setMultiLine(true, true);
    messageEditor.setReadOnly(true);
    messageEditor.setScrollbarsShown(true);
    messageEditor.setCaretVisible(false);
    messageEditor.setPopupMenuEnabled(true);
    messageEditor.setText(std::move(message), juce::dontSendNotification);
    messageEditor.setColour(juce::TextEditor::textColourId,
                            warningTone ? choroboros::ui::managedDialogBodyTextColour()
                                        : choroboros::ui::managedDialogMutedTextColour());
    messageEditor.setColour(juce::CaretComponent::caretColourId, accent);
    addAndMakeVisible(messageEditor);

    choroboros::ui::styleManagedDialogButton(closeButton, !warningTone, accent);
    closeButton.setButtonText("Close");
    closeButton.onClick = [this] { closeDialog(); };
    addAndMakeVisible(closeButton);
}

void MessageDialog::paint(juce::Graphics& g)
{
    choroboros::ui::paintManagedDialogBackground(g,
                                                 getLocalBounds().toFloat(),
                                                 choroboros::ui::managedDialogAccent(warningTone));
}

void MessageDialog::resized()
{
    auto area = getLocalBounds().reduced(20);
    titleLabel.setBounds(area.removeFromTop(28));
    area.removeFromTop(12);

    auto buttonArea = area.removeFromBottom(30);
    messageEditor.setBounds(area);

    closeButton.setBounds(buttonArea.getCentreX() - 50, buttonArea.getY(), 100, 28);
}

void MessageDialog::closeDialog()
{
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState(0);
        dw->setVisible(false);
        return;
    }

    juce::Component::SafePointer<MessageDialog> safeThis(this);
    if (auto* parent = getParentComponent())
        parent->removeChildComponent(this);
    juce::MessageManager::callAsync([safeThis]()
    {
        delete safeThis.getComponent();
    });
}
