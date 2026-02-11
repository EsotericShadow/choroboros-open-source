#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "FeedbackCollector.h"

class FeedbackDialog : public juce::Component,
                       public juce::Button::Listener
{
public:
    FeedbackDialog(FeedbackCollector& collector);
    ~FeedbackDialog() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    
    static void show(FeedbackCollector& collector);
    
private:
    FeedbackCollector& feedbackCollector;
    juce::TextEditor feedbackText;
    juce::TextButton saveButton;
    juce::TextButton emailButton;
    juce::TextButton cancelButton;
    juce::Label titleLabel;
    juce::Label infoLabel;
    
    void saveFeedback();
    void openEmail();
};
