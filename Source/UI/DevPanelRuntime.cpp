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

#include "DevPanel.h"
#include "../Plugin/PluginEditor.h"
#include "../Plugin/PluginProcessor.h"
#include "DevPanelSupport.h"
#include <cmath>
#include <limits>

using namespace devpanel;

namespace
{
constexpr int kPanelAutoPadding = 26;

bool sectionNameMatchesFilter(const juce::String& sectionName, const juce::StringArray& terms)
{
    if (terms.isEmpty())
        return true;

    const juce::String candidate = sectionName.toLowerCase();
    for (const auto& term : terms)
    {
        if (!candidate.contains(term))
            return false;
    }
    return true;
}

void applySectionFilterToPanel(juce::PropertyPanel& panel, const juce::String& filterText)
{
    const juce::String trimmedFilter = filterText.trim();
    if (trimmedFilter.isEmpty())
        return;

    juce::StringArray filterTerms;
    filterTerms.addTokens(trimmedFilter.toLowerCase(), " \t,;/|:+-_()", "\"'");
    filterTerms.trim();
    filterTerms.removeEmptyStrings();
    if (filterTerms.isEmpty())
        return;

    const auto sectionNames = panel.getSectionNames();
    juce::Array<bool> matches;
    matches.ensureStorageAllocated(sectionNames.size());
    bool hasAnyMatch = false;
    for (const auto& sectionName : sectionNames)
    {
        const bool matchesFilter = sectionNameMatchesFilter(sectionName, filterTerms)
                                || sectionNameMatchesFilter(normalizeSectionHeader(sectionName), filterTerms);
        matches.add(matchesFilter);
        hasAnyMatch = hasAnyMatch || matchesFilter;
    }

    for (int i = 0; i < sectionNames.size(); ++i)
        panel.setSectionOpen(i, hasAnyMatch && matches[i]);
}

void openAllSections(juce::PropertyPanel& panel)
{
    const auto sectionNames = panel.getSectionNames();
    for (int i = 0; i < sectionNames.size(); ++i)
        panel.setSectionOpen(i, true);
}

int rectangleArea(const juce::Rectangle<int>& r)
{
    return juce::jmax(0, r.getWidth()) * juce::jmax(0, r.getHeight());
}
}

int DevPanel::getSelectedSubTab() const
{
    const int safeMainTab = juce::jlimit(0, 6, selectedRightTab);
    const int maxSubTab = juce::jmax(0, getSubTabCount(safeMainTab) - 1);
    return juce::jlimit(0, maxSubTab, selectedSubTabs[static_cast<size_t>(safeMainTab)]);
}

int DevPanel::getSubTabCount(int mainTab) const
{
    switch (juce::jlimit(0, 6, mainTab))
    {
        case 0: return 2; // Overview
        case 1: return 0; // Modulation (unified deck, no secondary tabs)
        case 2: return 2; // Tone
        case 3: return 3; // Engine
        case 4: return 5; // Look & Feel
        case 5: return 3; // Validation
        case 6: return 1; // Settings
        default: break;
    }
    return 1;
}

juce::String DevPanel::getSubTabName(int mainTab, int subTab) const
{
    const int safeMainTab = juce::jlimit(0, 6, mainTab);
    const int safeSubTab = juce::jlimit(0, juce::jmax(0, getSubTabCount(safeMainTab) - 1), subTab);
    switch (safeMainTab)
    {
        case 0:
            switch (safeSubTab)
            {
                case 0: return "Signal Flow";
                case 1: default: return "Delay Visual";
            }
        case 1:
            return "LFO + Trajectory";
        case 2:
            switch (safeSubTab)
            {
                case 0: return "Spectrum";
                case 1: default: return "Engine Response";
            }
        case 3:
        {
            const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
            const bool hq = processor.isHqEnabled();
            juce::String macroLabel;
            switch (engine)
            {
                case 0: macroLabel = "Bloom Macros"; break;
                case 1: macroLabel = "Focus Macros"; break;
                case 2: macroLabel = hq ? "Tape Macros" : "BBD Macros"; break;
                case 3: macroLabel = hq ? "Orbit Macros" : "Warp Macros"; break;
                case 4: macroLabel = hq ? "Ensemble Macros" : "Intensity Macros"; break;
                default: macroLabel = "Engine Macros"; break;
            }
            switch (safeSubTab)
            {
                case 0: return "Signal Flow";
                case 1: return "Engine Identity";
                case 2: default: return macroLabel;
            }
        }
        case 4:
            switch (safeSubTab)
            {
                case 0: return "Mapping";
                case 1: return "UI Feel";
                case 2: return "Engine Layout";
                case 3: return "Global Layout";
                case 4: default: return "Text Animations";
            }
        case 5:
            switch (safeSubTab)
            {
                case 0: return "Telemetry";
                case 1: return "Trace Matrix";
                case 2: default: return "Console";
            }
        case 6:
            switch (safeSubTab)
            {
                case 0: default: return "Actions";
            }
        default: break;
    }
    return "View";
}

juce::PropertyPanel* DevPanel::getActiveEngineLayoutPanel()
{
    const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    switch (engine)
    {
        case 0: return &layoutGreenPanel;
        case 1: return &layoutBluePanel;
        case 2: return &layoutRedPanel;
        case 3: return &layoutPurplePanel;
        case 4: default: return &layoutBlackPanel;
    }
}

juce::PropertyPanel* DevPanel::getActiveEngineInternalsPanel()
{
    const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    const bool hqEnabled = processor.isHqEnabled();
    switch (engine)
    {
        case 0: return hqEnabled ? &internalsGreenHqPanel : &internalsGreenNqPanel;
        case 1: return hqEnabled ? &internalsBlueHqPanel : &internalsBlueNqPanel;
        case 2: return hqEnabled ? &internalsRedHqPanel : &internalsRedNqPanel;
        case 3: return hqEnabled ? &internalsPurpleHqPanel : &internalsPurpleNqPanel;
        case 4: default: return hqEnabled ? &internalsBlackHqPanel : &internalsBlackNqPanel;
    }
}

void DevPanel::refreshSecondaryTabButtons()
{
    juce::TextButton* buttons[5] = { &subTabButtonA, &subTabButtonB, &subTabButtonC, &subTabButtonD, &subTabButtonE };
    const int safeMainTab = juce::jlimit(0, 6, selectedRightTab);
    const int subTabCount = getSubTabCount(safeMainTab);
    selectedSubTabs[static_cast<size_t>(safeMainTab)] =
        juce::jlimit(0, juce::jmax(0, subTabCount - 1), selectedSubTabs[static_cast<size_t>(safeMainTab)]);
    const int selectedSubTab = getSelectedSubTab();

    for (int i = 0; i < 5; ++i)
    {
        auto* button = buttons[i];
        const bool visible = i < subTabCount;
        button->setVisible(visible);
        if (!visible)
        {
            button->setToggleState(false, juce::dontSendNotification);
            continue;
        }
        button->setButtonText(getSubTabName(safeMainTab, i));
        button->setToggleState(i == selectedSubTab, juce::dontSendNotification);
        styleHackerTextButton(*button, i == selectedSubTab);
        button->setTooltip("Subview: " + getSubTabName(safeMainTab, i));
    }
}

void DevPanel::resized()
{
    const auto previousViewPos = viewport.getViewPosition();
    auto bounds = getLocalBounds().reduced(18);
    viewport.setBounds(bounds);

    constexpr int contentInsetLeft = 16;
    constexpr int contentInsetRight = 16;
    constexpr int contentInsetTop = 14;
    constexpr int contentInsetBottom = 16;
    const int columnGap = 28;
    const int usableContentWidth = juce::jmax(320, bounds.getWidth() - viewport.getScrollBarThickness() - contentInsetLeft - contentInsetRight);
    const int contentWidth = usableContentWidth + contentInsetLeft + contentInsetRight;
    const float leftColumnRatio = usableContentWidth >= 1500 ? 0.29f : 0.31f;
    int leftColumnWidth = juce::jlimit(260, 430, static_cast<int>(std::round(usableContentWidth * leftColumnRatio)));
    int rightColumnWidth = usableContentWidth - columnGap - leftColumnWidth;
    constexpr int minRightColumnWidth = 540;
    if (rightColumnWidth < minRightColumnWidth)
    {
        const int widthDeficit = minRightColumnWidth - rightColumnWidth;
        leftColumnWidth = juce::jmax(260, leftColumnWidth - widthDeficit);
        rightColumnWidth = usableContentWidth - columnGap - leftColumnWidth;
    }
    const int leftX = contentInsetLeft;
    const int rightX = leftColumnWidth + columnGap;
    const int rightColumnX = leftX + rightX;

    struct SectionSpacing
    {
        int titleHeight = 34;
        int descriptionHeight = 24;
        int titleToDescriptionGap = 0;
        int descriptionToPanelGap = 4;
        int sectionGap = 30;
    };

    const SectionSpacing compactSection { 30, 20, 0, 4, 22 };
    const SectionSpacing visualSection { 36, 24, 2, 8, 22 };

    struct DeckSpacing
    {
        int deckPadding = 12;
        int cardGap = 12;
        int afterDeckGap = 28;
    };

    const bool wideRightColumn = rightColumnWidth >= 980;
    const DeckSpacing modulationDeckSpacing = wideRightColumn ? DeckSpacing { 12, 10, 24 }
                                                              : DeckSpacing { 10, 8, 20 };
    const DeckSpacing overviewDeckSpacing = wideRightColumn ? DeckSpacing { 12, 10, 24 }
                                                            : DeckSpacing { 10, 8, 20 };
    const DeckSpacing toneDeckSpacing = wideRightColumn ? DeckSpacing { 16, 14, 28 }
                                                        : DeckSpacing { 12, 10, 22 };
    const DeckSpacing engineDeckSpacing = wideRightColumn ? DeckSpacing { 15, 13, 26 }
                                                          : DeckSpacing { 12, 10, 22 };
    const DeckSpacing lookFeelDeckSpacing = wideRightColumn ? DeckSpacing { 14, 12, 24 }
                                                            : DeckSpacing { 12, 10, 20 };

    auto layoutSectionHeader = [&](int x, int width, int& y, juce::Label& title, juce::Label& description, const SectionSpacing& spacing)
    {
        title.setBounds(x, y, width, spacing.titleHeight);
        y += spacing.titleHeight + spacing.titleToDescriptionGap;
        description.setBounds(x, y, width, spacing.descriptionHeight);
        y += spacing.descriptionHeight + spacing.descriptionToPanelGap;
    };

    auto layoutPanelOnly = [&](int x, int width, int& y, juce::PropertyPanel& panel, const SectionSpacing& spacing)
    {
        const int panelHeight = panel.getTotalContentHeight() + kPanelAutoPadding;
        panel.setBounds(x, y, width, panelHeight);
        y += panelHeight + spacing.sectionGap;
    };

    auto layoutVisualDeck = [&](int x, int width, int& y,
                                VisualDeckContent& deck,
                                const juce::Array<juce::PropertyComponent*>& cards,
                                const juce::Array<juce::PropertyComponent*>& allCards,
                                const DeckSpacing& spacing)
    {
        for (auto* card : allCards)
        {
            if (card == nullptr)
                continue;
            card->setVisible(false);
            card->setBounds(0, 0, 0, 0);
        }

        int cardsHeight = 0;
        for (int i = 0; i < cards.size(); ++i)
        {
            if (auto* card = cards[i])
            {
                card->setVisible(true);
                cardsHeight += card->getPreferredHeight();
                if (i < cards.size() - 1)
                    cardsHeight += spacing.cardGap;
            }
        }

        if (cardsHeight <= 0)
        {
            deck.setBounds(0, 0, 0, 0);
            return;
        }

        const int deckHeight = juce::jmax(0, cardsHeight + spacing.deckPadding * 2);
        deck.setBounds(x, y, width, deckHeight);

        int cy = spacing.deckPadding;
        for (auto* card : cards)
        {
            if (card == nullptr)
                continue;
            const int cardHeight = card->getPreferredHeight();
            card->setBounds(spacing.deckPadding, cy, juce::jmax(40, width - spacing.deckPadding * 2), cardHeight);
            cy += cardHeight + spacing.cardGap;
        }
        y += deckHeight + spacing.afterDeckGap;
    };

    auto layoutModulationDeck = [&](int x, int width, int& y, const DeckSpacing& spacing)
    {
        auto findRoleCard = [&](const juce::String& role) -> juce::PropertyComponent*
        {
            for (auto* card : modulationVisualDeckCards)
            {
                if (card == nullptr)
                    continue;
                if (!card->getProperties().contains("devpanelModRole"))
                    continue;
                if (card->getProperties()["devpanelModRole"].toString() == role)
                    return card;
            }
            return nullptr;
        };

        for (auto* card : modulationVisualDeckCards)
        {
            if (card == nullptr)
                continue;
            card->setVisible(false);
            card->setBounds(0, 0, 0, 0);
        }

        auto* lfoLeftCard = findRoleCard("lfo_left");
        auto* lfoRightCard = findRoleCard("lfo_right");
        auto* trajectoryCard = findRoleCard("trajectory");
        auto* workbenchCard = findRoleCard("lfo_workbench");

        if (lfoLeftCard == nullptr && modulationVisualDeckCards.size() > 0)
            lfoLeftCard = modulationVisualDeckCards[0];
        if (lfoRightCard == nullptr && modulationVisualDeckCards.size() > 1)
            lfoRightCard = modulationVisualDeckCards[1];
        if (trajectoryCard == nullptr && modulationVisualDeckCards.size() > 2)
            trajectoryCard = modulationVisualDeckCards[2];
        if (workbenchCard == nullptr && modulationVisualDeckCards.size() > 3)
            workbenchCard = modulationVisualDeckCards[3];

        if (lfoLeftCard == nullptr && lfoRightCard == nullptr
            && trajectoryCard == nullptr && workbenchCard == nullptr)
        {
            modulationVisualDeck.setBounds(0, 0, 0, 0);
            return;
        }

        const int innerWidth = juce::jmax(40, width - spacing.deckPadding * 2);
        const int rowGap = juce::jmax(8, spacing.cardGap);
        int rowTopY = spacing.deckPadding;
        int topRowHeight = 0;
        int trajectoryHeight = 0;
        int workbenchHeight = 0;

        if (lfoLeftCard != nullptr || lfoRightCard != nullptr)
        {
            const int leftHeight = lfoLeftCard != nullptr ? lfoLeftCard->getPreferredHeight() : 0;
            const int rightHeight = lfoRightCard != nullptr ? lfoRightCard->getPreferredHeight() : 0;
            topRowHeight = juce::jmax(leftHeight, rightHeight);
            rowTopY += topRowHeight + rowGap;
        }

        if (trajectoryCard != nullptr)
        {
            trajectoryHeight = trajectoryCard->getPreferredHeight();
            rowTopY += trajectoryHeight + rowGap;
        }

        if (workbenchCard != nullptr)
        {
            workbenchHeight = workbenchCard->getPreferredHeight();
            rowTopY += workbenchHeight + rowGap;
        }

        const int contentHeight = rowTopY - rowGap;
        const int deckHeight = juce::jmax(0, contentHeight + spacing.deckPadding);
        modulationVisualDeck.setBounds(x, y, width, deckHeight);

        int cy = spacing.deckPadding;

        if (lfoLeftCard != nullptr || lfoRightCard != nullptr)
        {
            const int sharedHeight = topRowHeight;
            if (lfoLeftCard != nullptr && lfoRightCard != nullptr)
            {
                const int twoUpGap = juce::jmax(10, rowGap);
                const int leftWidth = juce::jmax(80, (innerWidth - twoUpGap) / 2);
                const int rightWidth = juce::jmax(80, innerWidth - leftWidth - twoUpGap);
                lfoLeftCard->setVisible(true);
                lfoRightCard->setVisible(true);
                lfoLeftCard->setBounds(spacing.deckPadding, cy, leftWidth, sharedHeight);
                lfoRightCard->setBounds(spacing.deckPadding + leftWidth + twoUpGap, cy, rightWidth, sharedHeight);
            }
            else if (lfoLeftCard != nullptr)
            {
                lfoLeftCard->setVisible(true);
                lfoLeftCard->setBounds(spacing.deckPadding, cy, innerWidth, sharedHeight);
            }
            else if (lfoRightCard != nullptr)
            {
                lfoRightCard->setVisible(true);
                lfoRightCard->setBounds(spacing.deckPadding, cy, innerWidth, sharedHeight);
            }
            cy += sharedHeight + rowGap;
        }

        if (trajectoryCard != nullptr)
        {
            int trajectoryWidth = innerWidth;
            if (innerWidth > 560)
                trajectoryWidth = juce::roundToInt(static_cast<float>(innerWidth) * 0.76f);
            trajectoryWidth = juce::jlimit(220, innerWidth, trajectoryWidth);
            const int trajectoryX = spacing.deckPadding + (innerWidth - trajectoryWidth) / 2;
            trajectoryCard->setVisible(true);
            trajectoryCard->setBounds(trajectoryX, cy, trajectoryWidth, trajectoryHeight);
            cy += trajectoryHeight + rowGap;
        }

        if (workbenchCard != nullptr)
        {
            workbenchCard->setVisible(true);
            workbenchCard->setBounds(spacing.deckPadding, cy, innerWidth, workbenchHeight);
        }

        y += deckHeight + spacing.afterDeckGap;
    };

    int leftY = contentInsetTop;
    int rightY = contentInsetTop;
    const int selectedSubTab = getSelectedSubTab();

    const int tabHeight = 38;
    const int tabGap = 6;
    const int tabButtonWidth = juce::jmax(82, (rightColumnWidth - (tabGap * 6)) / 7);
    int tabX = rightColumnX;
    tabOverviewButton.setBounds(tabX, rightY, tabButtonWidth, tabHeight);
    tabX += tabButtonWidth + tabGap;
    tabInternalsButton.setBounds(tabX, rightY, tabButtonWidth, tabHeight);
    tabX += tabButtonWidth + tabGap;
    tabBbdButton.setBounds(tabX, rightY, tabButtonWidth, tabHeight);
    tabX += tabButtonWidth + tabGap;
    tabTapeButton.setBounds(tabX, rightY, tabButtonWidth, tabHeight);
    tabX += tabButtonWidth + tabGap;
    tabLayoutButton.setBounds(tabX, rightY, tabButtonWidth, tabHeight);
    tabX += tabButtonWidth + tabGap;
    tabValidationButton.setBounds(tabX, rightY, tabButtonWidth, tabHeight);
    tabX += tabButtonWidth + tabGap;
    tabSettingsButton.setBounds(tabX, rightY, tabButtonWidth, tabHeight);
    rightY += tabHeight + 8;

    refreshSecondaryTabButtons();
    const int subTabHeight = 30;
    const int subTabGap = 6;
    const int subTabCount = getSubTabCount(selectedRightTab);
    juce::TextButton* subTabButtons[5] = { &subTabButtonA, &subTabButtonB, &subTabButtonC, &subTabButtonD, &subTabButtonE };
    if (subTabCount > 0)
    {
        const int subTabButtonWidth = juce::jmax(108, (rightColumnWidth - (subTabGap * juce::jmax(0, subTabCount - 1)))
                                                       / juce::jmax(1, subTabCount));
        int subTabX = rightColumnX;
        for (int i = 0; i < 5; ++i)
        {
            if (i < subTabCount)
            {
                subTabButtons[i]->setBounds(subTabX, rightY, subTabButtonWidth, subTabHeight);
                subTabX += subTabButtonWidth + subTabGap;
            }
            else
            {
                subTabButtons[i]->setBounds(0, 0, 0, 0);
            }
        }
        rightY += subTabHeight + 14;
    }
    else
    {
        for (auto* button : subTabButtons)
            button->setBounds(0, 0, 0, 0);
        rightY += 2;
    }

    const int activeProfileH = 26;
    activeProfileLabel.setBounds(rightColumnX, rightY, rightColumnWidth, activeProfileH);
    rightY += activeProfileH + 4;
    const int scopeHintH = 22;
    activeScopeHintLabel.setBounds(rightColumnX, rightY, rightColumnWidth, scopeHintH);
    rightY += scopeHintH + 12;

    const int toolsH = 32;
    const int modeLabelW = 48;
    const int modeComboW = 136;
    const int modeHqW = 64;
    const int modeGap = 8;
    devEngineModeLabel.setBounds(rightColumnX, rightY + 4, modeLabelW, toolsH - 4);
    devEngineModeBox.setBounds(rightColumnX + modeLabelW + modeGap, rightY, modeComboW, toolsH);
    devHqModeToggle.setBounds(rightColumnX + modeLabelW + modeGap + modeComboW + modeGap, rightY, modeHqW, toolsH);
    rightY += toolsH + 14;

    const bool showRightTools = selectedRightTab == 3;
    if (showRightTools)
    {
        const int labelW = 126;
        const int clearW = 76;
        const int toolsGap = 8;
        const int editorX = rightColumnX + labelW + toolsGap;
        const int editorRight = rightColumnX + rightColumnWidth - clearW - toolsGap;
        const int editorW = juce::jmax(140, editorRight - editorX);
        engineFilterLabel.setBounds(rightColumnX, rightY + 4, labelW, toolsH - 4);
        engineFilterEditor.setBounds(editorX, rightY, editorW, toolsH);
        engineFilterClearButton.setBounds(rightColumnX + rightColumnWidth - clearW, rightY, clearW, toolsH);
        rightY += toolsH + 18;
    }
    else
    {
        engineFilterLabel.setBounds(0, 0, 0, 0);
        engineFilterEditor.setBounds(0, 0, 0, 0);
        engineFilterClearButton.setBounds(0, 0, 0, 0);
    }

    auto selectedDeckCards = [this](const juce::Array<juce::PropertyComponent*>& source, int selectedIndex)
    {
        juce::Array<juce::PropertyComponent*> result;
        bool usedTaggedSelection = false;
        const int activeEngineIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const int activeHqMode = processor.isHqEnabled() ? 1 : 0;
        for (auto* card : source)
        {
            if (card == nullptr)
                continue;
            if (!card->getProperties().contains("devpanelSubTab"))
                continue;
            usedTaggedSelection = true;
            const int cardSubTab = static_cast<int>(card->getProperties()["devpanelSubTab"]);
            if (cardSubTab != selectedIndex)
                continue;
            if (card->getProperties().contains("devpanelEngineIndex"))
            {
                const int cardEngine = static_cast<int>(card->getProperties()["devpanelEngineIndex"]);
                if (cardEngine != activeEngineIndex)
                    continue;
            }
            if (card->getProperties().contains("devpanelHqMode"))
            {
                const int cardHqMode = static_cast<int>(card->getProperties()["devpanelHqMode"]);
                if (cardHqMode >= 0 && cardHqMode != activeHqMode)
                    continue;
            }
            result.add(card);
        }

        if (!usedTaggedSelection)
        {
            if (juce::isPositiveAndBelow(selectedIndex, source.size()))
                result.add(source[selectedIndex]);
        }
        return result;
    };

    // Inspector (left column): controls/readouts for current tab.
    juce::String inspectorScope;
    switch (selectedRightTab)
    {
        case 0: inspectorScope = "Overview / " + getSubTabName(0, selectedSubTab); break;
        case 1: inspectorScope = "Modulation / " + getSubTabName(1, selectedSubTab); break;
        case 2: inspectorScope = "Tone / " + getSubTabName(2, selectedSubTab); break;
        case 3: inspectorScope = "Engine / " + getSubTabName(3, selectedSubTab); break;
        case 4: inspectorScope = "Look & Feel / " + getSubTabName(4, selectedSubTab); break;
        case 5: inspectorScope = "Validation / " + getSubTabName(5, selectedSubTab); break;
        case 6: inspectorScope = "Settings / " + getSubTabName(6, selectedSubTab); break;
        default: inspectorScope = "Overview / " + getSubTabName(0, selectedSubTab); break;
    }

    inspectorTitle.setText("Inspector", juce::dontSendNotification);
    inspectorDescription.setText(inspectorScope + " controls/readouts. Use sections to edit values.",
                                 juce::dontSendNotification);
    const int inspectorTitleH = 30;
    const int inspectorDescH = 22;
    inspectorTitle.setBounds(leftX, leftY, leftColumnWidth, inspectorTitleH);
    leftY += inspectorTitleH;
    inspectorDescription.setBounds(leftX, leftY, leftColumnWidth, inspectorDescH);
    leftY += inspectorDescH + 8;

    if (selectedRightTab == 0)
    {
        layoutPanelOnly(leftX, leftColumnWidth, leftY, overviewPanel, compactSection);
    }
    else if (selectedRightTab == 1)
    {
        layoutPanelOnly(leftX, leftColumnWidth, leftY, modulationPanel, compactSection);
    }
    else if (selectedRightTab == 2)
    {
        layoutPanelOnly(leftX, leftColumnWidth, leftY, tonePanel, compactSection);
    }
    else if (selectedRightTab == 3)
    {
        layoutPanelOnly(leftX, leftColumnWidth, leftY, enginePanel, compactSection);
    }
    else if (selectedRightTab == 4)
    {
        if (selectedSubTab == 0)
            layoutPanelOnly(leftX, leftColumnWidth, leftY, mappingPanel, compactSection);
        else if (selectedSubTab == 1)
            layoutPanelOnly(leftX, leftColumnWidth, leftY, uiPanel, compactSection);
        else if (selectedSubTab == 2)
        {
            if (auto* activeEnginePanel = getActiveEngineLayoutPanel())
                layoutPanelOnly(leftX, leftColumnWidth, leftY, *activeEnginePanel, compactSection);
        }
        else if (selectedSubTab == 3)
            layoutPanelOnly(leftX, leftColumnWidth, leftY, layoutGlobalPanel, compactSection);
        else if (selectedSubTab == 4)
            layoutPanelOnly(leftX, leftColumnWidth, leftY, layoutTextAnimationPanel, compactSection);
    }
    else if (selectedRightTab == 5)
    {
        layoutPanelOnly(leftX, leftColumnWidth, leftY, validationPanel, compactSection);
    }
    else if (selectedRightTab == 6)
    {
        layoutPanelOnly(leftX, leftColumnWidth, leftY, settingsPanel, compactSection);
    }

    if (selectedRightTab == 0)
    {
        layoutSectionHeader(rightColumnX, rightColumnWidth, rightY, overviewTitle, overviewDescription, visualSection);
        auto cards = selectedDeckCards(overviewVisualDeckCards, selectedSubTab);
        layoutVisualDeck(rightColumnX, rightColumnWidth, rightY, overviewVisualDeck, cards, overviewVisualDeckCards, overviewDeckSpacing);
    }
    else if (selectedRightTab == 1)
    {
        layoutSectionHeader(rightColumnX, rightColumnWidth, rightY, modulationTitle, modulationDescription, visualSection);
        layoutModulationDeck(rightColumnX, rightColumnWidth, rightY, modulationDeckSpacing);
    }
    else if (selectedRightTab == 2)
    {
        layoutSectionHeader(rightColumnX, rightColumnWidth, rightY, toneTitle, toneDescription, visualSection);
        auto cards = selectedDeckCards(toneVisualDeckCards, selectedSubTab);
        layoutVisualDeck(rightColumnX, rightColumnWidth, rightY, toneVisualDeck, cards, toneVisualDeckCards, toneDeckSpacing);
    }
    else if (selectedRightTab == 3)
    {
        layoutSectionHeader(rightColumnX, rightColumnWidth, rightY, engineTitle, engineDescription, visualSection);
        juce::Array<juce::PropertyComponent*> engineCards;
        auto appendUnique = [&engineCards](const juce::Array<juce::PropertyComponent*>& source)
        {
            for (auto* card : source)
            {
                if (card == nullptr)
                    continue;
                if (!engineCards.contains(card))
                    engineCards.add(card);
            }
        };

        // Keep signal flow visible; show identity/macros only when their subtab is selected.
        appendUnique(selectedDeckCards(engineVisualDeckCards, 0));
        appendUnique(selectedDeckCards(engineVisualDeckCards, selectedSubTab));
        layoutVisualDeck(rightColumnX, rightColumnWidth, rightY, engineVisualDeck, engineCards, engineVisualDeckCards, engineDeckSpacing);

        if (auto* activeInternalsPanel = getActiveEngineInternalsPanel())
            layoutPanelOnly(rightColumnX, rightColumnWidth, rightY, *activeInternalsPanel, compactSection);
        if (bbdPanel.isVisible())
            layoutPanelOnly(rightColumnX, rightColumnWidth, rightY, bbdPanel, compactSection);
        if (tapePanel.isVisible())
            layoutPanelOnly(rightColumnX, rightColumnWidth, rightY, tapePanel, compactSection);
    }
    else if (selectedRightTab == 4)
    {
        layoutSectionHeader(rightColumnX, rightColumnWidth, rightY, layoutTitle, layoutDescription, visualSection);
        auto cards = selectedDeckCards(lookFeelVisualDeckCards, 0);
        layoutVisualDeck(rightColumnX, rightColumnWidth, rightY, lookFeelVisualDeck, cards, lookFeelVisualDeckCards, lookFeelDeckSpacing);
    }
    else if (selectedRightTab == 5)
    {
        layoutSectionHeader(rightColumnX, rightColumnWidth, rightY, validationTitle, validationDescription, visualSection);
        auto cards = selectedDeckCards(validationVisualDeckCards, selectedSubTab);
        layoutVisualDeck(rightColumnX, rightColumnWidth, rightY, validationVisualDeck, cards, validationVisualDeckCards, lookFeelDeckSpacing);
    }
    else if (selectedRightTab == 6)
    {
        layoutSectionHeader(rightColumnX, rightColumnWidth, rightY, settingsTitle, settingsDescription, visualSection);

        const int buttonHeight = 38;
        const int buttonGap = 12;
        const int firstRowWidth = (rightColumnWidth - (buttonGap * 2)) / 3;
        int rowY = rightY;
        int rowX = rightColumnX;
        fxPresetOffButton.setBounds(rowX, rowY, firstRowWidth, buttonHeight);
        rowX += firstRowWidth + buttonGap;
        fxPresetSubtleButton.setBounds(rowX, rowY, firstRowWidth, buttonHeight);
        rowX += firstRowWidth + buttonGap;
        fxPresetMediumButton.setBounds(rowX, rowY, rightColumnX + rightColumnWidth - rowX, buttonHeight);

        rowY += buttonHeight + 14;
        const int secondRowWidth = (rightColumnWidth - (buttonGap * 3)) / 4;
        rowX = rightColumnX;
        lockToggleButton.setBounds(rowX, rowY, secondRowWidth, buttonHeight);
        rowX += secondRowWidth + buttonGap;
        resetFactoryButton.setBounds(rowX, rowY, secondRowWidth, buttonHeight);
        rowX += secondRowWidth + buttonGap;
        saveDefaultsButton.setBounds(rowX, rowY, secondRowWidth, buttonHeight);
        rowX += secondRowWidth + buttonGap;
        copyJsonButton.setBounds(rowX, rowY, rightColumnX + rightColumnWidth - rowX, buttonHeight);

        rightY = rowY + buttonHeight + 16;
    }

    if (selectedRightTab != 6)
    {
        fxPresetOffButton.setBounds(0, 0, 0, 0);
        fxPresetSubtleButton.setBounds(0, 0, 0, 0);
        fxPresetMediumButton.setBounds(0, 0, 0, 0);
        lockToggleButton.setBounds(0, 0, 0, 0);
        resetFactoryButton.setBounds(0, 0, 0, 0);
        saveDefaultsButton.setBounds(0, 0, 0, 0);
        copyJsonButton.setBounds(0, 0, 0, 0);
    }

    const int contentHeight = std::max(leftY, rightY) + contentInsetBottom;
    content.setBounds(0, 0, contentWidth, contentHeight);

    const int maxViewX = juce::jmax(0, content.getWidth() - viewport.getViewWidth());
    const int maxViewY = juce::jmax(0, content.getHeight() - viewport.getViewHeight());
    viewport.setViewPosition(juce::jlimit(0, maxViewX, previousViewPos.x),
                             juce::jlimit(0, maxViewY, previousViewPos.y));

    if (tutorialActive)
    {
        layoutTutorialOverlay();
        updateTutorialHighlight();
        tutorialFocusHighlight.toFront(false);
        tutorialOverlay.toFront(false);
    }
    else
    {
        tutorialOverlay.setVisible(false);
        tutorialFocusHighlight.setVisible(false);
    }
}

void DevPanel::updateActiveProfileLabel()
{
    static const juce::String engineNames[] { "Green", "Blue", "Red", "Purple", "Black" };
    const int colorIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    const bool hqEnabled = processor.isHqEnabled();
    setCurrentEngineSkinColour(colorIndex);
    getDevPanelThemeLookAndFeel().refreshThemeColours();
    getDevPanelSectionLookAndFeel().refreshThemeColours();
    sendLookAndFeelChange();
    const juce::Colour profileAccent = hackerText();
    const juce::String profileName = engineNames[colorIndex] + (hqEnabled ? " HQ" : " NQ");
    suppressDevModeControlCallbacks = true;
    devEngineModeBox.setSelectedId(colorIndex + 1, juce::dontSendNotification);
    devHqModeToggle.setToggleState(hqEnabled, juce::dontSendNotification);
    suppressDevModeControlCallbacks = false;

    activeProfileLabel.setColour(juce::Label::textColourId, profileAccent);
    devEngineModeLabel.setColour(juce::Label::textColourId, hackerTextDim());
    const int selectorAccentIndex = (settingsAccentSource == 1)
        ? juce::jlimit(0, 4, settingsManualAccent)
        : colorIndex;
    styleProfileSelectorComboBox(devEngineModeBox, profileSelectorColourForEngineIndex(selectorAccentIndex));
    styleHackerToggleButton(devHqModeToggle);
    styleHackerTextButton(copyJsonButton, false);
    styleHackerTextButton(saveDefaultsButton, false);
    styleHackerTextButton(resetFactoryButton, false);
    styleHackerTextButton(lockToggleButton, false);
    styleHackerTextButton(fxPresetOffButton, false);
    styleHackerTextButton(fxPresetSubtleButton, false);
    styleHackerTextButton(fxPresetMediumButton, false);
    styleHackerTextButton(engineFilterClearButton, false);
    styleHackerTextButton(tutorialNextButton, false);
    styleHackerTextButton(tutorialNextSectionButton, false);
    styleHackerTextButton(tutorialSkipButton, false);
    styleHackerEditor(engineFilterEditor);

    activeProfileLabel.setText("Active Profile: " + profileName,
                               juce::dontSendNotification);

    juce::String scopeText;
    const juce::String subView = getSubTabName(selectedRightTab, getSelectedSubTab());
    switch (selectedRightTab)
    {
        case 0:
            scopeText = "Now in Overview / " + subView + ": check mapped values and signal flow for " + profileName + ".";
            break;
        case 1:
            scopeText = "Now in Modulation / " + subView + ": controls write to " + profileName + ".";
            break;
        case 2:
            scopeText = "Now in Tone / " + subView + ": analyzer cards and tone controls target " + profileName + ".";
            break;
        case 3:
            scopeText = "Now in Engine / " + subView + ": edits write to " + profileName + ".";
            break;
        case 4:
            scopeText = "Now in Look & Feel / " + subView + ": UI/editor tuning only (no DSP profile write).";
            break;
        case 5:
            scopeText = "Now in Validation / " + subView + ": read-only telemetry and wiring checks for " + profileName + ".";
            break;
        case 6:
            scopeText = "Now in Settings / " + subView + ": manage Dev Panel actions, defaults, and JSON export.";
            break;
        default:
            scopeText = "Now in Overview / " + subView + ": check mapped values and signal flow for " + profileName + ".";
            break;
    }

    activeScopeHintLabel.setColour(juce::Label::textColourId, hackerTextDim());
    activeScopeHintLabel.setText(scopeText, juce::dontSendNotification);
    activeScopeHintLabel.setTooltip(scopeText + " Switch Profile or HQ above to retarget context.");
}

void DevPanel::updateEngineSectionVisibility()
{
    const int colorIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    const bool hqEnabled = processor.isHqEnabled();
    const bool showEngine = selectedRightTab == 3;
    setActiveEngineSectionPrefix({}, false);
    const bool isRedNQ = colorIndex == 2 && !hqEnabled;
    const bool isRedHQ = colorIndex == 2 && hqEnabled;

    const auto setProfilePanelVisible = [](juce::PropertyPanel& panel, bool visible)
    {
        panel.setVisible(visible);
        if (!visible)
        {
            const auto sections = panel.getSectionNames();
            for (int i = 0; i < sections.size(); ++i)
                panel.setSectionOpen(i, false);
        }
    };

    juce::PropertyPanel* activePanel = showEngine ? getActiveEngineInternalsPanel() : nullptr;
    setProfilePanelVisible(internalsGreenNqPanel, activePanel == &internalsGreenNqPanel);
    setProfilePanelVisible(internalsGreenHqPanel, activePanel == &internalsGreenHqPanel);
    setProfilePanelVisible(internalsBlueNqPanel, activePanel == &internalsBlueNqPanel);
    setProfilePanelVisible(internalsBlueHqPanel, activePanel == &internalsBlueHqPanel);
    setProfilePanelVisible(internalsRedNqPanel, activePanel == &internalsRedNqPanel);
    setProfilePanelVisible(internalsRedHqPanel, activePanel == &internalsRedHqPanel);
    setProfilePanelVisible(internalsPurpleNqPanel, activePanel == &internalsPurpleNqPanel);
    setProfilePanelVisible(internalsPurpleHqPanel, activePanel == &internalsPurpleHqPanel);
    setProfilePanelVisible(internalsBlackNqPanel, activePanel == &internalsBlackNqPanel);
    setProfilePanelVisible(internalsBlackHqPanel, activePanel == &internalsBlackHqPanel);

    bbdPanel.setVisible(showEngine && isRedNQ);
    tapePanel.setVisible(showEngine && isRedHQ);

    const juce::String sectionFilter = engineFilterEditor.getText().trim();
    if (showEngine && sectionFilter.isNotEmpty())
    {
        applySectionFilterToPanel(enginePanel, sectionFilter);
        if (activePanel != nullptr)
            applySectionFilterToPanel(*activePanel, sectionFilter);
        if (bbdPanel.isVisible())
            applySectionFilterToPanel(bbdPanel, sectionFilter);
        if (tapePanel.isVisible())
            applySectionFilterToPanel(tapePanel, sectionFilter);
    }
}

void DevPanel::updateRightTabVisibility()
{
    const bool showOverview = selectedRightTab == 0;
    const bool showModulation = selectedRightTab == 1;
    const bool showTone = selectedRightTab == 2;
    const bool showEngine = selectedRightTab == 3;
    const bool showLookFeel = selectedRightTab == 4;
    const bool showValidation = selectedRightTab == 5;
    const bool showSettings = selectedRightTab == 6;
    const int selectedSubTab = getSelectedSubTab();
    refreshSecondaryTabButtons();

    const int activeEngineIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    setCurrentEngineSkinColour(activeEngineIndex);
    if (!(showOverview || showModulation || showTone || showEngine))
        setLinkedGroup(LinkGroup::none);
    const juce::Colour overviewAccent = visualOverview();
    const juce::Colour modulationAccent = visualModulation();
    const juce::Colour toneAccent = visualTone();
    const juce::Colour engineAccent = visualEngine();
    const juce::Colour validationAccent = visualValidation();
    const juce::Colour layoutAccent = visualLayout();
    const juce::Colour inactiveTitle = hackerTextDim();
    const juce::Colour inactiveDesc = hackerTextMuted();

    auto applySectionTheme = [&](juce::Label& title, juce::Label& description, bool active, juce::Colour)
    {
        title.setColour(juce::Label::textColourId, active ? hackerText() : inactiveTitle);
        description.setColour(juce::Label::textColourId, active ? hackerTextDim() : inactiveDesc);
    };

    applySectionTheme(overviewTitle, overviewDescription, showOverview, overviewAccent);
    applySectionTheme(modulationTitle, modulationDescription, showModulation, modulationAccent);
    applySectionTheme(toneTitle, toneDescription, showTone, toneAccent);
    applySectionTheme(engineTitle, engineDescription, showEngine, engineAccent);
    applySectionTheme(mappingTitle, mappingDescription, showLookFeel && selectedSubTab == 0, layoutAccent);
    applySectionTheme(uiTitle, uiDescription, showLookFeel && selectedSubTab == 1, layoutAccent);
    applySectionTheme(layoutTitle, layoutDescription, showLookFeel, layoutAccent);
    applySectionTheme(validationTitle, validationDescription, showValidation, validationAccent);
    applySectionTheme(settingsTitle, settingsDescription, showSettings, visualNeutral());

    auto styleTabButton = [](juce::TextButton& button, bool active, juce::Colour)
    {
        const auto bg = juce::Colour(0xff050505);
        const auto bgOn = hackerBgActive().withAlpha(0.88f);
        const auto text = active ? hackerText() : hackerTextDim();
        button.setColour(juce::TextButton::buttonColourId, bg);
        button.setColour(juce::TextButton::buttonOnColourId, bgOn);
        button.setColour(juce::TextButton::textColourOffId, text);
        button.setColour(juce::TextButton::textColourOnId, hackerText());
    };

    styleTabButton(tabOverviewButton, showOverview, overviewAccent);
    styleTabButton(tabInternalsButton, showModulation, modulationAccent);
    styleTabButton(tabBbdButton, showTone, toneAccent);
    styleTabButton(tabTapeButton, showEngine, engineAccent);
    styleTabButton(tabLayoutButton, showLookFeel, layoutAccent);
    styleTabButton(tabValidationButton, showValidation, validationAccent);
    styleTabButton(tabSettingsButton, showSettings, visualNeutral());

    const juce::Colour mutedAccent = hackerTextMuted();
    overviewVisualDeck.setAccentColour(showOverview ? overviewAccent : mutedAccent);
    modulationVisualDeck.setAccentColour(showModulation ? modulationAccent : mutedAccent);
    toneVisualDeck.setAccentColour(showTone ? toneAccent : mutedAccent);
    engineVisualDeck.setAccentColour(showEngine ? engineAccent : mutedAccent);
    lookFeelVisualDeck.setAccentColour(showLookFeel ? layoutAccent : mutedAccent);
    validationVisualDeck.setAccentColour(showValidation ? validationAccent : mutedAccent);

    auto setAllSectionsEnabled = [](juce::PropertyPanel& panel, bool panelVisible)
    {
        const auto sectionNames = panel.getSectionNames();
        for (int i = 0; i < sectionNames.size(); ++i)
        {
            panel.setSectionEnabled(i, true);

            if (!panelVisible)
                panel.setSectionOpen(i, false);
        }
    };

    const bool showLookFeelMapping = showLookFeel && selectedSubTab == 0;
    const bool showLookFeelUi = showLookFeel && selectedSubTab == 1;
    const bool showLookFeelEngineLayout = showLookFeel && selectedSubTab == 2;
    const bool showLookFeelGlobalLayout = showLookFeel && selectedSubTab == 3;
    const bool showLookFeelTextAnimations = showLookFeel && selectedSubTab == 4;
    const bool showLayoutGreen = showLookFeelEngineLayout && activeEngineIndex == 0;
    const bool showLayoutBlue = showLookFeelEngineLayout && activeEngineIndex == 1;
    const bool showLayoutRed = showLookFeelEngineLayout && activeEngineIndex == 2;
    const bool showLayoutPurple = showLookFeelEngineLayout && activeEngineIndex == 3;
    const bool showLayoutBlack = showLookFeelEngineLayout && activeEngineIndex == 4;

    overviewTitle.setVisible(showOverview);
    overviewDescription.setVisible(showOverview);
    overviewPanel.setVisible(showOverview);
    overviewVisualDeck.setVisible(showOverview);
    modulationTitle.setVisible(showModulation);
    modulationDescription.setVisible(showModulation);
    modulationPanel.setVisible(showModulation);
    modulationVisualDeck.setVisible(showModulation);
    toneTitle.setVisible(showTone);
    toneDescription.setVisible(showTone);
    tonePanel.setVisible(showTone);
    toneVisualDeck.setVisible(showTone);
    engineTitle.setVisible(showEngine);
    engineDescription.setVisible(showEngine);
    enginePanel.setVisible(showEngine);
    engineVisualDeck.setVisible(showEngine);
    lookFeelVisualDeck.setVisible(showLookFeel);
    mappingTitle.setVisible(showLookFeelMapping);
    mappingDescription.setVisible(showLookFeelMapping);
    mappingPanel.setVisible(showLookFeelMapping);
    uiTitle.setVisible(showLookFeelUi);
    uiDescription.setVisible(showLookFeelUi);
    uiPanel.setVisible(showLookFeelUi);
    layoutGlobalPanel.setVisible(showLookFeelGlobalLayout);
    layoutTextAnimationPanel.setVisible(showLookFeelTextAnimations);
    layoutGreenPanel.setVisible(showLayoutGreen);
    layoutBluePanel.setVisible(showLayoutBlue);
    layoutRedPanel.setVisible(showLayoutRed);
    layoutPurplePanel.setVisible(showLayoutPurple);
    layoutBlackPanel.setVisible(showLayoutBlack);
    activeProfileLabel.setVisible(true);
    activeScopeHintLabel.setVisible(true);
    if (!settingsShowScopeHintLine)
        activeScopeHintLabel.setVisible(false);
    devEngineModeLabel.setVisible(true);
    devEngineModeBox.setVisible(true);
    devHqModeToggle.setVisible(true);
    const bool showEngineInternalsTools = showEngine;
    engineFilterLabel.setVisible(showEngineInternalsTools);
    engineFilterEditor.setVisible(showEngineInternalsTools);
    engineFilterClearButton.setVisible(showEngineInternalsTools);
    engineFilterLabel.setColour(juce::Label::textColourId, showEngineInternalsTools ? hackerText() : hackerTextMuted());
    styleHackerEditor(engineFilterEditor);
    styleHackerTextButton(engineFilterClearButton, false);
    validationTitle.setVisible(showValidation);
    validationDescription.setVisible(showValidation);
    validationPanel.setVisible(showValidation);
    validationVisualDeck.setVisible(showValidation);
    settingsTitle.setVisible(showSettings);
    settingsDescription.setVisible(showSettings);
    settingsPanel.setVisible(showSettings);
    layoutTitle.setVisible(showLookFeel);
    layoutDescription.setVisible(showLookFeel);
    inspectorTitle.setVisible(true);
    inspectorDescription.setVisible(true);
    fxPresetOffButton.setVisible(showSettings);
    fxPresetSubtleButton.setVisible(showSettings);
    fxPresetMediumButton.setVisible(showSettings);
    lockToggleButton.setVisible(showSettings);
    resetFactoryButton.setVisible(showSettings);
    saveDefaultsButton.setVisible(showSettings);
    copyJsonButton.setVisible(showSettings);

    internalsTitle.setVisible(false);
    internalsDescription.setVisible(false);
    internalsGreenNqPanel.setVisible(false);
    internalsGreenHqPanel.setVisible(false);
    internalsBlueNqPanel.setVisible(false);
    internalsBlueHqPanel.setVisible(false);
    internalsRedNqPanel.setVisible(false);
    internalsRedHqPanel.setVisible(false);
    internalsPurpleNqPanel.setVisible(false);
    internalsPurpleHqPanel.setVisible(false);
    internalsBlackNqPanel.setVisible(false);
    internalsBlackHqPanel.setVisible(false);

    setAllSectionsEnabled(overviewPanel, showOverview);
    setAllSectionsEnabled(modulationPanel, showModulation);
    setAllSectionsEnabled(tonePanel, showTone);
    setAllSectionsEnabled(enginePanel, showEngine);
    setAllSectionsEnabled(validationPanel, showValidation);
    setAllSectionsEnabled(settingsPanel, showSettings);
    setAllSectionsEnabled(mappingPanel, showLookFeelMapping);
    setAllSectionsEnabled(uiPanel, showLookFeelUi);
    setAllSectionsEnabled(layoutGlobalPanel, showLookFeelGlobalLayout);
    setAllSectionsEnabled(layoutTextAnimationPanel, showLookFeelTextAnimations);
    setAllSectionsEnabled(layoutGreenPanel, showLayoutGreen);
    setAllSectionsEnabled(layoutBluePanel, showLayoutBlue);
    setAllSectionsEnabled(layoutRedPanel, showLayoutRed);
    setAllSectionsEnabled(layoutPurplePanel, showLayoutPurple);
    setAllSectionsEnabled(layoutBlackPanel, showLayoutBlack);

    if (showOverview)
        openAllSections(overviewPanel);
    if (showModulation)
        openAllSections(modulationPanel);
    if (showTone)
        openAllSections(tonePanel);
    if (showValidation)
        openAllSections(validationPanel);
    if (showSettings)
        openAllSections(settingsPanel);
    const auto* activeInternalsPanel = showEngine ? getActiveEngineInternalsPanel() : nullptr;
    auto setInternalsEnabled = [&](juce::PropertyPanel& panel)
    {
        setAllSectionsEnabled(panel, activeInternalsPanel == &panel);
    };
    setInternalsEnabled(internalsGreenNqPanel);
    setInternalsEnabled(internalsGreenHqPanel);
    setInternalsEnabled(internalsBlueNqPanel);
    setInternalsEnabled(internalsBlueHqPanel);
    setInternalsEnabled(internalsRedNqPanel);
    setInternalsEnabled(internalsRedHqPanel);
    setInternalsEnabled(internalsPurpleNqPanel);
    setInternalsEnabled(internalsPurpleHqPanel);
    setInternalsEnabled(internalsBlackNqPanel);
    setInternalsEnabled(internalsBlackHqPanel);
    setAllSectionsEnabled(bbdPanel, showEngine && activeInternalsPanel == &internalsRedNqPanel);
    setAllSectionsEnabled(tapePanel, showEngine && activeInternalsPanel == &internalsRedHqPanel);

    if (showEngine)
    {
        updateEngineSectionVisibility();
    }
    else
    {
        setActiveEngineSectionPrefix({}, false);
        internalsGreenNqPanel.setVisible(false);
        internalsGreenHqPanel.setVisible(false);
        internalsBlueNqPanel.setVisible(false);
        internalsBlueHqPanel.setVisible(false);
        internalsRedNqPanel.setVisible(false);
        internalsRedHqPanel.setVisible(false);
        internalsPurpleNqPanel.setVisible(false);
        internalsPurpleHqPanel.setVisible(false);
        internalsBlackNqPanel.setVisible(false);
        internalsBlackHqPanel.setVisible(false);
        bbdPanel.setVisible(false);
        tapePanel.setVisible(false);
    }

    if (!showModulation)
        modulationVisualDeck.setBounds(0, 0, 0, 0);
    if (!showOverview)
        overviewVisualDeck.setBounds(0, 0, 0, 0);
    if (!showTone)
        toneVisualDeck.setBounds(0, 0, 0, 0);
    if (!showEngine)
        engineVisualDeck.setBounds(0, 0, 0, 0);
    if (!showLookFeel)
        lookFeelVisualDeck.setBounds(0, 0, 0, 0);
    if (!showValidation)
        validationVisualDeck.setBounds(0, 0, 0, 0);

    bbdTitle.setVisible(false);
    bbdDescription.setVisible(false);
    tapeTitle.setVisible(false);
    tapeDescription.setVisible(false);

    tabOverviewButton.setToggleState(showOverview, juce::dontSendNotification);
    tabInternalsButton.setToggleState(showModulation, juce::dontSendNotification);
    tabBbdButton.setToggleState(showTone, juce::dontSendNotification);
    tabTapeButton.setToggleState(showEngine, juce::dontSendNotification);
    tabLayoutButton.setToggleState(showLookFeel, juce::dontSendNotification);
    tabValidationButton.setToggleState(showValidation, juce::dontSendNotification);
    tabSettingsButton.setToggleState(showSettings, juce::dontSendNotification);

    updateAnalyzerDemandFromVisibility();
}

void DevPanel::applyUiPreferences()
{
    const float uiTextScale = (settingsUiTextSize <= 0 ? 0.90f : (settingsUiTextSize >= 2 ? 1.18f : 1.0f));
    setDevPanelUserTextScale(uiTextScale);
    setDevPanelTextColourMode(settingsUiTextColourMode);
    setDevPanelThemePreset(settingsThemePreset);
    setDevPanelAccentSource(settingsAccentSource);
    setDevPanelManualAccentIndex(settingsManualAccent);
    setDevPanelColourVisionMode(settingsColourVisionMode);
    setDevPanelReducedMotion(settingsReducedMotion);
    setDevPanelLargeHitTargets(settingsLargeHitTargets);
    setDevPanelStrongFocusRing(settingsStrongFocusRing);

    const int currentEngine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    setCurrentEngineSkinColour(currentEngine);
    getDevPanelThemeLookAndFeel().refreshThemeColours();
    getDevPanelSectionLookAndFeel().refreshThemeColours();
    sendLookAndFeelChange();

    auto styleTitleLabel = [](juce::Label& label)
    {
        label.setFont(makeTitleFont(Typography::title, true));
        label.setColour(juce::Label::textColourId, hackerText());
    };
    auto styleDescriptionLabel = [](juce::Label& label)
    {
        label.setFont(makeLabelFont(Typography::description, false));
        label.setColour(juce::Label::textColourId, hackerTextDim());
    };

    styleTitleLabel(mappingTitle);
    styleDescriptionLabel(mappingDescription);
    styleTitleLabel(uiTitle);
    styleDescriptionLabel(uiDescription);
    styleTitleLabel(overviewTitle);
    styleDescriptionLabel(overviewDescription);
    styleTitleLabel(modulationTitle);
    styleDescriptionLabel(modulationDescription);
    styleTitleLabel(toneTitle);
    styleDescriptionLabel(toneDescription);
    styleTitleLabel(engineTitle);
    styleDescriptionLabel(engineDescription);
    styleTitleLabel(validationTitle);
    styleDescriptionLabel(validationDescription);
    styleTitleLabel(internalsTitle);
    styleDescriptionLabel(internalsDescription);
    styleTitleLabel(bbdTitle);
    styleDescriptionLabel(bbdDescription);
    styleTitleLabel(tapeTitle);
    styleDescriptionLabel(tapeDescription);
    styleTitleLabel(layoutTitle);
    styleDescriptionLabel(layoutDescription);
    styleTitleLabel(settingsTitle);
    styleDescriptionLabel(settingsDescription);
    styleTitleLabel(inspectorTitle);
    styleDescriptionLabel(inspectorDescription);

    activeProfileLabel.setFont(makeLabelFont(Typography::description, false));
    activeScopeHintLabel.setFont(makeLabelFont(Typography::description, false));
    devEngineModeLabel.setFont(makeLabelFont(Typography::labelSmall, false));
    engineFilterLabel.setFont(makeLabelFont(Typography::labelSmall, false));
    tutorialStepLabel.setFont(makeLabelFont(Typography::labelSmall, false));
    tutorialTitleLabel.setFont(makeLabelFont(Typography::title, true));
    tutorialFocusHintLabel.setFont(makeLabelFont(Typography::labelSmall, false));
    tutorialBodyText.setFont(makeLabelFont(Typography::description, false));
    engineFilterEditor.setFont(makeLabelFont(Typography::labelSmall, false));

    const int selectorAccentIndex = (settingsAccentSource == 1)
        ? juce::jlimit(0, 4, settingsManualAccent)
        : currentEngine;
    styleProfileSelectorComboBox(devEngineModeBox, profileSelectorColourForEngineIndex(selectorAccentIndex));
    styleHackerToggleButton(devHqModeToggle);
    styleHackerTextButton(copyJsonButton, false);
    styleHackerTextButton(saveDefaultsButton, false);
    styleHackerTextButton(resetFactoryButton, false);
    styleHackerTextButton(lockToggleButton, false);
    styleHackerTextButton(fxPresetOffButton, false);
    styleHackerTextButton(fxPresetSubtleButton, false);
    styleHackerTextButton(fxPresetMediumButton, false);
    styleHackerTextButton(engineFilterClearButton, false);
    styleHackerTextButton(tutorialNextButton, false);
    styleHackerTextButton(tutorialNextSectionButton, false);
    styleHackerTextButton(tutorialSkipButton, false);
    styleHackerEditor(engineFilterEditor);

    if (validationConsoleComponent != nullptr)
    {
        validationConsoleComponent->setAutoScroll(settingsConsoleAutoScroll);
        validationConsoleComponent->setShowTimestamps(settingsConsoleTimestamps);
        validationConsoleComponent->setWrapLongLines(settingsConsoleWrapLines);
        validationConsoleComponent->setMaxOutputLines(settingsConsoleMaxLines);
    }

    const int minRowHeight = settingsLargeHitTargets ? 46 : 38;
    const auto enforceMinRowHeight = [minRowHeight](juce::PropertyPanel& panel)
    {
        auto* root = panel.getChildComponent(0);
        if (root == nullptr)
            return;
        std::function<void(juce::Component&)> walk = [&](juce::Component& c)
        {
            if (auto* prop = dynamic_cast<juce::PropertyComponent*>(&c))
            {
                if (prop->getPreferredHeight() <= 50)
                    prop->setPreferredHeight(minRowHeight);
            }
            for (int i = 0; i < c.getNumChildComponents(); ++i)
                walk(*c.getChildComponent(i));
        };
        walk(*root);
    };

    enforceMinRowHeight(mappingPanel);
    enforceMinRowHeight(uiPanel);
    enforceMinRowHeight(overviewPanel);
    enforceMinRowHeight(modulationPanel);
    enforceMinRowHeight(tonePanel);
    enforceMinRowHeight(enginePanel);
    enforceMinRowHeight(validationPanel);
    enforceMinRowHeight(settingsPanel);
    enforceMinRowHeight(internalsGreenNqPanel);
    enforceMinRowHeight(internalsGreenHqPanel);
    enforceMinRowHeight(internalsBlueNqPanel);
    enforceMinRowHeight(internalsBlueHqPanel);
    enforceMinRowHeight(internalsRedNqPanel);
    enforceMinRowHeight(internalsRedHqPanel);
    enforceMinRowHeight(internalsPurpleNqPanel);
    enforceMinRowHeight(internalsPurpleHqPanel);
    enforceMinRowHeight(internalsBlackNqPanel);
    enforceMinRowHeight(internalsBlackHqPanel);
    enforceMinRowHeight(bbdPanel);
    enforceMinRowHeight(tapePanel);
    enforceMinRowHeight(layoutGlobalPanel);
    enforceMinRowHeight(layoutTextAnimationPanel);
    enforceMinRowHeight(layoutGreenPanel);
    enforceMinRowHeight(layoutBluePanel);
    enforceMinRowHeight(layoutRedPanel);
    enforceMinRowHeight(layoutPurplePanel);
    enforceMinRowHeight(layoutBlackPanel);

    updateActiveProfileLabel();
    refreshSecondaryTabButtons();
    updateRightTabVisibility();
    if (tutorialActive)
        applyTutorialStep();
    resized();
    repaint();
}

bool DevPanel::isPropertyVisibleInViewport(const juce::PropertyComponent* property) const
{
    if (property == nullptr || !property->isShowing())
        return false;
    if (!viewport.isShowing())
        return false;

    const auto propertyScreen = property->getScreenBounds();
    if (propertyScreen.isEmpty())
        return false;

    return propertyScreen.intersects(viewport.getScreenBounds());
}

void DevPanel::updateAnalyzerDemandFromVisibility()
{
    auto anyVisible = [this](const juce::Array<juce::PropertyComponent*>& properties) -> bool
    {
        for (auto* property : properties)
        {
            if (isPropertyVisibleInViewport(property))
                return true;
        }
        return false;
    };

    const bool modulationDemand = anyVisible(modulationVisualizerProperties);
    const bool spectrumDemand = anyVisible(spectrumVisualizerProperties);
    const bool transferDemand = anyVisible(transferVisualizerProperties);
    const bool telemetryDemand = anyVisible(analyzerTelemetryProperties)
                              || (selectedRightTab == 5 && validationPanel.isVisible());

    if (modulationDemand == lastModulationDemand
        && spectrumDemand == lastSpectrumDemand
        && transferDemand == lastTransferDemand
        && telemetryDemand == lastTelemetryDemand)
    {
        return;
    }

    lastModulationDemand = modulationDemand;
    lastSpectrumDemand = spectrumDemand;
    lastTransferDemand = transferDemand;
    lastTelemetryDemand = telemetryDemand;
    processor.setAnalyzerCardDemand(modulationDemand, spectrumDemand, transferDemand, telemetryDemand);
}

int DevPanel::refreshVisibleLiveReadouts()
{
    int refreshedCount = 0;
    for (auto* property : liveReadoutProperties)
    {
        if (!isPropertyVisibleInViewport(property))
            continue;
        property->refresh();
        ++refreshedCount;
    }
    return refreshedCount;
}

void DevPanel::timerCallback()
{
    updateConsoleSweeps();

    if (tutorialActive)
    {
        ++tutorialPulseTick;
        const float pulse = isDevPanelReducedMotionEnabled()
            ? 0.92f
            : (0.55f + 0.45f * std::sin(static_cast<float>(tutorialPulseTick) * 0.24f));
        tutorialFocusHighlight.setPulseAlpha(pulse);
        layoutTutorialOverlay();
        updateTutorialHighlight();
        tutorialFocusHighlight.toFront(false);
        tutorialOverlay.toFront(false);
    }

    auto readRaw = [this](const char* paramId) -> float
    {
        if (auto* p = processor.getValueTreeState().getRawParameterValue(paramId))
            return p->load();
        return 0.0f;
    };

    const int currentEngineIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
    const bool currentHqState = processor.isHqEnabled();
    const juce::String currentSectionFilter = engineFilterEditor.getText().trim();

    const float currentRateRaw = readRaw(ChoroborosAudioProcessor::RATE_ID);
    const float currentDepthRaw = readRaw(ChoroborosAudioProcessor::DEPTH_ID);
    const float currentOffsetRaw = readRaw(ChoroborosAudioProcessor::OFFSET_ID);
    const float currentWidthRaw = readRaw(ChoroborosAudioProcessor::WIDTH_ID);
    const float currentColorRaw = readRaw(ChoroborosAudioProcessor::COLOR_ID);
    const float currentMixRaw = readRaw(ChoroborosAudioProcessor::MIX_ID);

    auto pushTouch = [this](const juce::String& label, const juce::String& valueText)
    {
        const juce::String timestamp = juce::Time::getCurrentTime().formatted("%H:%M:%S");
        const juce::String prefix = label + ":";
        const juce::String line = prefix + " " + valueText + " @ " + timestamp;
        for (int i = recentTouchHistory.size() - 1; i >= 0; --i)
        {
            if (recentTouchHistory[i].startsWithIgnoreCase(prefix))
                recentTouchHistory.remove(i);
        }
        recentTouchHistory.insert(0, line);
        while (recentTouchHistory.size() > 10)
            recentTouchHistory.remove(recentTouchHistory.size() - 1);
        appendRecentTouchLogLine(line);
    };

    if (!macroTouchHistoryPrimed)
    {
        lastRateRaw = currentRateRaw;
        lastDepthRaw = currentDepthRaw;
        lastOffsetRaw = currentOffsetRaw;
        lastWidthRaw = currentWidthRaw;
        lastColorRaw = currentColorRaw;
        lastMixRaw = currentMixRaw;
        macroTouchHistoryPrimed = true;
    }
    else
    {
        if (std::abs(currentRateRaw - lastRateRaw) > 1.0e-4f)
        {
            const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, currentRateRaw);
            pushTouch("Rate", juce::String(mapped, 3) + " Hz");
            lastRateRaw = currentRateRaw;
        }
        if (std::abs(currentDepthRaw - lastDepthRaw) > 1.0e-4f)
        {
            const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::DEPTH_ID, currentDepthRaw);
            pushTouch("Depth", juce::String(mapped * 100.0f, 1) + " %");
            lastDepthRaw = currentDepthRaw;
        }
        if (std::abs(currentOffsetRaw - lastOffsetRaw) > 1.0e-4f)
        {
            const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::OFFSET_ID, currentOffsetRaw);
            pushTouch("Offset", juce::String(mapped, 2) + " deg");
            lastOffsetRaw = currentOffsetRaw;
        }
        if (std::abs(currentWidthRaw - lastWidthRaw) > 1.0e-4f)
        {
            const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::WIDTH_ID, currentWidthRaw);
            pushTouch("Width", juce::String(mapped, 3) + " x");
            lastWidthRaw = currentWidthRaw;
        }
        if (std::abs(currentColorRaw - lastColorRaw) > 1.0e-4f)
        {
            const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID, currentColorRaw);
            pushTouch("Color", juce::String(mapped, 3));
            lastColorRaw = currentColorRaw;
        }
        if (std::abs(currentMixRaw - lastMixRaw) > 1.0e-4f)
        {
            const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::MIX_ID, currentMixRaw);
            pushTouch("Mix", juce::String(mapped * 100.0f, 1) + " %");
            lastMixRaw = currentMixRaw;
        }
    }

    if (currentEngineIndex != lastKnownEngineIndex
        || currentHqState != lastKnownHqState
        || currentSectionFilter != lastKnownSectionFilter)
    {
        static const juce::String engineNames[] { "Green", "Blue", "Red", "Purple", "Black" };
        if (currentEngineIndex != lastKnownEngineIndex)
            pushTouch("Engine", engineNames[currentEngineIndex]);
        if (currentHqState != lastKnownHqState)
            pushTouch("Mode", currentHqState ? "HQ" : "NQ");
        if (currentSectionFilter != lastKnownSectionFilter && currentSectionFilter.isNotEmpty())
            pushTouch("Filter", currentSectionFilter);

        lastKnownEngineIndex = currentEngineIndex;
        lastKnownHqState = currentHqState;
        lastKnownSectionFilter = currentSectionFilter;
        updateActiveProfileLabel();
        updateRightTabVisibility();
        resized();
    }

    const int requestedRefreshHz = processor.getAnalyzerRefreshHz();
    const int ticksPerRefresh = juce::jmax(1, kDevPanelTimerHz / juce::jmax(1, requestedRefreshHz));
    ++analyzerRefreshTickCounter;
    if (analyzerRefreshTickCounter >= ticksPerRefresh)
    {
        analyzerRefreshTickCounter = 0;
        updateAnalyzerDemandFromVisibility();
        refreshVisibleLiveReadouts();
    }

    // These inspector tabs are intentionally fixed-open to avoid tiny accordion sections.
    if (selectedRightTab == 0)
        openAllSections(overviewPanel);
    else if (selectedRightTab == 1)
        openAllSections(modulationPanel);
    else if (selectedRightTab == 2)
        openAllSections(tonePanel);
    else if (selectedRightTab == 5)
        openAllSections(validationPanel);

    const auto panelHeightDrifted = [](juce::PropertyPanel& panel, int panelPadding) -> bool
    {
        if (!panel.isVisible() || !panel.isShowing())
            return false;
        const int targetHeight = panel.getTotalContentHeight() + panelPadding;
        return std::abs(panel.getHeight() - targetHeight) > 1;
    };

    // PropertyPanel section expand/collapse doesn't always trigger parent layout immediately.
    // Detect panel content-height drift and force a relayout so sections resize instantly.
    const bool needsRelayout =
        panelHeightDrifted(mappingPanel, kPanelAutoPadding)
        || panelHeightDrifted(uiPanel, kPanelAutoPadding)
        || panelHeightDrifted(layoutGlobalPanel, kPanelAutoPadding)
        || panelHeightDrifted(layoutTextAnimationPanel, kPanelAutoPadding)
        || panelHeightDrifted(layoutGreenPanel, kPanelAutoPadding)
        || panelHeightDrifted(layoutBluePanel, kPanelAutoPadding)
        || panelHeightDrifted(layoutRedPanel, kPanelAutoPadding)
        || panelHeightDrifted(layoutPurplePanel, kPanelAutoPadding)
        || panelHeightDrifted(layoutBlackPanel, kPanelAutoPadding)
        || panelHeightDrifted(overviewPanel, kPanelAutoPadding)
        || panelHeightDrifted(modulationPanel, kPanelAutoPadding)
        || panelHeightDrifted(tonePanel, kPanelAutoPadding)
        || panelHeightDrifted(enginePanel, kPanelAutoPadding)
        || panelHeightDrifted(validationPanel, kPanelAutoPadding)
        || panelHeightDrifted(settingsPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsGreenNqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsGreenHqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsBlueNqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsBlueHqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsRedNqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsRedHqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsPurpleNqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsPurpleHqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsBlackNqPanel, kPanelAutoPadding)
        || panelHeightDrifted(internalsBlackHqPanel, kPanelAutoPadding)
        || panelHeightDrifted(bbdPanel, kPanelAutoPadding)
        || panelHeightDrifted(tapePanel, kPanelAutoPadding);

    if (needsRelayout)
        resized();

    if (saveButtonResetCountdownTicks > 0)
    {
        --saveButtonResetCountdownTicks;
        if (saveButtonResetCountdownTicks == 0)
        {
            saveDefaultsButton.setButtonText("Set Current as Defaults");
            saveDefaultsButton.setEnabled(true);
        }
    }

    if (resetFactoryButtonResetCountdownTicks > 0)
    {
        --resetFactoryButtonResetCountdownTicks;
        if (resetFactoryButtonResetCountdownTicks == 0)
        {
            resetFactoryButton.setButtonText("Reset to Factory");
            resetFactoryButton.setEnabled(true);
        }
    }
}

bool DevPanel::startTutorial(const juce::String& requestedTopic, juce::String& statusMessage)
{
    juce::String resolvedTopic;
    juce::String resolvedTitle;
    auto steps = buildTutorialScript(requestedTopic, resolvedTopic, resolvedTitle);
    if (steps.empty())
    {
        statusMessage = "ERROR: unknown tutorial topic `"
                      + (requestedTopic.trim().isEmpty() ? juce::String("core") : requestedTopic.trim())
                      + "`. Try: core, overview, modulation, tone, engine, validation, bbd, tape, phase, bimodulation, saturation, envelope.";
        return false;
    }

    tutorialActive = true;
    tutorialTopicKey = resolvedTopic;
    tutorialTopicTitle = resolvedTitle;
    tutorialSteps = std::move(steps);
    tutorialStepIndex = 0;
    tutorialFocusTarget.clear();
    tutorialPulseTick = 0;
    tutorialOverlay.setVisible(true);

    applyTutorialStep();
    statusMessage = "Started tutorial `" + tutorialTopicKey + "` (" + juce::String(static_cast<int>(tutorialSteps.size()))
                + " steps). Use Next/Got It or `tutorial skip`.";
    return true;
}

bool DevPanel::stopTutorial(bool skipped, juce::String& statusMessage)
{
    if (!tutorialActive)
    {
        statusMessage = "No active tutorial.";
        return false;
    }

    const juce::String finishedTitle = tutorialTopicTitle.isNotEmpty() ? tutorialTopicTitle : tutorialTopicKey;
    tutorialActive = false;
    tutorialTopicKey.clear();
    tutorialTopicTitle.clear();
    tutorialSteps.clear();
    tutorialStepIndex = -1;
    tutorialFocusTarget.clear();
    tutorialPulseTick = 0;
    tutorialOverlay.setVisible(false);
    tutorialFocusHighlight.setVisible(false);
    resized();
    repaint();

    if (skipped)
    {
        statusMessage = "Tutorial skipped.";
        appendRecentTouchLogLine("Tutorial skipped: " + finishedTitle);
    }
    else
    {
        statusMessage = "Tutorial complete: " + finishedTitle + ".";
        appendRecentTouchLogLine("Tutorial complete: " + finishedTitle);
    }
    return true;
}

void DevPanel::advanceTutorialStep()
{
    if (!tutorialActive)
        return;

    ++tutorialStepIndex;
    if (tutorialStepIndex >= static_cast<int>(tutorialSteps.size()))
    {
        juce::String status;
        stopTutorial(false, status);
        return;
    }
    applyTutorialStep();
}

void DevPanel::advanceTutorialSection()
{
    if (!tutorialActive)
        return;

    const int currentTab = selectedRightTab;
    const int currentSubTab = getSelectedSubTab();

    int effectiveTab = currentTab;
    int effectiveSubTab = currentSubTab;
    for (int i = tutorialStepIndex + 1; i < static_cast<int>(tutorialSteps.size()); ++i)
    {
        const auto& step = tutorialSteps[static_cast<size_t>(i)];
        const int nextTab = step.rightTab >= 0 ? juce::jlimit(0, 6, step.rightTab) : effectiveTab;
        const int nextSubTab = step.subTab >= 0 ? juce::jlimit(0, juce::jmax(0, getSubTabCount(nextTab) - 1), step.subTab)
                                                : effectiveSubTab;

        if (nextTab != currentTab || nextSubTab != currentSubTab)
        {
            tutorialStepIndex = i;
            applyTutorialStep();
            return;
        }

        effectiveTab = nextTab;
        effectiveSubTab = nextSubTab;
    }

    juce::String status;
    stopTutorial(false, status);
}

void DevPanel::applyTutorialStep()
{
    if (!tutorialActive || tutorialStepIndex < 0 || tutorialStepIndex >= static_cast<int>(tutorialSteps.size()))
        return;

    const auto& step = tutorialSteps[static_cast<size_t>(tutorialStepIndex)];

    if (step.forceEngine >= 0)
    {
        const int engineIndex = juce::jlimit(0, 4, step.forceEngine);
        if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
        {
            const auto& range = processor.getValueTreeState().getParameterRange(ChoroborosAudioProcessor::ENGINE_COLOR_ID);
            const float normalized = range.convertTo0to1(static_cast<float>(engineIndex));
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalized);
            param->endChangeGesture();
        }
    }

    if (step.forceHq >= 0)
    {
        if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::HQ_ID))
        {
            const bool hqEnabled = step.forceHq > 0;
            param->beginChangeGesture();
            param->setValueNotifyingHost(hqEnabled ? 1.0f : 0.0f);
            param->endChangeGesture();
        }
    }

    if (step.rightTab >= 0)
    {
        selectedRightTab = juce::jlimit(0, 6, step.rightTab);
        selectedSubTabs[static_cast<size_t>(selectedRightTab)] =
            juce::jlimit(0, juce::jmax(0, getSubTabCount(selectedRightTab) - 1),
                         selectedSubTabs[static_cast<size_t>(selectedRightTab)]);
    }

    const int activeTab = juce::jlimit(0, 6, selectedRightTab);
    if (step.subTab >= 0)
    {
        const int clampedSub = juce::jlimit(0, juce::jmax(0, getSubTabCount(activeTab) - 1), step.subTab);
        selectedSubTabs[static_cast<size_t>(activeTab)] = clampedSub;
    }

    tutorialFocusTarget = step.focusTarget.toLowerCase().trim();
    refreshSecondaryTabButtons();
    updateActiveProfileLabel();
    updateRightTabVisibility();
    expandTutorialFocusSections();
    resized();
    revealTutorialFocusTarget();

    const int totalSteps = static_cast<int>(tutorialSteps.size());
    const bool finalStep = tutorialStepIndex >= totalSteps - 1;
    bool hasNextSection = false;
    int effectiveTab = selectedRightTab;
    int effectiveSubTab = getSelectedSubTab();
    for (int i = tutorialStepIndex + 1; i < totalSteps; ++i)
    {
        const auto& future = tutorialSteps[static_cast<size_t>(i)];
        const int nextTab = future.rightTab >= 0 ? juce::jlimit(0, 6, future.rightTab) : effectiveTab;
        const int nextSub = future.subTab >= 0 ? juce::jlimit(0, juce::jmax(0, getSubTabCount(nextTab) - 1), future.subTab)
                                               : effectiveSubTab;
        if (nextTab != selectedRightTab || nextSub != getSelectedSubTab())
        {
            hasNextSection = true;
            break;
        }
        effectiveTab = nextTab;
        effectiveSubTab = nextSub;
    }
    tutorialStepLabel.setText("Tutorial: " + tutorialTopicTitle + "  •  Step "
                              + juce::String(tutorialStepIndex + 1) + "/" + juce::String(totalSteps),
                              juce::dontSendNotification);
    tutorialTitleLabel.setText(step.title, juce::dontSendNotification);
    tutorialBodyText.setText(step.body, juce::dontSendNotification);
    tutorialBodyText.setCaretPosition(0);
    const juce::String focusHintText = settingsShowTutorialHintsOnOpen
        ? (step.focusHint.isNotEmpty() ? ("Focus: " + step.focusHint) : juce::String())
        : juce::String();
    tutorialFocusHintLabel.setText(focusHintText,
                                   juce::dontSendNotification);
    tutorialNextButton.setButtonText(finalStep ? "Got It" : "Next");
    tutorialNextSectionButton.setEnabled(hasNextSection);
    tutorialNextSectionButton.setButtonText(hasNextSection ? "Next Section" : "Last Section");

    juce::Colour accent = hackerText();
    switch (activeTab)
    {
        case 0: accent = visualOverview(); break;
        case 1: accent = visualModulation(); break;
        case 2: accent = visualTone(); break;
        case 3: accent = visualEngine(); break;
        case 4: accent = visualLayout(); break;
        case 5: accent = visualValidation(); break;
        case 6: default: accent = hackerText(); break;
    }
    tutorialOverlay.setAccentColour(accent);
    tutorialFocusHighlight.setAccentColour(accent);
    tutorialFocusHighlight.setPulseAlpha(1.0f);
    tutorialOverlay.setVisible(true);

    layoutTutorialOverlay();
    updateTutorialHighlight();
    tutorialFocusHighlight.toFront(false);
    tutorialOverlay.toFront(false);
}

void DevPanel::layoutTutorialOverlay()
{
    if (!tutorialActive || !tutorialOverlay.isVisible())
        return;

    const auto viewPos = viewport.getViewPosition();
    juce::Rectangle<int> visibleAreaInContent(viewPos.x, viewPos.y, viewport.getViewWidth(), viewport.getViewHeight());
    if (visibleAreaInContent.isEmpty())
        return;

    visibleAreaInContent = visibleAreaInContent.reduced(8);

    juce::StringArray lines;
    lines.addLines(tutorialBodyText.getText());
    int estimatedLines = juce::jmax(1, lines.size());
    for (const auto& line : lines)
        estimatedLines += line.length() / 72;
    estimatedLines = juce::jlimit(4, 12, estimatedLines);

    const int width = juce::jlimit(360, 560, static_cast<int>(std::round(visibleAreaInContent.getWidth() * 0.45)));
    const int bodyHeight = juce::jlimit(78, 150, estimatedLines * 12);
    const int hintHeight = tutorialFocusHintLabel.getText().trim().isEmpty() ? 0 : 18;
    const int height = juce::jlimit(168, 250, 20 + 30 + 4 + bodyHeight + hintHeight + 34 + 20);

    auto placeCandidate = [&](int x, int y) -> juce::Rectangle<int>
    {
        auto candidate = juce::Rectangle<int>(x, y, width, height);
        if (candidate.getX() < visibleAreaInContent.getX())
            candidate.setX(visibleAreaInContent.getX());
        if (candidate.getY() < visibleAreaInContent.getY())
            candidate.setY(visibleAreaInContent.getY());
        if (candidate.getRight() > visibleAreaInContent.getRight())
            candidate.setX(visibleAreaInContent.getRight() - candidate.getWidth());
        if (candidate.getBottom() > visibleAreaInContent.getBottom())
            candidate.setY(visibleAreaInContent.getBottom() - candidate.getHeight());
        return candidate;
    };

    juce::Rectangle<int> focusBounds;
    if (auto* target = resolveTutorialFocusComponent(tutorialFocusTarget))
    {
        if (target->isVisible() && target->getWidth() > 0 && target->getHeight() > 0)
            focusBounds = target->getBounds().expanded(10).getIntersection(content.getLocalBounds());
    }

    constexpr int margin = 10;
    std::array<juce::Rectangle<int>, 4> candidates
    {
        placeCandidate(visibleAreaInContent.getRight() - width - margin, visibleAreaInContent.getY() + margin),
        placeCandidate(visibleAreaInContent.getX() + margin, visibleAreaInContent.getY() + margin),
        placeCandidate(visibleAreaInContent.getRight() - width - margin, visibleAreaInContent.getBottom() - height - margin),
        placeCandidate(visibleAreaInContent.getX() + margin, visibleAreaInContent.getBottom() - height - margin)
    };

    juce::Rectangle<int> best = candidates[0];
    int bestScore = std::numeric_limits<int>::max();
    for (const auto& candidate : candidates)
    {
        const int overlapScore = rectangleArea(candidate.getIntersection(focusBounds)) * 10;
        const int topBias = candidate.getY() - visibleAreaInContent.getY();
        const int score = overlapScore + topBias;
        if (score < bestScore)
        {
            bestScore = score;
            best = candidate;
        }
    }
    tutorialOverlay.setBounds(best);

    auto area = tutorialOverlay.getLocalBounds().reduced(12, 10);
    tutorialStepLabel.setBounds(area.removeFromTop(18));
    tutorialTitleLabel.setBounds(area.removeFromTop(28));
    area.removeFromTop(4);

    auto footer = area.removeFromBottom(38 + hintHeight);
    tutorialBodyText.setBounds(area);

    if (hintHeight > 0)
        tutorialFocusHintLabel.setBounds(footer.removeFromTop(hintHeight));
    else
        tutorialFocusHintLabel.setBounds(0, 0, 0, 0);
    auto buttons = footer.removeFromBottom(34);
    constexpr int gap = 8;
    constexpr int skipW = 110;
    constexpr int nextW = 96;
    tutorialSkipButton.setBounds(buttons.removeFromLeft(skipW));
    buttons.removeFromLeft(gap);
    tutorialNextButton.setBounds(buttons.removeFromRight(nextW));
    buttons.removeFromRight(gap);
    tutorialNextSectionButton.setBounds(buttons);
}

void DevPanel::updateTutorialHighlight()
{
    if (!tutorialActive || tutorialFocusTarget.isEmpty())
    {
        tutorialFocusHighlight.setVisible(false);
        return;
    }

    auto* target = resolveTutorialFocusComponent(tutorialFocusTarget);
    if (target == nullptr || !target->isVisible() || target->getWidth() <= 0 || target->getHeight() <= 0)
    {
        tutorialFocusHighlight.setVisible(false);
        return;
    }

    auto focusBounds = target->getBounds().expanded(8);
    focusBounds = focusBounds.getIntersection(content.getLocalBounds());
    if (focusBounds.isEmpty() || focusBounds.getWidth() < 16 || focusBounds.getHeight() < 16)
    {
        tutorialFocusHighlight.setVisible(false);
        return;
    }

    tutorialFocusHighlight.setBounds(focusBounds);
    tutorialFocusHighlight.setVisible(true);
}

void DevPanel::expandTutorialFocusSections()
{
    if (!tutorialActive)
        return;

    const juce::String key = tutorialFocusTarget.toLowerCase().trim();
    if (key.isEmpty())
        return;

    if (key == "overview_inspector" || key == "overview_readouts")
    {
        openAllSections(overviewPanel);
    }
    else if (key == "modulation_inspector" || key == "modulation_readouts")
    {
        openAllSections(modulationPanel);
    }
    else if (key == "tone_inspector" || key == "tone_controls" || key == "tone_readouts")
    {
        openAllSections(tonePanel);
    }
    else if (key == "validation_inspector" || key == "validation_readouts")
    {
        openAllSections(validationPanel);
    }
    else if (key == "layout_inspector")
    {
        openAllSections(mappingPanel);
        openAllSections(uiPanel);
    }
    else if (key == "layout_active_inspector")
    {
        const int selectedSubTab = getSelectedSubTab();
        if (selectedRightTab == 4)
        {
            if (selectedSubTab == 0)
                openAllSections(mappingPanel);
            else if (selectedSubTab == 1)
                openAllSections(uiPanel);
            else if (selectedSubTab == 2)
            {
                if (auto* active = getActiveEngineLayoutPanel())
                    openAllSections(*active);
            }
            else if (selectedSubTab == 3)
                openAllSections(layoutGlobalPanel);
            else if (selectedSubTab == 4)
                openAllSections(layoutTextAnimationPanel);
        }
    }
    else if (key == "engine_inspector" || key == "bbd_controls" || key == "tape_controls")
    {
        openAllSections(enginePanel);
        if (auto* activeInternals = getActiveEngineInternalsPanel())
            openAllSections(*activeInternals);
        if (bbdPanel.isVisible())
            openAllSections(bbdPanel);
        if (tapePanel.isVisible())
            openAllSections(tapePanel);
    }
}

void DevPanel::revealTutorialFocusTarget()
{
    if (!tutorialActive || tutorialFocusTarget.isEmpty())
        return;

    auto* target = resolveTutorialFocusComponent(tutorialFocusTarget);
    if (target == nullptr || !target->isVisible() || target->getWidth() <= 0 || target->getHeight() <= 0)
        return;

    const auto focusBounds = target->getBounds().expanded(18).getIntersection(content.getLocalBounds());
    if (focusBounds.isEmpty())
        return;

    const auto viewPos = viewport.getViewPosition();
    const auto viewportRect = juce::Rectangle<int>(viewPos.x, viewPos.y, viewport.getViewWidth(), viewport.getViewHeight());
    int desiredX = viewportRect.getX();
    int desiredY = viewportRect.getY();

    if (focusBounds.getY() < viewportRect.getY() + 10)
        desiredY = focusBounds.getY() - 18;
    else if (focusBounds.getBottom() > viewportRect.getBottom() - 10)
        desiredY = focusBounds.getBottom() - viewportRect.getHeight() + 18;

    if (focusBounds.getX() < viewportRect.getX() + 10)
        desiredX = focusBounds.getX() - 18;
    else if (focusBounds.getRight() > viewportRect.getRight() - 10)
        desiredX = focusBounds.getRight() - viewportRect.getWidth() + 18;

    const int maxX = juce::jmax(0, content.getWidth() - viewport.getViewWidth());
    const int maxY = juce::jmax(0, content.getHeight() - viewport.getViewHeight());
    desiredX = juce::jlimit(0, maxX, desiredX);
    desiredY = juce::jlimit(0, maxY, desiredY);

    if (desiredX != viewPos.x || desiredY != viewPos.y)
        viewport.setViewPosition(desiredX, desiredY);
}

juce::Component* DevPanel::resolveTutorialFocusComponent(const juce::String& focusTarget) const
{
    const juce::String key = focusTarget.toLowerCase();
    if (key == "active_profile") return const_cast<juce::Label*>(&activeProfileLabel);
    if (key == "dev_profile_controls") return const_cast<juce::ComboBox*>(&devEngineModeBox);
    if (key == "engine_selector") return const_cast<juce::ComboBox*>(&devEngineModeBox);
    if (key == "hq_toggle") return const_cast<juce::ToggleButton*>(&devHqModeToggle);
    if (key == "overview_visual") return const_cast<VisualDeckContent*>(&overviewVisualDeck);
    if (key == "overview_inspector" || key == "overview_readouts")
        return const_cast<juce::PropertyPanel*>(&overviewPanel);
    if (key == "modulation_visual" || key == "lfo_scope") return const_cast<VisualDeckContent*>(&modulationVisualDeck);
    if (key == "tone_visual" || key == "spectrum_visual" || key == "transfer_visual") return const_cast<VisualDeckContent*>(&toneVisualDeck);
    if (key == "engine_visual" || key == "engine_signal_flow") return const_cast<VisualDeckContent*>(&engineVisualDeck);
    if (key == "validation_visual" || key == "trace_matrix" || key == "validation_console")
        return const_cast<VisualDeckContent*>(&validationVisualDeck);
    if (key == "modulation_controls")
    {
        for (auto* card : modulationVisualDeckCards)
        {
            if (card == nullptr || !card->isVisible())
                continue;
            if (card->getProperties().contains("devpanelModRole")
                && card->getProperties()["devpanelModRole"].toString() == "lfo_workbench")
                return card;
        }
        return const_cast<VisualDeckContent*>(&modulationVisualDeck);
    }
    if (key == "modulation_inspector" || key == "modulation_readouts")
        return const_cast<juce::PropertyPanel*>(&modulationPanel);
    if (key == "tone_inspector" || key == "tone_readouts")
        return const_cast<juce::PropertyPanel*>(&tonePanel);
    if (key == "tone_controls")
    {
        for (auto* card : toneVisualDeckCards)
        {
            if (card == nullptr || !card->isVisible())
                continue;
            if (card->getProperties().contains("devpanelSubTab")
                && static_cast<int>(card->getProperties()["devpanelSubTab"]) == 1)
            {
                if (card->getName().containsIgnoreCase("Transfer Controls")
                    || card->getName().containsIgnoreCase("Tone Controls")
                    || card->getName().containsIgnoreCase("Engine Response Controls")
                    || card->getName().containsIgnoreCase("Saturation Controls")
                    || card->getTooltip().containsIgnoreCase("transfer controls")
                    || card->getTooltip().containsIgnoreCase("response controls")
                    || card->getTooltip().containsIgnoreCase("Saturation control workbench"))
                    return card;
            }
        }
        return const_cast<juce::PropertyPanel*>(&tonePanel);
    }
    if (key == "engine_inspector" || key == "bbd_controls" || key == "tape_controls")
        return const_cast<juce::PropertyPanel*>(&enginePanel);
    if (key == "validation_inspector" || key == "validation_readouts")
        return const_cast<juce::PropertyPanel*>(&validationPanel);
    if (key == "layout_inspector") return const_cast<juce::PropertyPanel*>(&mappingPanel);
    if (key == "layout_active_inspector")
    {
        const int selectedSubTab = getSelectedSubTab();
        if (selectedRightTab == 4)
        {
            if (selectedSubTab == 0) return const_cast<juce::PropertyPanel*>(&mappingPanel);
            if (selectedSubTab == 1) return const_cast<juce::PropertyPanel*>(&uiPanel);
            if (selectedSubTab == 2)
            {
                if (auto* active = const_cast<DevPanel*>(this)->getActiveEngineLayoutPanel())
                    return active;
            }
            if (selectedSubTab == 3) return const_cast<juce::PropertyPanel*>(&layoutGlobalPanel);
            if (selectedSubTab == 4) return const_cast<juce::PropertyPanel*>(&layoutTextAnimationPanel);
        }
    }
    if (key == "settings_actions") return const_cast<juce::TextButton*>(&saveDefaultsButton);
    return nullptr;
}

std::vector<DevPanel::TutorialStep> DevPanel::buildTutorialScript(const juce::String& requestedTopic,
                                                                  juce::String& resolvedTopic,
                                                                  juce::String& resolvedTitle) const
{
    juce::String topic = requestedTopic.trim().toLowerCase();
    if (topic.isEmpty() || topic == "core" || topic == "full" || topic == "walkthrough" || topic == "tutorial")
        topic = "core";

    auto makeStep = [](const juce::String& title,
                       const juce::String& body,
                       int rightTab,
                       int subTab,
                       int forceEngine,
                       int forceHq,
                       const juce::String& focusTarget,
                       const juce::String& focusHint) -> TutorialStep
    {
        TutorialStep step;
        step.title = title;
        step.body = body;
        step.rightTab = rightTab;
        step.subTab = subTab;
        step.forceEngine = forceEngine;
        step.forceHq = forceHq;
        step.focusTarget = focusTarget;
        step.focusHint = focusHint;
        return step;
    };

    std::vector<TutorialStep> steps;

    if (topic == "core")
    {
        resolvedTopic = "core";
        resolvedTitle = "Core DSP Walkthrough";
        steps.push_back(makeStep("Step 1: Orientation and Profile Target",
                                 "Start at Profile and HQ controls. HQ/NQ means High Quality vs Normal Quality processing mode. Every edit in Modulation, Tone, and Engine writes to the active profile shown here.",
                                 0, 0, -1, -1, "dev_profile_controls",
                                 "Profile selector + HQ/NQ define your target profile."));
        steps.push_back(makeStep("Step 2: Host Context First",
                                 "Before tuning, read host sample rate and buffer size in telemetry. Different sessions can sound/measure different because DSP runs with different timing budgets.",
                                 5, 0, -1, -1, "validation_readouts",
                                 "Host context: sample rate + buffer size."));
        steps.push_back(makeStep("Step 3: End-to-End Build Recipe",
                                 "Recipe you can repeat:\n1) Start in Overview and pick profile.\n2) Set modulation motion (Rate/Depth/Width).\n3) Shape tone (LPF/HPF + drive).\n4) Validate telemetry and trace matrix.\n5) Save/export safely.",
                                 0, 0, -1, -1, "overview_visual",
                                 "This is the full workflow: build -> listen -> validate -> persist."));
        steps.push_back(makeStep("Step 4: Chorus Principle",
                                 "A chorus duplicates the signal, delays it a little, and keeps changing that delay over time. Mixing dry+moving-delay creates the thick multi-voice illusion.",
                                 0, 0, -1, -1, "overview_visual",
                                 "Overview / Signal Flow: chorus from first principles."));
        steps.push_back(makeStep("Step 5: Overview Inspector Readouts",
                                 "Use Overview inspector readouts for fast sanity checks before deep edits: active profile values, current behavior, and immediate context.",
                                 0, 0, -1, -1, "overview_readouts",
                                 "Overview readouts before touching controls."));
        steps.push_back(makeStep("Step 6: Overview Delay Visual",
                                 "Switch to Delay Visual (second-level subtab) and confirm movement shape and range. Listen for smooth motion without obvious stepping.",
                                 0, 1, -1, -1, "overview_visual",
                                 "Overview second-level subtab: Delay Visual."));
        steps.push_back(makeStep("Step 7: LFO Definition and Motion",
                                 "LFO means Low Frequency Oscillator: a slow repeating control signal that moves delay time. In Modulation, the unified motion deck shows Left/Right LFO scopes together.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "Left/Right scopes define movement speed and amount."));
        steps.push_back(makeStep("Step 8: Modulation Controls + Readouts",
                                 "Set core motion and listen:\n- Rate: movement speed\n- Depth: movement amount\n- Width: stereo spread\nReadouts confirm exact values while you listen.",
                                 1, 0, -1, -1, "modulation_controls",
                                 "Use the LFO Control Workbench under the motion visuals."));
        steps.push_back(makeStep("Step 9: Right-Side Motion and Anti-Phase",
                                 "Compare LFO Left and Right side-by-side. Anti-phase means one side rises while the other falls (often near 180 degrees offset), which increases stereo width.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "Left/Right phase relationship drives stereo width."));
        steps.push_back(makeStep("Step 10: Trajectory and Zipper",
                                 "Trajectory is how quickly targets are allowed to move over time. Zipper noise means audible stepping from abrupt parameter jumps. Use trajectory/smoothing limits to prevent it.",
                                 1, 0, -1, -1, "modulation_visual",
                                 "Trajectory card + LFO workbench prevent stepping artifacts."));
        steps.push_back(makeStep("Step 11: LPF/HPF and Aliasing",
                                 "Go to Tone / Spectrum. LPF/HPF means Low-Pass and High-Pass filters. LPF darkens highs; HPF removes lows. Aliasing is unwanted high-frequency foldback artifacts, often controlled with filtering.",
                                 2, 0, -1, -1, "spectrum_visual",
                                 "Tone / Spectrum: frequency shaping basics."));
        steps.push_back(makeStep("Step 12: Tone Controls and Listening Goal",
                                 "Tune tone with intent: reduce harsh swish with LPF, remove mud with HPF, and keep clarity in the source. Use analyzer visuals to confirm what you hear.",
                                 2, 0, -1, -1, "tone_readouts",
                                 "Tone inspector controls + analyzer readouts."));
        steps.push_back(makeStep("Step 13: Engine Response and Harmonics",
                                 "Switch to Tone / Engine Response. This subtab is engine-aware: Red modes expose saturation/tape behavior, while other engines expose their own response curves and tone behavior. Harmonics are extra overtones created by non-linearity.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Engine Response curve for the active engine."));
        steps.push_back(makeStep("Step 14: Engine Response Controls",
                                 "Use the Engine Response card controls while watching curve behavior. For Red, tune drive/saturation; for other engines, tune response-shaping controls tied to that engine family.",
                                 2, 1, -1, -1, "tone_controls",
                                 "Tone / Engine Response controls tied to audible effect."));
        steps.push_back(makeStep("Step 15: Engine Signal Flow",
                                 "Move to Engine / Signal Flow for algorithm routing context before fine tuning.",
                                 3, 0, -1, -1, "engine_visual",
                                 "Engine first subtab: routing context."));
        steps.push_back(makeStep("Step 16: Engine Identity",
                                 "Engine Identity shows which algorithm family is active and what behavior to expect from it.",
                                 3, 1, -1, -1, "engine_visual",
                                 "Engine second subtab: Identity."));
        steps.push_back(makeStep("Step 17: Engine Comparison Table",
                                 "Quick engine map:\nGreen: dynamic/envelope-reactive.\nBlue: modern/clean focus.\nRed NQ: BBD vintage grit.\nRed HQ: tape-style modulation.\nPurple: complex/bi-mod motion.\nBlack: intense/edge and ensemble variants.",
                                 3, 1, -1, -1, "engine_visual",
                                 "Plain comparison for choosing an engine."));
        steps.push_back(makeStep("Step 18: Macro Definition",
                                 "Macro means high-level control that steers multiple lower-level parameters together. Use Engine / Macros for broad tonal movement quickly.",
                                 3, 2, -1, -1, "engine_inspector",
                                 "Engine / Macros: musical high-level controls."));
        steps.push_back(makeStep("Step 19: Internals Definition",
                                 "Internals means low-level algorithm parameters. Use Engine / Internals only when macro-level shaping is not enough.",
                                 3, 0, -1, -1, "engine_inspector",
                                 "Engine / Internals for fine-grain algorithm tuning."));
        steps.push_back(makeStep("Step 20: Look & Feel Mapping",
                                 "Look & Feel / Mapping changes UI-to-DSP behavior (how controls map), not DSP algorithm code itself.",
                                 4, 0, -1, -1, "layout_active_inspector",
                                 "Look & Feel / Mapping controls."));
        steps.push_back(makeStep("Step 21: Look & Feel UI Feel",
                                 "UI Feel adjusts interaction response and smoothing feel for editing ergonomics.",
                                 4, 1, -1, -1, "layout_active_inspector",
                                 "Look & Feel / UI Feel controls."));
        steps.push_back(makeStep("Step 22: Look & Feel Engine Layout",
                                 "Engine Layout sets per-engine visual placement/sizing in the main UI.",
                                 4, 2, -1, -1, "layout_active_inspector",
                                 "Look & Feel / Engine Layout."));
        steps.push_back(makeStep("Step 23: Look & Feel Global Layout",
                                 "Global Layout adjusts shared UI coordinates/styles such as top buttons, engine selector, and global knob response.",
                                 4, 3, -1, -1, "layout_active_inspector",
                                 "Look & Feel / Global Layout."));
        steps.push_back(makeStep("Step 24: Look & Feel Text Animations",
                                 "Text Animations controls the glow/reflect and flip motion systems for main, color, and mix value readouts.",
                                 4, 4, -1, -1, "layout_active_inspector",
                                 "Look & Feel / Text Animations."));
        steps.push_back(makeStep("Step 25: Telemetry Overview",
                                 "Validation / Telemetry shows process time, peaks, callback/write counters, and mode transitions. This is your runtime health dashboard.",
                                 5, 0, -1, -1, "validation_readouts",
                                 "Validation / Telemetry: live health data."));
        steps.push_back(makeStep("Step 26: Telemetry Green/Yellow/Red",
                                 "Use traffic-light guidance:\nGreen: process_ms_peak < 50% of audio-block budget.\nYellow: 50-80% of budget.\nRed: >80% or spikes near/over budget.\nBudget(ms)=bufferSize/sampleRate*1000.",
                                 5, 0, -1, -1, "validation_readouts",
                                 "Interpret process timing against block budget."));
        steps.push_back(makeStep("Step 27: Write-Rate Guidance",
                                 "Parameter write-rate guide:\nGreen: writes/callback usually <=1.\nYellow: sustained ~2-4 writes/callback.\nRed: sustained >4 writes/callback or bursty spikes. Investigate automation density/smoothing.",
                                 5, 0, -1, -1, "validation_readouts",
                                 "Use callback/write ratio to spot automation pressure."));
        steps.push_back(makeStep("Step 28: Trace Matrix Definitions",
                                 "Trace Matrix terms:\nraw = host/UI raw parameter value.\nmapped = scaled musical value.\nsnapshot = active profile stored value.\neffective = runtime value currently used by DSP.",
                                 5, 1, -1, -1, "trace_matrix",
                                 "Validation / Trace Matrix term definitions."));
        steps.push_back(makeStep("Step 29: Console Command Surface",
                                 "Validation / Console core commands: `engine`, `hq`, `view`, `set/get/reset`, `undo/redo/history`, `stats`, `list`, `search`, `watch`, `dump`, `diff factory`, `export script`, `import script`.",
                                 5, 2, -1, -1, "validation_console",
                                 "Console command vocabulary."));
        steps.push_back(makeStep("Step 30: Console Workflow",
                                 "Fast safe workflow:\n1) `search <term>`\n2) `watch <slug>`\n3) `set` / `add` / `sub`\n4) `undo` if needed\n5) `diff factory` and `stats` to validate.",
                                 5, 2, -1, -1, "validation_console",
                                 "Console build loop while listening."));
        steps.push_back(makeStep("Step 31: Persistence Safety",
                                 "Persistence safety:\n- `cp json`: copy current state snapshot.\n- `export script`: portable editable command list.\n- `save defaults`: changes startup behavior (high impact).\n- `reset factory`: destructive reset; use only after backup/export.",
                                 6, 0, -1, -1, "settings_actions",
                                 "Know safe vs destructive persistence actions."));
        steps.push_back(makeStep("Step 32: Import/Apply Workflow",
                                 "Safe apply order:\n1) backup with `cp json` or `export script`.\n2) try edits in session.\n3) validate telemetry + trace matrix.\n4) only then `save defaults`.",
                                 6, 0, -1, -1, "settings_actions",
                                 "Persistence workflow with rollback safety."));
        steps.push_back(makeStep("Step 33: End-to-End Recipe Check",
                                 "Build recipe recap: choose profile -> shape motion -> shape tone -> validate runtime -> persist safely. If results differ between projects, re-check host sample rate, buffer size, and automation density first.",
                                 0, 0, -1, -1, "overview_visual",
                                 "Complete loop: build, validate, persist."));
        steps.push_back(makeStep("Step 34: Return to Overview",
                                 "Tutorial complete. You are back at Overview / Signal Flow. Use this as your home base before each new tuning pass.",
                                 0, 0, -1, -1, "overview_visual",
                                 "Back to Overview after full pass."));
        return steps;
    }

    if (topic == "overview")
    {
        resolvedTopic = "overview";
        resolvedTitle = "Overview Primer";
        steps.push_back(makeStep("Overview: Signal Context",
                                 "This view gives a fast signal-level snapshot before deep tuning. Start here when checking whether profile changes are reaching motion and tone stages.",
                                 0, 0, -1, -1, "overview_visual",
                                 "Overview cards for immediate context."));
        steps.push_back(makeStep("Overview: Inspector Readouts",
                                 "Open the left inspector sections in Overview to validate the current profile readouts before moving into deep tuning tabs.",
                                 0, 0, -1, -1, "overview_inspector",
                                 "Overview inspector readouts/controls."));
        steps.push_back(makeStep("Overview: Delay Visual Subtab",
                                 "Use the second-level Delay Visual subtab for quick movement sanity checks before modulation edits.",
                                 0, 1, -1, -1, "overview_visual",
                                 "Overview second-level subtab: Delay Visual."));
        return steps;
    }

    if (topic == "modulation")
    {
        resolvedTopic = "modulation";
        resolvedTitle = "Modulation Deep Dive";
        steps.push_back(makeStep("Modulation: LFO Shape",
                                 "The LFO is the movement engine. Use Rate and Depth to establish macro motion shape and speed.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "LFO scope and controls."));
        steps.push_back(makeStep("Modulation: Readouts + Workbench",
                                 "The inspector keeps passive readouts, while active edits now live in the LFO Control Workbench below the motion visualizers.",
                                 1, 0, -1, -1, "modulation_controls",
                                 "Readouts left, controls in the unified workbench."));
        steps.push_back(makeStep("Modulation: Left/Right Comparison",
                                 "Use the two-up LFO cards to compare stereo-side motion in one glance instead of switching subtabs.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "Two-up LFO cards for stereo motion checks."));
        steps.push_back(makeStep("Modulation: Stability Controls",
                                 "Trajectory limits and smoothing guard against abrupt delay snaps. Tune them for motion that is rich but stable.",
                                 1, 0, -1, -1, "modulation_visual",
                                 "Trajectory card below LFO Left/Right."));
        return steps;
    }

    if (topic == "tone")
    {
        resolvedTopic = "tone";
        resolvedTitle = "Tone Deep Dive";
        steps.push_back(makeStep("Tone: Spectral Control",
                                 "Use LPF/HPF, pre-emphasis, and dynamics controls to place chorus texture around the source. Keep upper swish under control while preserving clarity.",
                                 2, 0, -1, -1, "tone_visual",
                                 "Tone visual deck and spectrum card."));
        steps.push_back(makeStep("Tone: Inspector Readouts + Controls",
                                 "The tone inspector readouts and controls map what you hear to concrete filter, dynamics, and engine-response parameters.",
                                 2, 0, -1, -1, "tone_readouts",
                                 "Tone inspector sections in Spectrum subtab."));
        steps.push_back(makeStep("Tone: Engine Response",
                                 "The Engine Response curve reflects the active engine mode. Red emphasizes saturation/tape transfer; non-red modes show their own response behavior and control set.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Engine Response curve and dynamic behavior."));
        steps.push_back(makeStep("Tone: Engine Response Subtab Controls",
                                 "On the Engine Response subtab, use controls while watching curve shape to tune the active engine's tone behavior safely.",
                                 2, 1, -1, -1, "tone_controls",
                                 "Tone controls in Engine Response subtab."));
        return steps;
    }

    if (topic == "engine")
    {
        resolvedTopic = "engine";
        resolvedTitle = "Engine Deep Dive";
        steps.push_back(makeStep("Engine: Profile Selection",
                                 "Engine color and HQ mode define the core algorithm family. Switching profile immediately retargets the controls in this panel.",
                                 3, 0, -1, -1, "engine_selector",
                                 "Profile selector and HQ toggle."));
        steps.push_back(makeStep("Engine: Signal Flow Subtab",
                                 "Start on signal flow to get routing context before adjusting internals.",
                                 3, 0, -1, -1, "engine_visual",
                                 "Engine second-level subtab: Signal Flow."));
        steps.push_back(makeStep("Engine: Macro Subtab",
                                 "Use the macro subtab to inspect profile macro behavior and response ranges.",
                                 3, 2, -1, -1, "engine_inspector",
                                 "Engine second-level subtab: Macros."));
        steps.push_back(makeStep("Engine: Internals",
                                 "Use the engine internals inspector for profile-specific tuning. This is where each algorithm's identity is sculpted.",
                                 3, 0, -1, -1, "engine_inspector",
                                 "Engine internals inspector."));
        return steps;
    }

    if (topic == "validation")
    {
        resolvedTopic = "validation";
        resolvedTitle = "Validation Deep Dive";
        steps.push_back(makeStep("Validation: Telemetry Subtab",
                                 "Start with telemetry readouts to inspect callback timing, write activity, and profile/mode transitions.",
                                 5, 0, -1, -1, "validation_readouts",
                                 "Validation second-level subtab: Telemetry."));
        steps.push_back(makeStep("Validation: Trace Matrix",
                                 "The trace matrix confirms that UI movement maps correctly into profile values and effective runtime values.",
                                 5, 1, -1, -1, "trace_matrix",
                                 "Trace matrix in validation visual deck."));
        steps.push_back(makeStep("Validation: Interactive Console",
                                 "Use the console for scripted state edits, history, diff checks, and automation-friendly diagnostics while audio is running.",
                                 5, 2, -1, -1, "validation_console",
                                 "Validation console card."));
        return steps;
    }

    if (topic == "bbd")
    {
        resolvedTopic = "bbd";
        resolvedTitle = "Bucket-Brigade Lesson";
        steps.push_back(makeStep("Step 1: Force Red NQ",
                                 "BBD behavior lives in the Red NQ profile. This lesson switches there so every control and visual reflects analog-style bucket-brigade behavior.",
                                 3, 0, 2, 0, "engine_selector",
                                 "Profile selector: Red with HQ off."));
        steps.push_back(makeStep("Step 2: BBD Stages",
                                 "Stages act like the number of buckets in the line. Stage count shifts delay behavior and can reshape texture and bandwidth.",
                                 3, 0, -1, -1, "bbd_controls",
                                 "BBD sections in engine internals."));
        steps.push_back(makeStep("Step 3: Clock Speed",
                                 "Clock speed sets bucket handoff rate. Slower clocks enable longer delay feel but with more vintage artifacts.",
                                 3, 0, -1, -1, "bbd_controls",
                                 "Clock-related controls in engine internals."));
        steps.push_back(makeStep("Step 4: Watch High-End Noise",
                                 "As clock slows, high-frequency whine and aliasing risk can increase. Use the spectrum view to observe that change while tuning.",
                                 2, 0, -1, -1, "spectrum_visual",
                                 "Spectrum analyzer in Tone tab."));
        steps.push_back(makeStep("Step 5: Anti-Alias and Reconstruction Filters",
                                 "Steeper low-pass filtering hides clock artifacts. Raising cutoff can brighten tone but may reintroduce noise.",
                                 3, 0, -1, -1, "bbd_controls",
                                 "Filter controls around the BBD core."));
        steps.push_back(makeStep("Step 6: Balance Clarity vs Character",
                                 "BBD tuning is a balance: brighter settings improve clarity, darker settings suppress artifacts and emphasize vintage character.",
                                 3, 0, -1, -1, "engine_inspector",
                                 "BBD tuning sections in inspector."));
        return steps;
    }

    if (topic == "tape")
    {
        resolvedTopic = "tape";
        resolvedTitle = "Magnetic Tape Lesson";
        steps.push_back(makeStep("Step 1: Force Red HQ",
                                 "Tape behavior lives in Red HQ. This switches profile so tape wow, flutter, and tape-tone controls are active.",
                                 3, 0, 2, 1, "engine_selector",
                                 "Profile selector: Red with HQ on."));
        steps.push_back(makeStep("Step 2: Tape Wow",
                                 "Wow is the slow drift caused by mechanical instability. It creates broad pitch movement that feels organic and worn.",
                                 3, 0, -1, -1, "tape_controls",
                                 "Tape wow controls in engine internals."));
        steps.push_back(makeStep("Step 3: Tape Flutter",
                                 "Flutter is faster jitter from tape/head friction. Blend wow and flutter carefully to avoid distracting warble.",
                                 3, 0, -1, -1, "tape_controls",
                                 "Tape flutter controls in engine internals."));
        steps.push_back(makeStep("Step 4: Saturation Context",
                                 "Tape tone depends on both modulation and non-linear drive. Move to Tone to evaluate harmonic behavior with analyzer visuals.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Engine Response curve in Tone tab."));
        steps.push_back(makeStep("Step 5: Tape Drive",
                                 "Tape Drive controls how hard the signal is pushed into the virtual medium. More drive adds density and natural compression.",
                                 3, 0, -1, -1, "tape_controls",
                                 "Tape drive and gain controls."));
        steps.push_back(makeStep("Step 6: Observe Soft Limiting",
                                 "As drive increases, peaks round rather than hard-clip. This thickens chorus tails while staying smoother than abrupt clipping.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Engine Response curve while increasing drive."));
        steps.push_back(makeStep("Step 7: Age/Tone LPF",
                                 "Lower LPF cutoff simulates darker, older tape. Raise it for cleaner top-end at the cost of less vintage softness.",
                                 2, 0, -1, -1, "tone_inspector",
                                 "LPF and tone-shaping controls in inspector."));
        return steps;
    }

    if (topic == "phase")
    {
        resolvedTopic = "phase";
        resolvedTitle = "Stereo Width Lesson";
        steps.push_back(makeStep("Step 1: Dual Delay Concept",
                                 "Stereo chorus uses left and right modulation paths. Phase relationships between those paths drive width perception.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "Modulation scope for left/right motion."));
        steps.push_back(makeStep("Step 2: Read the Scope",
                                 "The scope shows how motion evolves in time. Identical movement on both sides reads narrower than offset movement.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "LFO scope in modulation visual deck."));
        steps.push_back(makeStep("Step 3: Width Offset",
                                 "Width offset sets phase relationship between left and right trajectories. It is the key stereo-width lever.",
                                 1, 0, -1, -1, "modulation_inspector",
                                 "Width/offset controls in inspector."));
        steps.push_back(makeStep("Step 4: 180 Degree Anti-Phase",
                                 "At roughly 180 degrees, one side stretches while the other compresses. This creates strong, enveloping stereo motion.",
                                 1, 0, -1, -1, "modulation_inspector",
                                 "Offset controls while monitoring scope."));
        steps.push_back(makeStep("Step 5: Musical Balance",
                                 "Maximum width is not always best. Tune offset for size while preserving mono compatibility and center stability.",
                                 1, 0, -1, -1, "modulation_visual",
                                 "Scope plus width controls together."));
        return steps;
    }

    if (topic == "bimodulation" || topic == "bi-modulation" || topic == "bimod" || topic == "warp")
    {
        resolvedTopic = "bimodulation";
        resolvedTitle = "Complex Motion Lesson";
        steps.push_back(makeStep("Step 1: Force Purple NQ",
                                 "Purple NQ emphasizes warp-style bi-mod behavior. This lesson switches there to expose dual-LFO style controls.",
                                 3, 0, 3, 0, "engine_selector",
                                 "Profile selector: Purple with HQ off."));
        steps.push_back(makeStep("Step 2: Two Independent Modulators",
                                 "Bi-modulation layers two LFO paths so motion stops sounding predictably periodic.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "LFO scope showing composite motion."));
        steps.push_back(makeStep("Step 3: Non-Related Speeds",
                                 "Set unrelated rates so cycles rarely align. This prevents obvious repeating sweeps and keeps motion evolving.",
                                 1, 0, -1, -1, "modulation_inspector",
                                 "Rate controls for independent modulators."));
        steps.push_back(makeStep("Step 4: Compound Shape",
                                 "When two modulation paths interact, the resulting delay trajectory becomes richer and less mechanical.",
                                 1, 0, -1, -1, "lfo_scope",
                                 "Right/left LFO views for compound behavior."));
        steps.push_back(makeStep("Step 5: Organic Motion",
                                 "Use this mode for living, animated chorus textures where predictable whoosh would feel static.",
                                 3, 0, -1, -1, "engine_inspector",
                                 "Purple warp internals in engine tab."));
        return steps;
    }

    if (topic == "saturation")
    {
        resolvedTopic = "saturation";
        resolvedTitle = "Harmonics Lesson";
        steps.push_back(makeStep("Step 1: Saturation Fundamentals",
                                 "Saturation is controlled clipping that reshapes waveform peaks and adds harmonics. Start on Tone / Engine Response with a Red mode selected.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Engine Response curve visualizer."));
        steps.push_back(makeStep("Step 2: Linear Reference",
                                 "A straight transfer line means input equals output. Deviations show where harmonic coloration starts.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Engine Response baseline reference."));
        steps.push_back(makeStep("Step 3: Increase Drive",
                                 "Raising drive pushes signal into non-linear regions. Watch the curve bend as harmonic content increases.",
                                 2, 1, -1, -1, "tone_inspector",
                                 "Drive controls in tone inspector."));
        steps.push_back(makeStep("Step 4: Hard Clip Context",
                                 "Switching to Black profile emphasizes harder clipping behavior. This creates stronger odd harmonics and bite.",
                                 2, 1, 4, -1, "engine_selector",
                                 "Profile selector for black engine contrast."));
        steps.push_back(makeStep("Step 5: Hard Clip Character",
                                 "Hard clipping flattens peaks quickly and can sound aggressive. Useful for edge, but easy to overdo.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Flattened transfer tops in black mode."));
        steps.push_back(makeStep("Step 6: Return to Tape Soft Clip",
                                 "Switch back to Red HQ tape for softer limiting. Soft clipping rounds peaks more gradually.",
                                 2, 1, 2, 1, "engine_selector",
                                 "Profile selector back to red HQ."));
        steps.push_back(makeStep("Step 7: Soft Clip Warmth",
                                 "Soft clipping adds density while staying smoother in transients. Use it for warm tails without brittle harshness.",
                                 2, 1, -1, -1, "transfer_visual",
                                 "Rounded transfer curve with tape behavior."));
        return steps;
    }

    if (topic == "envelope" || topic == "dynamic" || topic == "dynamics")
    {
        resolvedTopic = "envelope";
        resolvedTitle = "Dynamic Lesson";
        steps.push_back(makeStep("Step 1: Force Green HQ",
                                 "Green HQ is optimized for dynamic response workflows. This lesson switches there for envelope-focused tuning.",
                                 3, 0, 0, 1, "engine_selector",
                                 "Profile selector: Green HQ."));
        steps.push_back(makeStep("Step 2: Envelope Follower",
                                 "The envelope follower tracks incoming level over time. It can modulate chorus behavior dynamically from the source performance.",
                                 3, 0, -1, -1, "engine_inspector",
                                 "Envelope-related controls in engine internals."));
        steps.push_back(makeStep("Step 3: Detector Speed",
                                 "Attack and release set how quickly the detector responds and recovers. Fast settings react instantly; slower settings smooth behavior.",
                                 3, 0, -1, -1, "engine_inspector",
                                 "Detector attack/release controls."));
        steps.push_back(makeStep("Step 4: Route to Modulation Depth",
                                 "Once detected, envelope motion can drive depth so chorus intensity follows performance energy.",
                                 1, 0, -1, -1, "modulation_inspector",
                                 "Depth and response controls in modulation."));
        steps.push_back(makeStep("Step 5: Amount Control",
                                 "Envelope amount sets how strongly level changes influence modulation depth.",
                                 3, 0, -1, -1, "engine_inspector",
                                 "Envelope amount and scaling controls."));
        steps.push_back(makeStep("Step 6: Musical Result",
                                 "Dynamic chorus breathes with the source: more intensity on loud phrases, less movement on quiet passages.",
                                 0, 0, -1, -1, "overview_visual",
                                 "Confirm behavior through overview visuals."));
        return steps;
    }

    return {};
}
