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
constexpr auto kSupportUrl = "https://choroboros.kaizenstrategic.ai/support";
} // namespace

HelpDialog::HelpDialog(MessageCallback callback)
    : showMessageCallback(std::move(callback))
{
    setSize(500, 436);

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

    supportBodyLabel.setText("Open the official support page for setup,\ncompatibility, and release guidance.",
                             juce::dontSendNotification);
    supportBodyLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::description, false));
    supportBodyLabel.setJustificationType(juce::Justification::centredLeft);
    supportBodyLabel.setColour(juce::Label::textColourId, body);
    addAndMakeVisible(supportBodyLabel);

    feedbackHintLabel.setText("Bug reports and feature requests: use the Feedback button.\n"
                              "Licensing: this official Kaizen beta build is under the EULA (About \u2192 View EULA).\n"
                              "Public source on GitHub remains GPLv3 for self-builds.",
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
    supportButton.setButtonText("Open Support");
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
    feedbackHintLabel.setBounds(area.removeFromTop(52));

    area.removeFromTop(12);
    auto buttonArea = area.removeFromTop(30);
    closeButton.setBounds(buttonArea.getCentreX() - 50, buttonArea.getY(), 100, 28);
}

void HelpDialog::openDocs()
{
    if (! juce::URL(kDocsUrl).launchInDefaultBrowser() && showMessageCallback)
    {
        showMessageCallback(juce::AlertWindow::WarningIcon,
                            "Couldn't Open Documentation",
                            "Choroboros couldn't open your default browser.\n\n"
                            "Open this URL manually:\n" + juce::String(kDocsUrl));
        return;
    }

    closeDialog();
}

void HelpDialog::emailSupport()
{
    if (! juce::URL(kSupportUrl).launchInDefaultBrowser() && showMessageCallback)
    {
        showMessageCallback(juce::AlertWindow::WarningIcon,
                            "Couldn't Open Support Page",
                            "Choroboros couldn't open your default browser.\n\n"
                            "Open this URL manually:\n" + juce::String(kSupportUrl) + "\n\n"
                            "Direct support email:\ninfo@kaizenstrategic.ai");
        return;
    }

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

    // Defer destruction so the call stack unwinds before `this` is deleted.
    // SafePointer guards against the component being destroyed by other means
    // before the async callback fires.
    juce::Component::SafePointer<HelpDialog> safeThis(this);
    if (auto* parent = getParentComponent())
        parent->removeChildComponent(this);
    juce::MessageManager::callAsync([safeThis]()
    {
        delete safeThis.getComponent();
    });
}
