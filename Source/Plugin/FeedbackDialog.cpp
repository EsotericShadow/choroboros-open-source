#include "FeedbackDialog.h"
#include <juce_gui_basics/juce_gui_basics.h>

FeedbackDialog::FeedbackDialog(FeedbackCollector& collector)
    : feedbackCollector(collector)
{
    setSize(500, 400);
    
    titleLabel.setText("Choroboros Alpha - Feedback", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    infoLabel.setText("Help us improve! Share your thoughts, bug reports, or feature requests.\n"
                      "Usage statistics will be included automatically.",
                      juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(infoLabel);
    
    feedbackText.setMultiLine(true, true);
    feedbackText.setReturnKeyStartsNewLine(true);
    // Note: setPlaceholderText may not be available in all JUCE versions
    // feedbackText.setPlaceholderText("Enter your feedback here...");
    addAndMakeVisible(feedbackText);
    
    saveButton.setButtonText("Save to File");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    emailButton.setButtonText("Open Email");
    emailButton.addListener(this);
    addAndMakeVisible(emailButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);
}

void FeedbackDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);
}

void FeedbackDialog::resized()
{
    auto area = getLocalBounds().reduced(20);
    
    titleLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(10);
    
    infoLabel.setBounds(area.removeFromTop(50));
    area.removeFromTop(10);
    
    feedbackText.setBounds(area.removeFromTop(200));
    area.removeFromTop(20);
    
    auto buttonArea = area.removeFromTop(30);
    saveButton.setBounds(buttonArea.removeFromLeft(120));
    buttonArea.removeFromLeft(10);
    emailButton.setBounds(buttonArea.removeFromLeft(120));
    buttonArea.removeFromLeft(10);
    cancelButton.setBounds(buttonArea.removeFromLeft(120));
}

void FeedbackDialog::buttonClicked(juce::Button* button)
{
    if (button == &saveButton)
    {
        saveFeedback();
    }
    else if (button == &emailButton)
    {
        openEmail();
    }
    else if (button == &cancelButton)
    {
        // Close dialog
        if (auto* parent = getParentComponent())
        {
            parent->removeChildComponent(this);
        }
        delete this;
    }
}

void FeedbackDialog::saveFeedback()
{
    if (feedbackCollector.saveFeedbackToFile(feedbackText.getText()))
    {
        // Close dialog - feedback is saved
        if (auto* parent = getParentComponent())
        {
            parent->removeChildComponent(this);
        }
        delete this;
    }
    // If save failed, dialog stays open so user can try again
}

void FeedbackDialog::openEmail()
{
    juce::String emailBody;
    emailBody << "Choroboros Alpha Feedback\n\n";
    emailBody << feedbackCollector.getUsageSummary() << "\n";
    emailBody << "User Feedback:\n";
    emailBody << feedbackText.getText() << "\n";
    
    juce::String emailLink = "mailto:info@kaizenstrategic.ai?subject=Choroboros%20Alpha%20Feedback&body=";
    emailLink += juce::URL::addEscapeChars(emailBody, false);
    
    juce::URL(emailLink).launchInDefaultBrowser();
    
    if (auto* parent = getParentComponent())
    {
        parent->removeChildComponent(this);
    }
    delete this;
}

void FeedbackDialog::show(FeedbackCollector& collector)
{
    auto* dialog = new FeedbackDialog(collector);
    
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(dialog);
    options.content->setSize(500, 400);
    options.dialogTitle = "Feedback";
    options.dialogBackgroundColour = juce::Colour(0xff2a2a2a);
    options.resizable = false;
    options.useNativeTitleBar = true;
    
    auto* window = options.launchAsync();
    (void)window; // Suppress unused variable warning
}
