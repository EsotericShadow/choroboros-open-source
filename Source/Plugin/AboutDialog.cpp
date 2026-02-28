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
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace
{
juce::Font makeRetroFont(float height, bool bold)
{
    juce::Font font { juce::FontOptions { height, bold ? juce::Font::bold : juce::Font::plain } };
    if (BinaryData::Retroica_ttfSize > 0)
    {
        static juce::Typeface::Ptr retroTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Retroica_ttf,
            static_cast<size_t>(BinaryData::Retroica_ttfSize));
        if (retroTypeface != nullptr)
            font = juce::Font { juce::FontOptions { retroTypeface }.withHeight(height) };
    }
    if (bold)
        font.setBold(true);
    return font;
}
} // namespace

AboutDialog::AboutDialog()
{
    setSize(450, 550);

    const auto accent = juce::Colour(0xff9dbd78);
    const auto bodyText = juce::Colour(0xffe8ecf1);
    const auto mutedText = juce::Colour(0xff9aa5b3);
    
    titleLabel.setText("Choroboros", juce::dontSendNotification);
    titleLabel.setFont(makeRetroFont(33.0f, true));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);
    
#ifdef CHOROBOROS_VERSION_STRING
    versionLabel.setText(juce::String("Version ") + juce::String(CHOROBOROS_VERSION_STRING), juce::dontSendNotification);
#else
    versionLabel.setText("Version 2.02-beta", juce::dontSendNotification);
#endif
    versionLabel.setFont(makeRetroFont(14.0f, false));
    versionLabel.setJustificationType(juce::Justification::centred);
    versionLabel.setColour(juce::Label::textColourId, mutedText);
    addAndMakeVisible(versionLabel);
    
    descriptionLabel.setText("A chorus that eats its own tail\nFive colors, ten algorithms", juce::dontSendNotification);
    descriptionLabel.setFont(makeRetroFont(14.0f, false));
    descriptionLabel.setJustificationType(juce::Justification::centred);
    descriptionLabel.setColour(juce::Label::textColourId, bodyText);
    addAndMakeVisible(descriptionLabel);
    
    companyLabel.setText("Kaizen Strategic AI Inc", juce::dontSendNotification);
    companyLabel.setFont(makeRetroFont(16.0f, true));
    companyLabel.setJustificationType(juce::Justification::centred);
    companyLabel.setColour(juce::Label::textColourId, bodyText);
    addAndMakeVisible(companyLabel);
    
    dbaLabel.setText("DBA: Green DSP", juce::dontSendNotification);
    dbaLabel.setFont(makeRetroFont(14.0f, false));
    dbaLabel.setJustificationType(juce::Justification::centred);
    dbaLabel.setColour(juce::Label::textColourId, bodyText);
    addAndMakeVisible(dbaLabel);
    
    locationLabel.setText("British Columbia, Canada", juce::dontSendNotification);
    locationLabel.setFont(makeRetroFont(14.0f, false));
    locationLabel.setJustificationType(juce::Justification::centred);
    locationLabel.setColour(juce::Label::textColourId, bodyText);
    addAndMakeVisible(locationLabel);
    
    copyrightLabel.setText("(C) 2026 Kaizen Strategic AI Inc", juce::dontSendNotification);
    copyrightLabel.setFont(makeRetroFont(12.0f, false));
    copyrightLabel.setJustificationType(juce::Justification::centred);
    copyrightLabel.setColour(juce::Label::textColourId, mutedText);
    addAndMakeVisible(copyrightLabel);
    
    contactLabel.setText("Info:", juce::dontSendNotification);
    contactLabel.setFont(makeRetroFont(14.0f, false));
    contactLabel.setJustificationType(juce::Justification::centred);
    contactLabel.setColour(juce::Label::textColourId, bodyText);
    addAndMakeVisible(contactLabel);
    
    contactLink.setButtonText("info@kaizenstrategic.ai");
    contactLink.setURL(juce::URL("mailto:info@kaizenstrategic.ai?subject=Choroboros%20Info"));
    contactLink.setFont(makeRetroFont(14.0f, false), false);
    contactLink.setColour(juce::HyperlinkButton::textColourId, accent.brighter(0.18f));
    addAndMakeVisible(contactLink);
    
    juceLabel.setText("Built with JUCE 8.0.12", juce::dontSendNotification);
    juceLabel.setFont(makeRetroFont(11.0f, false));
    juceLabel.setJustificationType(juce::Justification::centred);
    juceLabel.setColour(juce::Label::textColourId, mutedText);
    addAndMakeVisible(juceLabel);
    
    closeButton.setButtonText("Close");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a3138));
    closeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a444d));
    closeButton.setColour(juce::TextButton::textColourOffId, bodyText);
    closeButton.setColour(juce::TextButton::textColourOnId, bodyText);
    closeButton.onClick = [this] { closeDialog(); };
    addAndMakeVisible(closeButton);
    
    licenseButton.setButtonText("View License");
    licenseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f2f23));
    licenseButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff2a4130));
    licenseButton.setColour(juce::TextButton::textColourOffId, accent.brighter(0.18f));
    licenseButton.setColour(juce::TextButton::textColourOnId, accent.brighter(0.18f));
    licenseButton.onClick = [this] { showLicense(); };
    addAndMakeVisible(licenseButton);
}

void AboutDialog::paint(juce::Graphics& g)
{
    const auto accent = juce::Colour(0xff9dbd78);
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll(juce::Colour(0xff090b0d));
    juce::ColourGradient bg(juce::Colour(0xff151a1f), 0.0f, 0.0f,
                            juce::Colour(0xff11161b), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds.reduced(8.0f), 10.0f);

    g.setColour(accent.withAlpha(0.78f));
    g.drawRoundedRectangle(bounds.reduced(8.5f), 10.0f, 1.2f);
    g.setColour(accent.withAlpha(0.18f));
    g.drawRoundedRectangle(bounds.reduced(12.0f), 8.0f, 1.0f);
}

void AboutDialog::resized()
{
    auto area = getLocalBounds().reduced(30);
    
    titleLabel.setBounds(area.removeFromTop(50));
    area.removeFromTop(10);
    
    versionLabel.setBounds(area.removeFromTop(25));
    area.removeFromTop(20);
    
    descriptionLabel.setBounds(area.removeFromTop(50));
    area.removeFromTop(30);
    
    companyLabel.setBounds(area.removeFromTop(25));
    area.removeFromTop(5);
    
    dbaLabel.setBounds(area.removeFromTop(25));
    area.removeFromTop(5);
    
    locationLabel.setBounds(area.removeFromTop(25));
    area.removeFromTop(30);
    
    copyrightLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(20);
    
    contactLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(5);
    
    contactLink.setBounds(area.removeFromTop(25));
    area.removeFromTop(30);
    
    juceLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(20);
    
    auto buttonArea = area.removeFromTop(35);
    licenseButton.setBounds(buttonArea.removeFromLeft(120).reduced(5, 0));
    buttonArea.removeFromLeft(10);
    closeButton.setBounds(buttonArea.removeFromLeft(120).reduced(5, 0));
}

void AboutDialog::closeDialog()
{
    if (auto* parent = getParentComponent())
    {
        parent->removeChildComponent(this);
    }
    delete this;
}

void AboutDialog::showLicense()
{
    // First, try to load EULA from BinaryData (bundled with plugin)
    const char* eulaData = BinaryData::EULA_md;
    int eulaSize = BinaryData::EULA_mdSize;
    
    if (eulaData != nullptr && eulaSize > 0)
    {
        // Save to temporary file and open it
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        juce::File tempEula = tempDir.getChildFile("Choroboros_EULA.md");
        
        if (tempEula.replaceWithData(eulaData, static_cast<size_t>(eulaSize)))
        {
            tempEula.revealToUser();
            juce::URL("file://" + tempEula.getFullPathName()).launchInDefaultBrowser();
            return;
        }
    }
    
    // Fallback: Try to find EULA.md in file system
    juce::File eulaFile;
    auto appDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    
    // Check if we're in a .app bundle (macOS)
    if (appDir.getFileName() == "MacOS")
    {
        // We're in Contents/MacOS, go up to Contents
        auto contentsDir = appDir.getParentDirectory();
        auto resourcesDir = contentsDir.getChildFile("Resources");
        eulaFile = resourcesDir.getChildFile("EULA.md");
        
        // If not in Resources, try Contents root
        if (!eulaFile.existsAsFile())
        {
            eulaFile = contentsDir.getChildFile("EULA.md");
        }
    }
    else
    {
        // Try current directory and parent directories
        eulaFile = appDir.getChildFile("EULA.md");
        if (!eulaFile.existsAsFile())
        {
            eulaFile = appDir.getParentDirectory().getChildFile("EULA.md");
        }
    }
    
    if (eulaFile.existsAsFile())
    {
        eulaFile.revealToUser();
        juce::URL("file://" + eulaFile.getFullPathName()).launchInDefaultBrowser();
    }
    else
    {
        // Final fallback: Show a dialog with key license terms
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "End User License Agreement",
            "Choroboros End User License Agreement\n\n"
            "Copyright (C) 2026 Kaizen Strategic AI Inc.\n"
            "DBA: Green DSP\n"
            "British Columbia, Canada\n\n"
            "This software is licensed, not sold. By using this software, you agree to the terms of the End User License Agreement.\n\n"
            "PROPRIETARY ALGORITHMS:\n"
            "The Purple engine algorithms (Phase-Warped Chorus and Orbit Chorus) are proprietary intellectual property of Kaizen Strategic AI Inc. "
            "These algorithms are protected by trade secret law and may not be reverse engineered, extracted, copied, or used without explicit written license.\n\n"
            "THIRD-PARTY COMPONENTS:\n"
            "This software uses the JUCE framework, subject to its own license terms.\n\n"
            "For the complete EULA, please contact:\n"
            "info@kaizenstrategic.ai\n\n"
            "For questions about this license, please contact Kaizen Strategic AI Inc."
        );
    }
}

void AboutDialog::show()
{
    auto* dialog = new AboutDialog();
    
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(dialog);
    options.dialogTitle = "About Choroboros";
    options.dialogBackgroundColour = juce::Colour(0xff1a1a1a);
    options.resizable = false;
    options.useNativeTitleBar = true;
    
    auto* window = options.launchAsync();
    (void)window;
}
