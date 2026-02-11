#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

/**
 * Lightweight feedback and usage tracking for alpha version
 * Privacy-conscious: No personal data, only usage patterns
 */
class FeedbackCollector
{
public:
    FeedbackCollector();
    ~FeedbackCollector();
    
    // Track usage events
    void trackEngineSwitch(int engineIndex, bool hq);
    void trackPresetLoad(int presetIndex, const juce::String& presetName);
    void trackSessionStart();
    void trackSessionEnd();
    
    // Get usage summary
    juce::String getUsageSummary() const;
    
    // Save feedback to file
    bool saveFeedbackToFile(const juce::String& feedbackText) const;
    
    // Get feedback file path
    static juce::File getFeedbackDirectory();
    
    // Check if user has opted out (stored in preferences)
    static bool isOptedOut();
    static void setOptedOut(bool optedOut);
    
private:
    struct UsageStats
    {
        int engineGreenCount = 0;
        int engineBlueCount = 0;
        int engineRedCount = 0;
        int enginePurpleCount = 0;
        int hqEnabledCount = 0;
        int presetLoads[6] = {0, 0, 0, 0, 0, 0};
        juce::int64 sessionStartTime = 0;
        juce::int64 totalSessionTime = 0;
        int sessionCount = 0;
    };
    
    UsageStats stats;
    juce::Time sessionStart;
    
    void loadStats();
    void saveStats() const;
    juce::File getStatsFile() const;
};
