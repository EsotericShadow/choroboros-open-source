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
        case 1: return 3; // Modulation
        case 2: return 2; // Tone
        case 3: return 4; // Engine
        case 4: return 4; // Look & Feel
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
            switch (safeSubTab)
            {
                case 0: return "LFO Left";
                case 1: return "LFO Right";
                case 2: default: return "Trajectory";
            }
        case 2:
            switch (safeSubTab)
            {
                case 0: return "Spectrum";
                case 1: default: return "Transfer";
            }
        case 3:
        {
            static const juce::String engineNames[] { "Green", "Blue", "Red", "Purple", "Black" };
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
            const juce::String internalsLabel = engineNames[engine] + (hq ? " HQ Internals" : " NQ Internals");
            switch (safeSubTab)
            {
                case 0: return "Signal Flow";
                case 1: return "Engine Identity";
                case 2: return macroLabel;
                case 3: default: return internalsLabel;
            }
        }
        case 4:
            switch (safeSubTab)
            {
                case 0: return "Mapping";
                case 1: return "UI Feel";
                case 2: return "Engine Layout";
                case 3: default: return "Global Layout";
            }
        case 5:
            switch (safeSubTab)
            {
                case 0: return "Telemetry";
                case 1: return "Trace Matrix";
                case 2: default: return "Live Log";
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
    juce::TextButton* buttons[4] = { &subTabButtonA, &subTabButtonB, &subTabButtonC, &subTabButtonD };
    const int safeMainTab = juce::jlimit(0, 6, selectedRightTab);
    const int subTabCount = getSubTabCount(safeMainTab);
    selectedSubTabs[static_cast<size_t>(safeMainTab)] =
        juce::jlimit(0, juce::jmax(0, subTabCount - 1), selectedSubTabs[static_cast<size_t>(safeMainTab)]);
    const int selectedSubTab = getSelectedSubTab();

    for (int i = 0; i < 4; ++i)
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
    auto bounds = getLocalBounds().reduced(14);
    viewport.setBounds(bounds);

    const int columnGap = 28;
    const int contentWidth = juce::jmax(320, bounds.getWidth() - viewport.getScrollBarThickness());
    const float leftColumnRatio = contentWidth >= 1500 ? 0.29f : 0.31f;
    int leftColumnWidth = juce::jlimit(260, 430, static_cast<int>(std::round(contentWidth * leftColumnRatio)));
    int rightColumnWidth = contentWidth - columnGap - leftColumnWidth;
    constexpr int minRightColumnWidth = 540;
    if (rightColumnWidth < minRightColumnWidth)
    {
        const int widthDeficit = minRightColumnWidth - rightColumnWidth;
        leftColumnWidth = juce::jmax(260, leftColumnWidth - widthDeficit);
        rightColumnWidth = contentWidth - columnGap - leftColumnWidth;
    }
    const int leftX = 0;
    const int rightX = leftColumnWidth + columnGap;

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

    int leftY = 0;
    int rightY = 0;
    const int selectedSubTab = getSelectedSubTab();

    const int tabHeight = 38;
    const int tabGap = 6;
    const int tabButtonWidth = juce::jmax(82, (rightColumnWidth - (tabGap * 6)) / 7);
    int tabX = rightX;
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
    const int subTabButtonWidth = juce::jmax(108, (rightColumnWidth - (subTabGap * juce::jmax(0, subTabCount - 1))) / juce::jmax(1, subTabCount));
    juce::TextButton* subTabButtons[4] = { &subTabButtonA, &subTabButtonB, &subTabButtonC, &subTabButtonD };
    int subTabX = rightX;
    for (int i = 0; i < 4; ++i)
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

    const int activeProfileH = 26;
    activeProfileLabel.setBounds(rightX, rightY, rightColumnWidth, activeProfileH);
    rightY += activeProfileH + 4;
    const int scopeHintH = 22;
    activeScopeHintLabel.setBounds(rightX, rightY, rightColumnWidth, scopeHintH);
    rightY += scopeHintH + 12;

    const int toolsH = 32;
    const int modeLabelW = 48;
    const int modeComboW = 136;
    const int modeHqW = 64;
    const int modeGap = 8;
    devEngineModeLabel.setBounds(rightX, rightY + 4, modeLabelW, toolsH - 4);
    devEngineModeBox.setBounds(rightX + modeLabelW + modeGap, rightY, modeComboW, toolsH);
    devHqModeToggle.setBounds(rightX + modeLabelW + modeGap + modeComboW + modeGap, rightY, modeHqW, toolsH);
    rightY += toolsH + 14;

    const bool showRightTools = selectedRightTab == 3 && selectedSubTab == 3;
    if (showRightTools)
    {
        const int labelW = 126;
        const int clearW = 76;
        const int toolsGap = 8;
        const int editorX = rightX + labelW + toolsGap;
        const int editorRight = rightX + rightColumnWidth - clearW - toolsGap;
        const int editorW = juce::jmax(140, editorRight - editorX);
        engineFilterLabel.setBounds(rightX, rightY + 4, labelW, toolsH - 4);
        engineFilterEditor.setBounds(editorX, rightY, editorW, toolsH);
        engineFilterClearButton.setBounds(rightX + rightColumnWidth - clearW, rightY, clearW, toolsH);
        rightY += toolsH + 18;
    }
    else
    {
        engineFilterLabel.setBounds(0, 0, 0, 0);
        engineFilterEditor.setBounds(0, 0, 0, 0);
        engineFilterClearButton.setBounds(0, 0, 0, 0);
    }

    auto selectedDeckCards = [](const juce::Array<juce::PropertyComponent*>& source, int selectedIndex)
    {
        juce::Array<juce::PropertyComponent*> result;
        if (juce::isPositiveAndBelow(selectedIndex, source.size()))
            result.add(source[selectedIndex]);
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
    inspectorDescription.setText(inspectorScope + " controls/readouts. Expand sections to edit values.",
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
        if (selectedSubTab == 3)
        {
            if (auto* activeInternalsPanel = getActiveEngineInternalsPanel())
                layoutPanelOnly(leftX, leftColumnWidth, leftY, *activeInternalsPanel, compactSection);
            if (bbdPanel.isVisible())
                layoutPanelOnly(leftX, leftColumnWidth, leftY, bbdPanel, compactSection);
            if (tapePanel.isVisible())
                layoutPanelOnly(leftX, leftColumnWidth, leftY, tapePanel, compactSection);
        }
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
        else
            layoutPanelOnly(leftX, leftColumnWidth, leftY, layoutGlobalPanel, compactSection);
    }
    else if (selectedRightTab == 5)
    {
        layoutPanelOnly(leftX, leftColumnWidth, leftY, validationPanel, compactSection);
    }
    else if (selectedRightTab == 6)
    {
        // Settings uses right-column action groups; keep inspector as context-only.
    }

    if (selectedRightTab == 0)
    {
        layoutSectionHeader(rightX, rightColumnWidth, rightY, overviewTitle, overviewDescription, visualSection);
        auto cards = selectedDeckCards(overviewVisualDeckCards, selectedSubTab);
        layoutVisualDeck(rightX, rightColumnWidth, rightY, overviewVisualDeck, cards, overviewVisualDeckCards, overviewDeckSpacing);
    }
    else if (selectedRightTab == 1)
    {
        layoutSectionHeader(rightX, rightColumnWidth, rightY, modulationTitle, modulationDescription, visualSection);
        auto cards = selectedDeckCards(modulationVisualDeckCards, selectedSubTab);
        layoutVisualDeck(rightX, rightColumnWidth, rightY, modulationVisualDeck, cards, modulationVisualDeckCards, modulationDeckSpacing);
    }
    else if (selectedRightTab == 2)
    {
        layoutSectionHeader(rightX, rightColumnWidth, rightY, toneTitle, toneDescription, visualSection);
        auto cards = selectedDeckCards(toneVisualDeckCards, selectedSubTab);
        layoutVisualDeck(rightX, rightColumnWidth, rightY, toneVisualDeck, cards, toneVisualDeckCards, toneDeckSpacing);
    }
    else if (selectedRightTab == 3)
    {
        layoutSectionHeader(rightX, rightColumnWidth, rightY, engineTitle, engineDescription, visualSection);
        auto cards = selectedDeckCards(engineVisualDeckCards, 0);
        layoutVisualDeck(rightX, rightColumnWidth, rightY, engineVisualDeck, cards, engineVisualDeckCards, engineDeckSpacing);
    }
    else if (selectedRightTab == 4)
    {
        layoutSectionHeader(rightX, rightColumnWidth, rightY, layoutTitle, layoutDescription, visualSection);
        auto cards = selectedDeckCards(lookFeelVisualDeckCards, 0);
        layoutVisualDeck(rightX, rightColumnWidth, rightY, lookFeelVisualDeck, cards, lookFeelVisualDeckCards, lookFeelDeckSpacing);
    }
    else if (selectedRightTab == 5)
    {
        layoutSectionHeader(rightX, rightColumnWidth, rightY, validationTitle, validationDescription, visualSection);
        auto cards = selectedDeckCards(validationVisualDeckCards, selectedSubTab);
        layoutVisualDeck(rightX, rightColumnWidth, rightY, validationVisualDeck, cards, validationVisualDeckCards, lookFeelDeckSpacing);
    }
    else if (selectedRightTab == 6)
    {
        layoutSectionHeader(rightX, rightColumnWidth, rightY, settingsTitle, settingsDescription, visualSection);

        const int buttonHeight = 38;
        const int buttonGap = 12;
        const int firstRowWidth = (rightColumnWidth - (buttonGap * 2)) / 3;
        int rowY = rightY;
        int rowX = rightX;
        fxPresetOffButton.setBounds(rowX, rowY, firstRowWidth, buttonHeight);
        rowX += firstRowWidth + buttonGap;
        fxPresetSubtleButton.setBounds(rowX, rowY, firstRowWidth, buttonHeight);
        rowX += firstRowWidth + buttonGap;
        fxPresetMediumButton.setBounds(rowX, rowY, rightX + rightColumnWidth - rowX, buttonHeight);

        rowY += buttonHeight + 14;
        const int secondRowWidth = (rightColumnWidth - (buttonGap * 3)) / 4;
        rowX = rightX;
        lockToggleButton.setBounds(rowX, rowY, secondRowWidth, buttonHeight);
        rowX += secondRowWidth + buttonGap;
        resetFactoryButton.setBounds(rowX, rowY, secondRowWidth, buttonHeight);
        rowX += secondRowWidth + buttonGap;
        saveDefaultsButton.setBounds(rowX, rowY, secondRowWidth, buttonHeight);
        rowX += secondRowWidth + buttonGap;
        copyJsonButton.setBounds(rowX, rowY, rightX + rightColumnWidth - rowX, buttonHeight);

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

    const int contentHeight = std::max(leftY, rightY) + 16;
    content.setBounds(0, 0, contentWidth, contentHeight);

    const int maxViewX = juce::jmax(0, content.getWidth() - viewport.getViewWidth());
    const int maxViewY = juce::jmax(0, content.getHeight() - viewport.getViewHeight());
    viewport.setViewPosition(juce::jlimit(0, maxViewX, previousViewPos.x),
                             juce::jlimit(0, maxViewY, previousViewPos.y));
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
    styleProfileSelectorComboBox(devEngineModeBox, profileSelectorColourForEngineIndex(colorIndex));
    styleHackerToggleButton(devHqModeToggle);
    styleHackerTextButton(copyJsonButton, false);
    styleHackerTextButton(saveDefaultsButton, false);
    styleHackerTextButton(resetFactoryButton, false);
    styleHackerTextButton(lockToggleButton, false);
    styleHackerTextButton(fxPresetOffButton, false);
    styleHackerTextButton(fxPresetSubtleButton, false);
    styleHackerTextButton(fxPresetMediumButton, false);
    styleHackerTextButton(engineFilterClearButton, false);
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
    const bool showEngine = selectedRightTab == 3 && getSelectedSubTab() == 3;
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
    layoutGreenPanel.setVisible(showLayoutGreen);
    layoutBluePanel.setVisible(showLayoutBlue);
    layoutRedPanel.setVisible(showLayoutRed);
    layoutPurplePanel.setVisible(showLayoutPurple);
    layoutBlackPanel.setVisible(showLayoutBlack);
    activeProfileLabel.setVisible(true);
    activeScopeHintLabel.setVisible(true);
    devEngineModeLabel.setVisible(true);
    devEngineModeBox.setVisible(true);
    devHqModeToggle.setVisible(true);
    const bool showEngineInternalsTools = showEngine && selectedSubTab == 3;
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

    internalsTitle.setVisible(showEngine && selectedSubTab == 3);
    internalsDescription.setVisible(showEngine && selectedSubTab == 3);
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
    setAllSectionsEnabled(mappingPanel, showLookFeelMapping);
    setAllSectionsEnabled(uiPanel, showLookFeelUi);
    setAllSectionsEnabled(layoutGlobalPanel, showLookFeelGlobalLayout);
    setAllSectionsEnabled(layoutGreenPanel, showLayoutGreen);
    setAllSectionsEnabled(layoutBluePanel, showLayoutBlue);
    setAllSectionsEnabled(layoutRedPanel, showLayoutRed);
    setAllSectionsEnabled(layoutPurplePanel, showLayoutPurple);
    setAllSectionsEnabled(layoutBlackPanel, showLayoutBlack);
    const auto* activeInternalsPanel = (showEngine && selectedSubTab == 3) ? getActiveEngineInternalsPanel() : nullptr;
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
    setAllSectionsEnabled(bbdPanel, showEngine && selectedSubTab == 3 && activeInternalsPanel == &internalsRedNqPanel);
    setAllSectionsEnabled(tapePanel, showEngine && selectedSubTab == 3 && activeInternalsPanel == &internalsRedHqPanel);

    if (showEngine && selectedSubTab == 3)
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

    bbdTitle.setVisible(showEngine && selectedSubTab == 3 && bbdPanel.isVisible());
    bbdDescription.setVisible(showEngine && selectedSubTab == 3 && bbdPanel.isVisible());
    tapeTitle.setVisible(showEngine && selectedSubTab == 3 && tapePanel.isVisible());
    tapeDescription.setVisible(showEngine && selectedSubTab == 3 && tapePanel.isVisible());

    tabOverviewButton.setToggleState(showOverview, juce::dontSendNotification);
    tabInternalsButton.setToggleState(showModulation, juce::dontSendNotification);
    tabBbdButton.setToggleState(showTone, juce::dontSendNotification);
    tabTapeButton.setToggleState(showEngine, juce::dontSendNotification);
    tabLayoutButton.setToggleState(showLookFeel, juce::dontSendNotification);
    tabValidationButton.setToggleState(showValidation, juce::dontSendNotification);
    tabSettingsButton.setToggleState(showSettings, juce::dontSendNotification);

    updateAnalyzerDemandFromVisibility();
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
        for (int i = recentTouchHistory.size() - 1; i >= 0; --i)
        {
            if (recentTouchHistory[i].startsWithIgnoreCase(prefix))
                recentTouchHistory.remove(i);
        }
        recentTouchHistory.insert(0, prefix + " " + valueText + " @ " + timestamp);
        while (recentTouchHistory.size() > 10)
            recentTouchHistory.remove(recentTouchHistory.size() - 1);
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
