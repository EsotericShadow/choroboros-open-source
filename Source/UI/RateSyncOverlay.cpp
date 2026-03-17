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

#include "RateSyncOverlay.h"
#include "CustomLookAndFeel.h"

RateSyncOverlay::RateSyncOverlay()
{
    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(true, false);
}

void RateSyncOverlay::configure(double bpm, double currentHz, double minHz, double maxHz)
{
    bpm_ = bpm;

    const auto buildItems = [&](std::vector<RateEntry>& dest,
                                const std::vector<std::pair<juce::String, double>>& defs)
    {
        dest.clear();
        for (const auto& [label, beats] : defs)
        {
            if (beats <= 0.0)
                continue;
            const double hz = bpm / (60.0 * beats);
            const bool inRange = (hz >= minHz && hz <= maxHz);
            const bool selected = std::abs(hz - currentHz) <= 0.01;
            dest.push_back({ label, beats, hz, inRange, selected });
        }
    };

    buildItems(straightItems_,
    {
        { "4 Bars", 16.0 },
        { "2 Bars", 8.0 },
        { "1 Bar",  4.0 },
        { "1/2",    2.0 },
        { "1/4",    1.0 },
        { "1/8",    0.5 },
        { "1/16",   0.25 },
        { "1/32",   0.125 },
        { "1/64",   0.0625 }
    });

    buildItems(tripletItems_,
    {
        { "1/1T",  (4.0 / 1.0)  * (2.0 / 3.0) },
        { "1/2T",  (4.0 / 2.0)  * (2.0 / 3.0) },
        { "1/4T",  (4.0 / 4.0)  * (2.0 / 3.0) },
        { "1/8T",  (4.0 / 8.0)  * (2.0 / 3.0) },
        { "1/16T", (4.0 / 16.0) * (2.0 / 3.0) },
        { "1/32T", (4.0 / 32.0) * (2.0 / 3.0) }
    });

    buildItems(dottedItems_,
    {
        { "1/1.",  (4.0 / 1.0)  * 1.5 },
        { "1/2.",  (4.0 / 2.0)  * 1.5 },
        { "1/4.",  (4.0 / 4.0)  * 1.5 },
        { "1/8.",  (4.0 / 8.0)  * 1.5 },
        { "1/16.", (4.0 / 16.0) * 1.5 },
        { "1/32.", (4.0 / 32.0) * 1.5 }
    });
}

void RateSyncOverlay::setAnchorPoint(juce::Point<int> anchor)
{
    anchorPoint_ = anchor;
}

int RateSyncOverlay::panelHeight() const
{
    const int rows = static_cast<int>(activeItems().size());
    return kHeaderHeight + kTabBarHeight + rows * kRowHeight + kPadding * 2;
}

const std::vector<RateSyncOverlay::RateEntry>& RateSyncOverlay::activeItems() const
{
    if (activeTab_ == 1) return tripletItems_;
    if (activeTab_ == 2) return dottedItems_;
    return straightItems_;
}

juce::Rectangle<int> RateSyncOverlay::headerRect() const
{
    return { panelBounds_.getX(), panelBounds_.getY(), panelBounds_.getWidth(), kHeaderHeight };
}

juce::Rectangle<int> RateSyncOverlay::tabBarRect() const
{
    return { panelBounds_.getX(), panelBounds_.getY() + kHeaderHeight, panelBounds_.getWidth(), kTabBarHeight };
}

juce::Rectangle<int> RateSyncOverlay::tabRect(int tabIndex) const
{
    const auto bar = tabBarRect();
    const int tabW = bar.getWidth() / 3;
    const int x = bar.getX() + tabIndex * tabW;
    const int w = (tabIndex == 2) ? (bar.getRight() - x) : tabW;
    return { x, bar.getY(), w, bar.getHeight() };
}

juce::Rectangle<int> RateSyncOverlay::rowRect(int rowIndex) const
{
    const int y = panelBounds_.getY() + kHeaderHeight + kTabBarHeight + kPadding + rowIndex * kRowHeight;
    return { panelBounds_.getX(), y, panelBounds_.getWidth(), kRowHeight };
}

int RateSyncOverlay::rowAtPoint(juce::Point<int> p) const
{
    const int itemsTop = panelBounds_.getY() + kHeaderHeight + kTabBarHeight + kPadding;
    if (p.y < itemsTop)
        return -1;
    const int row = (p.y - itemsTop) / kRowHeight;
    if (row < 0 || row >= static_cast<int>(activeItems().size()))
        return -1;
    if (p.x < panelBounds_.getX() || p.x >= panelBounds_.getRight())
        return -1;
    return row;
}

int RateSyncOverlay::tabAtPoint(juce::Point<int> p) const
{
    const auto bar = tabBarRect();
    if (!bar.contains(p))
        return -1;
    const int tabW = bar.getWidth() / 3;
    return juce::jlimit(0, 2, (p.x - bar.getX()) / tabW);
}

void RateSyncOverlay::resized()
{
    const int ph = panelHeight();
    int px = anchorPoint_.x - kPanelWidth / 2;
    int py = anchorPoint_.y - ph - 8;

    px = juce::jlimit(4, juce::jmax(4, getWidth() - kPanelWidth - 4), px);
    if (py < 4)
        py = anchorPoint_.y + 8;
    py = juce::jlimit(4, juce::jmax(4, getHeight() - ph - 4), py);

    panelBounds_ = { px, py, kPanelWidth, ph };
}

void RateSyncOverlay::paint(juce::Graphics& g)
{
    auto* claf = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel());

    const auto accent = claf ? claf->getThemeAccentColour() : juce::Colour(0xff7fb8ff);
    const auto panelColour = claf ? claf->getThemePanelColour() : juce::Colour(0xff121417);
    const auto outlineColour = claf ? claf->getThemePanelOutlineColour() : accent.withAlpha(0.82f);
    const auto font = claf ? claf->getPopupMenuFont() : juce::Font(14.0f);

    // Scrim
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRect(getLocalBounds());

    const auto pf = panelBounds_.toFloat();

    // Background gradient
    juce::ColourGradient fill(panelColour.brighter(0.08f), pf.getX(), pf.getY(),
                              panelColour.darker(0.25f).interpolatedWith(accent, 0.08f),
                              pf.getX(), pf.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(pf, 6.0f);

    // Border
    g.setColour(outlineColour.withAlpha(0.92f));
    g.drawRoundedRectangle(pf, 6.0f, 1.15f);

    // Header
    {
        const auto hr = headerRect();
        g.setColour(accent.brighter(0.4f));
        g.setFont(font.boldened());
        g.drawText("Rate Sync @ " + juce::String(bpm_, 2) + " BPM",
                   hr.reduced(10, 0), juce::Justification::centredLeft, true);
    }

    // Separator below header
    {
        const float sepY = static_cast<float>(panelBounds_.getY() + kHeaderHeight);
        g.setColour(accent.withAlpha(0.42f));
        g.drawLine(pf.getX() + 10.0f, sepY, pf.getRight() - 10.0f, sepY, 1.0f);
    }

    // Tab bar
    const juce::String tabLabels[] = { "Straight", "Triplet", "Dotted" };
    for (int i = 0; i < 3; ++i)
    {
        const auto tr = tabRect(i).reduced(3, 3).toFloat();
        if (i == activeTab_)
        {
            g.setColour(accent.withAlpha(0.30f));
            g.fillRoundedRectangle(tr, 4.0f);
            g.setColour(accent.withAlpha(0.65f));
            g.drawRoundedRectangle(tr, 4.0f, 1.0f);
            g.setColour(juce::Colours::white);
        }
        else
        {
            g.setColour(juce::Colours::white.withAlpha(0.60f));
        }
        g.setFont(font.withHeight(font.getHeight() * 0.88f));
        g.drawText(tabLabels[i], tabRect(i), juce::Justification::centred, true);
    }

    // Separator below tabs
    {
        const float sepY = static_cast<float>(panelBounds_.getY() + kHeaderHeight + kTabBarHeight);
        g.setColour(accent.withAlpha(0.30f));
        g.drawLine(pf.getX() + 10.0f, sepY, pf.getRight() - 10.0f, sepY, 1.0f);
    }

    // Item rows
    const auto& items = activeItems();
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        const auto& entry = items[static_cast<size_t>(i)];
        const auto rr = rowRect(i).reduced(4, 1);
        const auto rrf = rr.toFloat();

        const float alpha = entry.inRange ? 1.0f : 0.48f;

        // Hover highlight
        if (i == hoveredRow_ && entry.inRange)
        {
            g.setColour(accent.withAlpha(0.30f));
            g.fillRoundedRectangle(rrf, 4.0f);
            g.setColour(accent.withAlpha(0.65f));
            g.drawRoundedRectangle(rrf, 4.0f, 1.0f);
        }

        // Checkmark for selected
        if (entry.selected)
        {
            const int cx = rr.getX() + 4;
            const int cy = rr.getCentreY();
            juce::Path tick;
            tick.startNewSubPath(static_cast<float>(cx), static_cast<float>(cy));
            tick.lineTo(static_cast<float>(cx + 4), static_cast<float>(cy + 4));
            tick.lineTo(static_cast<float>(cx + 10), static_cast<float>(cy - 4));
            g.setColour(accent.withMultipliedAlpha(alpha));
            g.strokePath(tick, juce::PathStrokeType(2.0f));
        }

        // Label text
        const int textLeft = rr.getX() + 18;
        g.setColour(juce::Colours::white.withMultipliedAlpha(alpha));
        g.setFont(font);
        g.drawText(entry.label, textLeft, rr.getY(), rr.getWidth() / 2, rr.getHeight(),
                   juce::Justification::centredLeft, true);

        // Hz text
        g.setColour(juce::Colours::white.withMultipliedAlpha(alpha * 0.7f));
        g.setFont(font.withHeight(font.getHeight() * 0.85f));
        g.drawText(juce::String(entry.hz, 2) + " Hz",
                   rr.getX(), rr.getY(), rr.getWidth() - 6, rr.getHeight(),
                   juce::Justification::centredRight, true);
    }
}

void RateSyncOverlay::mouseDown(const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (!panelBounds_.contains(pos))
    {
        if (onDismiss)
            onDismiss();
        return;
    }

    // Tab click
    const int tab = tabAtPoint(pos);
    if (tab >= 0 && tab != activeTab_)
    {
        activeTab_ = tab;
        hoveredRow_ = -1;
        resized();
        repaint();
        return;
    }

    // Row click
    const int row = rowAtPoint(pos);
    if (row >= 0)
    {
        const auto& items = activeItems();
        const auto& entry = items[static_cast<size_t>(row)];
        if (entry.inRange)
        {
            if (onRateSelected)
                onRateSelected(entry.hz);
            if (onDismiss)
                onDismiss();
        }
    }
}

void RateSyncOverlay::mouseMove(const juce::MouseEvent& e)
{
    const int newRow = rowAtPoint(e.getPosition());
    if (newRow != hoveredRow_)
    {
        hoveredRow_ = newRow;
        repaint();
    }
}

void RateSyncOverlay::mouseExit(const juce::MouseEvent&)
{
    if (hoveredRow_ != -1)
    {
        hoveredRow_ = -1;
        repaint();
    }
}

bool RateSyncOverlay::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onDismiss)
            onDismiss();
        return true;
    }
    return false;
}
