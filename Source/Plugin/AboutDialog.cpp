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

#include "AboutDialog.h"
#include "PluginProcessor.h"
#include "../UI/DevPanelSupport.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

AboutDialog::AboutDialog()
{
    setSize(450, 520);

    const auto accent = devpanel::hackerText();        // bright green
    const auto body   = devpanel::hackerTextDim();     // softer green
    const auto muted  = devpanel::hackerTextMuted();   // dim green

    // --- Title ---
    titleLabel.setText("Choroboros", juce::dontSendNotification);
    titleLabel.setFont(devpanel::makeTitleFont(28.0f, true));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);

    // --- Version ---
#ifdef CHOROBOROS_VERSION_STRING
    versionLabel.setText(juce::String("v") + juce::String(CHOROBOROS_VERSION_STRING),
                         juce::dontSendNotification);
#else
    versionLabel.setText("v2.05", juce::dontSendNotification);
#endif
    versionLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::description, false));
    versionLabel.setJustificationType(juce::Justification::centred);
    versionLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(versionLabel);

    // --- Tagline ---
    descriptionLabel.setText("A chorus that eats its own tail\nFive colors \u2022 Ten algorithms",
                             juce::dontSendNotification);
    descriptionLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::description, false));
    descriptionLabel.setJustificationType(juce::Justification::centred);
    descriptionLabel.setColour(juce::Label::textColourId, body);
    addAndMakeVisible(descriptionLabel);

    // --- Company ---
    companyLabel.setText("Kaizen DSP", juce::dontSendNotification);
    companyLabel.setFont(devpanel::makeLabelFont(14.0f, true));
    companyLabel.setJustificationType(juce::Justification::centred);
    companyLabel.setColour(juce::Label::textColourId, body);
    addAndMakeVisible(companyLabel);

    locationLabel.setText("British Columbia, Canada", juce::dontSendNotification);
    locationLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::label, false));
    locationLabel.setJustificationType(juce::Justification::centred);
    locationLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(locationLabel);

    // --- Copyright ---
    copyrightLabel.setText("\u00A9 2026 Kaizen Strategic AI Inc.", juce::dontSendNotification);
    copyrightLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::labelSmall, false));
    copyrightLabel.setJustificationType(juce::Justification::centred);
    copyrightLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(copyrightLabel);

    // --- Contact link (replaces separate contactLabel + contactLink) ---
    contactLink.setButtonText("info@kaizenstrategic.ai");
    contactLink.setURL(juce::URL("mailto:info@kaizenstrategic.ai?subject=Choroboros%20Info"));
    contactLink.setFont(devpanel::makeLabelFont(devpanel::Typography::label, false), false);
    contactLink.setColour(juce::HyperlinkButton::textColourId, accent);
    addAndMakeVisible(contactLink);

    // --- Built with ---
    juceLabel.setText("Built with JUCE 8.0.12", juce::dontSendNotification);
    juceLabel.setFont(devpanel::makeLabelFont(devpanel::Typography::labelSmall, false));
    juceLabel.setJustificationType(juce::Justification::centred);
    juceLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(juceLabel);

    // --- Buttons (DevPanel hacker style) ---
    devpanel::styleHackerTextButton(licenseButton, true);
    licenseButton.setButtonText("View License");
    licenseButton.onClick = [this] { showLicense(); };
    addAndMakeVisible(licenseButton);

    devpanel::styleHackerTextButton(closeButton, false);
    closeButton.setButtonText("Close");
    closeButton.onClick = [this] { closeDialog(); };
    addAndMakeVisible(closeButton);
}

void AboutDialog::paint(juce::Graphics& g)
{
    const auto accent = devpanel::hackerText();
    const auto bounds = getLocalBounds().toFloat();

    // HackerTheme background
    g.fillAll(devpanel::hackerBg());
    juce::ColourGradient bg(devpanel::hackerBgElevated(), 0.0f, 0.0f,
                            devpanel::hackerBg(), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds.reduced(6.0f), 8.0f);

    // Border — matches DevPanel section styling
    g.setColour(devpanel::hackerBorder().withAlpha(0.7f));
    g.drawRoundedRectangle(bounds.reduced(6.5f), 8.0f, 1.0f);

    // Subtle inner glow
    g.setColour(accent.withAlpha(0.06f));
    g.drawRoundedRectangle(bounds.reduced(10.0f), 6.0f, 0.8f);
}

void AboutDialog::resized()
{
    auto area = getLocalBounds().reduced(28);

    titleLabel.setBounds(area.removeFromTop(42));
    area.removeFromTop(4);
    versionLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(16);

    descriptionLabel.setBounds(area.removeFromTop(44));
    area.removeFromTop(24);

    companyLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(4);
    locationLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(20);

    copyrightLabel.setBounds(area.removeFromTop(18));
    area.removeFromTop(16);
    contactLink.setBounds(area.removeFromTop(22));
    area.removeFromTop(20);

    juceLabel.setBounds(area.removeFromTop(18));
    area.removeFromTop(16);

    // Centred button row
    auto buttonArea = area.removeFromTop(30);
    const int totalW = 250;
    const int bx = (buttonArea.getWidth() - totalW) / 2;
    licenseButton.setBounds(buttonArea.getX() + bx, buttonArea.getY(), 120, 28);
    closeButton.setBounds(buttonArea.getX() + bx + 130, buttonArea.getY(), 120, 28);
}

void AboutDialog::closeDialog()
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
    juce::Component::SafePointer<AboutDialog> safeThis(this);
    if (auto* parent = getParentComponent())
        parent->removeChildComponent(this);
    juce::MessageManager::callAsync([safeThis]()
    {
        delete safeThis.getComponent();
    });
}

void AboutDialog::showLicense()
{
    // Try to load EULA from BinaryData (bundled with plugin)
    const char* eulaData = BinaryData::EULA_md;
    int eulaSize = BinaryData::EULA_mdSize;

    if (eulaData != nullptr && eulaSize > 0)
    {
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        juce::File tempEula = tempDir.getChildFile("Choroboros_EULA.md");

        if (tempEula.replaceWithData(eulaData, static_cast<size_t>(eulaSize)))
        {
            juce::URL("file://" + tempEula.getFullPathName()).launchInDefaultBrowser();
            return;
        }
    }

    // Fallback: Try to find EULA.md relative to executable
    auto appDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    juce::File eulaFile;

    if (appDir.getFileName() == "MacOS")
    {
        auto contentsDir = appDir.getParentDirectory();
        eulaFile = contentsDir.getChildFile("Resources").getChildFile("EULA.md");
        if (!eulaFile.existsAsFile())
            eulaFile = contentsDir.getChildFile("EULA.md");
    }
    else
    {
        eulaFile = appDir.getChildFile("EULA.md");
        if (!eulaFile.existsAsFile())
            eulaFile = appDir.getParentDirectory().getChildFile("EULA.md");
    }

    if (eulaFile.existsAsFile())
    {
        juce::URL("file://" + eulaFile.getFullPathName()).launchInDefaultBrowser();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "End User License Agreement",
            "Choroboros End User License Agreement\n\n"
            "Copyright (C) 2026 Kaizen Strategic AI Inc.\n"
            "British Columbia, Canada\n\n"
            "This software is licensed, not sold. By using this software, you agree "
            "to the terms of the End User License Agreement.\n\n"
            "PROPRIETARY ALGORITHMS:\n"
            "The Purple engine algorithms (Phase-Warped Chorus and Orbit Chorus) are "
            "proprietary intellectual property of Kaizen Strategic AI Inc. These "
            "algorithms are protected by trade secret law and may not be reverse "
            "engineered, extracted, copied, or used without explicit written license.\n\n"
            "THIRD-PARTY COMPONENTS:\n"
            "This software uses the JUCE framework, subject to its own license terms.\n\n"
            "For the complete EULA, please contact:\n"
            "info@kaizenstrategic.ai");
    }
}

void AboutDialog::show()
{
    auto* dialog = new AboutDialog();

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(dialog);
    options.dialogTitle = "About Choroboros";
    options.dialogBackgroundColour = devpanel::hackerBg();
    options.resizable = false;
    options.useNativeTitleBar = true;

    auto* window = options.launchAsync();
    (void)window;
}
