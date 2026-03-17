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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

class RateSyncOverlay : public juce::Component
{
public:
    struct RateEntry
    {
        juce::String label;
        double beatsPerCycle = 0.0;
        double hz = 0.0;
        bool inRange = false;
        bool selected = false;
    };

    RateSyncOverlay();
    ~RateSyncOverlay() override = default;

    void configure(double bpm, double currentHz, double minHz, double maxHz);
    void setAnchorPoint(juce::Point<int> anchor);

    std::function<void(double hz)> onRateSelected;
    std::function<void()> onDismiss;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    double bpm_ = 120.0;

    std::vector<RateEntry> straightItems_;
    std::vector<RateEntry> tripletItems_;
    std::vector<RateEntry> dottedItems_;

    int activeTab_ = 0;
    int hoveredRow_ = -1;

    juce::Point<int> anchorPoint_;
    juce::Rectangle<int> panelBounds_;

    static constexpr int kPanelWidth = 220;
    static constexpr int kHeaderHeight = 24;
    static constexpr int kTabBarHeight = 26;
    static constexpr int kRowHeight = 22;
    static constexpr int kPadding = 6;

    const std::vector<RateEntry>& activeItems() const;
    int panelHeight() const;
    juce::Rectangle<int> headerRect() const;
    juce::Rectangle<int> tabBarRect() const;
    juce::Rectangle<int> tabRect(int tabIndex) const;
    juce::Rectangle<int> rowRect(int rowIndex) const;
    int rowAtPoint(juce::Point<int> p) const;
    int tabAtPoint(juce::Point<int> p) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RateSyncOverlay)
};
