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

//==============================================================================
/**
    Distinguishes between different contexts in which preset state is applied.

    Each context has different semantics for listeners, preset invalidation,
    validation strictness, and analytics tracking.

    - HostRestore: DAW loading plugin state (e.g., session restore, undo/redo)
    - UserPresetLoad: User loaded a user-saved preset
    - FactoryPresetLoad: User loaded a factory preset
    - Migration: Migrating legacy preset format or applying defaults
*/
enum class ApplyContext
{
    HostRestore,        // Host requesting state via setStateInformation()
    UserPresetLoad,     // User loaded a user-saved preset
    FactoryPresetLoad,  // User loaded a factory preset
    Migration,          // Migrating legacy format or applying defaults
};
