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

#include <juce_core/juce_core.h>

/**
 * Manages user consent preferences for analytics and diagnostics.
 *
 * Consent is read once at construction and cached in memory.
 * Default is OFF for both analytics and diagnostics (opt-in model).
 * PropertiesFile is written only when preferences are explicitly changed.
 */
class ConsentService
{
public:
    ConsentService();

    // Query methods (fast in-memory reads)
    bool isAnalyticsEnabled() const noexcept { return analyticsEnabled_; }
    bool isDiagnosticsEnabled() const noexcept { return diagnosticsEnabled_; }

    // Setter methods (write to PropertiesFile and update cache)
    void setAnalyticsEnabled(bool enabled);
    void setDiagnosticsEnabled(bool enabled);

private:
    bool analyticsEnabled_ = false;    // Default OFF
    bool diagnosticsEnabled_ = false;  // Default OFF

    void loadFromPropertiesFile();
    void saveToPropertiesFile();
    juce::File getSettingsFile() const;
};
