#include "AboutDialog.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

AboutDialog::AboutDialog()
{
    setSize(450, 550);
    
    titleLabel.setText("Choroboros", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    versionLabel.setText("Version 1.0.1 (Alpha)", juce::dontSendNotification);
    versionLabel.setFont(juce::Font(14.0f));
    versionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(versionLabel);
    
    descriptionLabel.setText("A chorus that eats its own tail\nFour colors, eight algorithms", juce::dontSendNotification);
    descriptionLabel.setFont(juce::Font(14.0f));
    descriptionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(descriptionLabel);
    
    companyLabel.setText("Kaizen Strategic AI Inc", juce::dontSendNotification);
    companyLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    companyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(companyLabel);
    
    dbaLabel.setText("DBA: Green DSP", juce::dontSendNotification);
    dbaLabel.setFont(juce::Font(14.0f));
    dbaLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dbaLabel);
    
    locationLabel.setText("British Columbia, Canada", juce::dontSendNotification);
    locationLabel.setFont(juce::Font(14.0f));
    locationLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(locationLabel);
    
    copyrightLabel.setText("(C) 2026 Kaizen Strategic AI Inc", juce::dontSendNotification);
    copyrightLabel.setFont(juce::Font(12.0f));
    copyrightLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(copyrightLabel);
    
    contactLabel.setText("Info:", juce::dontSendNotification);
    contactLabel.setFont(juce::Font(14.0f));
    contactLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(contactLabel);
    
    contactLink.setButtonText("info@kaizenstrategic.ai");
    contactLink.setURL(juce::URL("mailto:info@kaizenstrategic.ai?subject=Choroboros%20Info"));
    contactLink.setFont(juce::Font(14.0f), false);
    addAndMakeVisible(contactLink);
    
    juceLabel.setText("Built with JUCE 8.0.12", juce::dontSendNotification);
    juceLabel.setFont(juce::Font(11.0f));
    juceLabel.setJustificationType(juce::Justification::centred);
    juceLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(juceLabel);
    
    closeButton.setButtonText("Close");
    closeButton.onClick = [this] { closeDialog(); };
    addAndMakeVisible(closeButton);
    
    licenseButton.setButtonText("View License");
    licenseButton.onClick = [this] { showLicense(); };
    addAndMakeVisible(licenseButton);
}

void AboutDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);
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
