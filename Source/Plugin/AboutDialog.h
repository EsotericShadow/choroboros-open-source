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
