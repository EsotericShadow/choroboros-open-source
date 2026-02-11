#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AboutDialog : public juce::Component
{
public:
    AboutDialog();
    ~AboutDialog() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    static void show();
    
private:
    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::Label descriptionLabel;
    juce::Label companyLabel;
    juce::Label dbaLabel;
    juce::Label locationLabel;
    juce::Label copyrightLabel;
    juce::Label contactLabel;
    juce::Label juceLabel;
    juce::TextButton closeButton;
    juce::TextButton licenseButton;
    juce::HyperlinkButton contactLink;
    
    void closeDialog();
    void showLicense();
};
