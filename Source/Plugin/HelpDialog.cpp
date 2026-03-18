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

#include "HelpDialog.h"
#include "PluginProcessor.h"
#include "../UI/DevPanelSupport.h"

namespace
{
constexpr auto kDocsUrl = "https://choroboros.kaizenstrategic.ai/docs";
constexpr auto kSupportMailto = "mailto:info@kaizenstrategic.ai?subject=Choroboros%20Support";
} // namespace

HelpDialog::HelpDialog()
{
    setSize(500, 390);

    const auto accent = devpanel::hackerText();
    const auto body = devpanel::hackerTextDim();
    const auto muted = devpanel::hackerTextMuted();

    titleLabel.setText("Choroboros - Help & Support", juce::dontSendNotification);
    titleLabel.setFont(devpanel::makeTitleFont(22.0f, true));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);

    introLabel.setText("Open the official documentation or contact support\nwithout leaving the plugin UI.",
                       juce::dontSendNotification);
    introLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::description, false));
    introLabel.setJustificationType(juce::Justification::centred);
    introLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(introLabel);

    docsHeaderLabel.setText("Documentation", juce::dontSendNotification);
    docsHeaderLabel.setFont(devpanel::makeLabelFont(14.0f, true));
    docsHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    docsHeaderLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(docsHeaderLabel);

    docsBodyLabel.setText("Read setup guidance, engine notes,\nknown issues, and release details.",
                          juce::dontSendNotification);
    docsBodyLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::description, false));
    docsBodyLabel.setJustificationType(juce::Justification::centredLeft);
    docsBodyLabel.setColour(juce::Label::textColourId, body);
    addAndMakeVisible(docsBodyLabel);

    supportHeaderLabel.setText("Support", juce::dontSendNotification);
    supportHeaderLabel.setFont(devpanel::makeLabelFont(14.0f, true));
    supportHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    supportHeaderLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(supportHeaderLabel);

    supportBodyLabel.setText("Open your default mail app with a prefilled\nsupport address for setup or account issues.",
                             juce::dontSendNotification);
    supportBodyLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::description, false));
    supportBodyLabel.setJustificationType(juce::Justification::centredLeft);
    supportBodyLabel.setColour(juce::Label::textColourId, body);
    addAndMakeVisible(supportBodyLabel);

    feedbackHintLabel.setText("Bug reports and feature requests use the separate Feedback button\nso support mail stays focused on assistance.",
                              juce::dontSendNotification);
    feedbackHintLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::labelSmall, false));
    feedbackHintLabel.setJustificationType(juce::Justification::centred);
    feedbackHintLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(feedbackHintLabel);

    devpanel::styleHackerTextButton(docsButton, true);
    docsButton.setButtonText("Open Docs");
    docsButton.onClick = [this] { openDocs(); };
    addAndMakeVisible(docsButton);

    devpanel::styleHackerTextButton(supportButton, false);
    supportButton.setButtonText("Email Support");
    supportButton.onClick = [this] { emailSupport(); };
    addAndMakeVisible(supportButton);

    devpanel::styleHackerTextButton(closeButton, false);
    closeButton.setButtonText("Close");
    closeButton.onClick = [this] { closeDialog(); };
    addAndMakeVisible(closeButton);
}

void HelpDialog::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll(devpanel::hackerBg());

    juce::ColourGradient bg(devpanel::hackerBgElevated(), 0.0f, 0.0f,
                            devpanel::hackerBg(), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds.reduced(6.0f), 8.0f);

    g.setColour(devpanel::hackerBorder().withAlpha(0.72f));
    g.drawRoundedRectangle(bounds.reduced(6.5f), 8.0f, 1.0f);

    g.setColour(devpanel::hackerText().withAlpha(0.06f));
    g.drawRoundedRectangle(bounds.reduced(10.0f), 6.0f, 0.8f);
}

void HelpDialog::resized()
{
    auto area = getLocalBounds().reduced(24);

    titleLabel.setBounds(area.removeFromTop(34));
    area.removeFromTop(8);
    introLabel.setBounds(area.removeFromTop(42));
    area.removeFromTop(18);

    auto docsArea = area.removeFromTop(82);
    docsHeaderLabel.setBounds(docsArea.removeFromTop(20));
    docsArea.removeFromTop(4);
    docsBodyLabel.setBounds(docsArea.removeFromTop(42));
    docsButton.setBounds(docsArea.removeFromTop(28).withWidth(120));

    area.removeFromTop(12);

    auto supportArea = area.removeFromTop(82);
    supportHeaderLabel.setBounds(supportArea.removeFromTop(20));
    supportArea.removeFromTop(4);
    supportBodyLabel.setBounds(supportArea.removeFromTop(42));
    supportButton.setBounds(supportArea.removeFromTop(28).withWidth(140));

    area.removeFromTop(14);
    feedbackHintLabel.setBounds(area.removeFromTop(34));

    area.removeFromTop(12);
    auto buttonArea = area.removeFromTop(30);
    closeButton.setBounds(buttonArea.getCentreX() - 50, buttonArea.getY(), 100, 28);
}

void HelpDialog::openDocs()
{
    juce::URL(kDocsUrl).launchInDefaultBrowser();
    closeDialog();
}

void HelpDialog::emailSupport()
{
    juce::URL(kSupportMailto).launchInDefaultBrowser();
    closeDialog();
}

void HelpDialog::closeDialog()
{
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState(0);
        dw->setVisible(false);
        return;
    }

    if (auto* parent = getParentComponent())
        parent->removeChildComponent(this);
    delete this;
}

void HelpDialog::show()
{
    auto* dialog = new HelpDialog();

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(dialog);
    options.dialogTitle = "Help & Support";
    options.dialogBackgroundColour = devpanel::hackerBg();
    options.resizable = false;
    options.useNativeTitleBar = true;

    auto* window = options.launchAsync();
    (void) window;
}
