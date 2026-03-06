#pragma once

#include "BinaryData.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <unordered_set>
#include <vector>

namespace devpanel
{
struct ControlMetadata
{
    juce::String id;
    juce::String label;
    juce::String stage;
    std::uint16_t appliesToMask = 0x03ffu; // 5 engines * 2 modes.
    juce::String rawToMappedHook;
    juce::String effectiveProbeHook;
    juce::String visualCardId;
    juce::String noVisualReason;
};

struct VisualCardLayoutSpec
{
    int headerPx = 18;
    int visualMinPx = 96;
    int controlsMinPx = 84;
};

constexpr VisualCardLayoutSpec kSparklineLayout { 24, 160, 110 };
constexpr VisualCardLayoutSpec kTransferLayout { 24, 190, 120 };
constexpr VisualCardLayoutSpec kSpectrumLayout { 24, 190, 120 };
constexpr VisualCardLayoutSpec kSignalFlowLayout { 26, 230, 136 };

namespace StagePalette
{
constexpr juce::uint32 overview = 0xff80ef80;
constexpr juce::uint32 modulation = 0xff80ef80;
constexpr juce::uint32 tone = 0xff80ef80;
constexpr juce::uint32 engine = 0xff80ef80;
constexpr juce::uint32 validation = 0xffffff;
constexpr juce::uint32 skinGreen = 0xff80ef80;
constexpr juce::uint32 skinBlue = 0xff7fb8ff;
constexpr juce::uint32 skinRed = 0xffff8d8b;
constexpr juce::uint32 skinPurple = 0xffb88dd8;
constexpr juce::uint32 skinBlack = 0xffffffff;
constexpr juce::uint32 live = 0xff9aff9a;
constexpr juce::uint32 frozen = 0xff6fcb6f;
constexpr juce::uint32 warning = 0xff80ef80;
constexpr juce::uint32 danger = 0xffb8ffb8;
constexpr juce::uint32 nodata = 0xff4f8f4f;
} // namespace StagePalette

namespace HackerTheme
{
inline juce::uint32 text = 0xff80ef80;
inline juce::uint32 textDim = 0xff63bf63;
inline juce::uint32 textMuted = 0xff4f8f4f;
inline juce::uint32 bg = 0xff040704;
inline juce::uint32 bgElevated = 0xff0b120b;
inline juce::uint32 bgField = 0xff0f1810;
inline juce::uint32 bgActive = 0xff143019;
inline juce::uint32 border = 0xff2f6f3a;
inline juce::uint32 borderStrong = 0xff4fa85f;
inline juce::uint32 grid = 0xff1d351f;
inline juce::uint32 surface = 0xff0a120d;
inline juce::uint32 surfaceAlt = 0xff101a14;
inline juce::uint32 wash = 0xff112114;
} // namespace HackerTheme

namespace VisualizerPalette
{
constexpr juce::uint32 overview = 0xff6fb4ff;
constexpr juce::uint32 modulation = 0xfff4a640;
constexpr juce::uint32 tone = 0xffb88dd8;
constexpr juce::uint32 engine = 0xffff8d8b;
constexpr juce::uint32 validation = 0xff73d08e;
constexpr juce::uint32 layout = 0xff6fd4d0;
constexpr juce::uint32 neutral = 0xff6f879f;
} // namespace VisualizerPalette

namespace Typography
{
constexpr float label = 12.0f;
constexpr float labelSmall = 11.0f;
constexpr float title = 17.5f;
constexpr float description = 13.5f;
constexpr float sectionHeader = 13.0f;
constexpr float button = 12.0f;
constexpr float combo = 12.0f;
constexpr float tooltip = 12.0f;
constexpr float propertyLabel = 12.0f;
constexpr float inspectorInput = 12.0f;
constexpr float inspectorReadout = 12.0f;
constexpr float inspectorReadoutMulti = 12.0f;
constexpr float signalFlowHeader = 12.0f;
constexpr float signalFlowRow = 12.0f;
constexpr float vizTitle = 10.2f;
constexpr float vizBody = 8.5f;
constexpr float vizSmall = 8.2f;
constexpr float vizTiny = 7.8f;
constexpr float consoleLog = 11.5f;
} // namespace Typography

enum class LinkGroup : int
{
    none = 0,
    overview = 1,
    modulation = 2,
    tone = 3,
    engine = 4,
    validation = 5
};

inline LinkGroup gLinkedGroup = LinkGroup::none;
inline juce::Colour gCurrentEngineSkinColour { juce::Colour(StagePalette::skinGreen) };
inline juce::String gActiveEngineSectionPrefix;
inline bool gStripActiveEngineSectionPrefix = false;
inline float gDevPanelUserTextScale = 1.0f;
inline int gDevPanelTextColourMode = 0; // 0=auto, 1=high-contrast, 2=white, 3=cyan, 4=amber
inline int gDevPanelThemePreset = 0; // 0=engine adaptive, 1=classic hacker, 2=neutral studio, 3=high contrast
inline int gDevPanelAccentSource = 0; // 0=follow engine, 1=manual
inline int gDevPanelManualAccentIndex = 0;
inline int gDevPanelColourVisionMode = 0; // 0=off, 1=deuteranopia, 2=protanopia, 3=tritanopia
inline bool gDevPanelReducedMotion = false;
inline bool gDevPanelLargeHitTargets = false;
inline bool gDevPanelStrongFocusRing = true;

juce::Font makeLabelFont(float height, bool bold, bool italic = false);
juce::Font makeTitleFont(float height, bool bold = true);
juce::Font makeTooltipFont(float height, bool italic = false);
juce::Font makeRetroFont(float height, bool bold);
void styleHackerTextButton(juce::TextButton& button, bool active = false);
void styleHackerToggleButton(juce::ToggleButton& toggle);
void styleHackerComboBox(juce::ComboBox& box);
void styleHackerEditor(juce::TextEditor& editor);

inline juce::Colour engineSkinColourForIndex(int engineIndex)
{
    switch (juce::jlimit(0, 4, engineIndex))
    {
        case 0: return juce::Colour(StagePalette::skinGreen);
        case 1: return juce::Colour(StagePalette::skinBlue);
        case 2: return juce::Colour(StagePalette::skinRed);
        case 3: return juce::Colour(StagePalette::skinPurple);
        case 4: default: return juce::Colour(StagePalette::skinBlack);
    }
}

inline float devPanelEffectiveTextScale() { return 1.10f * juce::jlimit(0.85f, 1.35f, gDevPanelUserTextScale); }

inline void setDevPanelUserTextScale(float scale) { gDevPanelUserTextScale = juce::jlimit(0.85f, 1.35f, scale); }
inline void setDevPanelTextColourMode(int mode) { gDevPanelTextColourMode = juce::jlimit(0, 4, mode); }
inline void setDevPanelThemePreset(int preset) { gDevPanelThemePreset = juce::jlimit(0, 3, preset); }
inline void setDevPanelAccentSource(int source) { gDevPanelAccentSource = juce::jlimit(0, 1, source); }
inline void setDevPanelManualAccentIndex(int index) { gDevPanelManualAccentIndex = juce::jlimit(0, 4, index); }
inline void setDevPanelColourVisionMode(int mode) { gDevPanelColourVisionMode = juce::jlimit(0, 3, mode); }
inline void setDevPanelReducedMotion(bool enabled) { gDevPanelReducedMotion = enabled; }
inline void setDevPanelLargeHitTargets(bool enabled) { gDevPanelLargeHitTargets = enabled; }
inline void setDevPanelStrongFocusRing(bool enabled) { gDevPanelStrongFocusRing = enabled; }
inline bool isDevPanelReducedMotionEnabled() { return gDevPanelReducedMotion; }
inline bool isDevPanelLargeHitTargetsEnabled() { return gDevPanelLargeHitTargets; }

inline juce::Colour applyColourVisionMode(juce::Colour accent)
{
    if (gDevPanelColourVisionMode <= 0)
        return accent;

    const float hueShift = (gDevPanelColourVisionMode == 1 ? 0.08f
                           : (gDevPanelColourVisionMode == 2 ? 0.14f : 0.32f));
    const float h = std::fmod(accent.getHue() + hueShift, 1.0f);
    const float s = juce::jlimit(0.30f, 0.95f, accent.getSaturation() * 0.78f);
    const float v = juce::jlimit(0.60f, 1.0f, accent.getBrightness() * 1.03f);
    return juce::Colour::fromHSV(h, s, v, 1.0f);
}

inline juce::Colour resolveEffectiveThemeAccent(int engineIndex)
{
    juce::Colour accent = engineSkinColourForIndex(engineIndex);

    if (gDevPanelThemePreset == 1)       accent = juce::Colour(StagePalette::skinGreen);
    else if (gDevPanelThemePreset == 2)  accent = juce::Colour(0xff8fd7cf);
    else if (gDevPanelThemePreset == 3)  accent = juce::Colours::white;

    // Manual accent must be a true override regardless of theme preset.
    if (gDevPanelAccentSource == 1)
        accent = engineSkinColourForIndex(gDevPanelManualAccentIndex);

    return applyColourVisionMode(accent);
}

inline void applyHackerThemeAccent(juce::Colour accent)
{
    const bool highContrastTheme = (gDevPanelThemePreset == 3);
    const auto baseBg = highContrastTheme ? juce::Colour(0xff030303) : juce::Colour(0xff040704);
    const auto baseElevated = highContrastTheme ? juce::Colour(0xff090909) : juce::Colour(0xff0b120b);
    const auto baseField = highContrastTheme ? juce::Colour(0xff0e0e0e) : juce::Colour(0xff0f1810);
    const auto baseSurface = highContrastTheme ? juce::Colour(0xff0a0a0a) : juce::Colour(0xff0a120d);
    const auto baseSurfaceAlt = highContrastTheme ? juce::Colour(0xff121212) : juce::Colour(0xff101a14);

    const auto tone = accent.withAlpha(1.0f);
    const auto toneDim = tone.interpolatedWith(juce::Colours::black, 0.28f);
    const auto toneMuted = tone.interpolatedWith(juce::Colours::black, 0.46f);
    const auto toneBorder = tone.interpolatedWith(juce::Colours::black, 0.62f);
    auto toneBorderStrong = tone.interpolatedWith(juce::Colours::black, 0.44f);
    if (gDevPanelStrongFocusRing || highContrastTheme)
        toneBorderStrong = toneBorderStrong.brighter(0.18f);
    const auto toneGrid = tone.interpolatedWith(juce::Colours::black, 0.76f);

    HackerTheme::text = tone.getARGB();
    HackerTheme::textDim = toneDim.getARGB();
    HackerTheme::textMuted = toneMuted.getARGB();
    HackerTheme::bg = baseBg.getARGB();
    HackerTheme::bgElevated = baseElevated.interpolatedWith(tone, 0.05f).getARGB();
    HackerTheme::bgField = baseField.interpolatedWith(tone, 0.08f).getARGB();
    HackerTheme::bgActive = juce::Colour(0xff122014).interpolatedWith(tone, 0.16f).getARGB();
    HackerTheme::border = toneBorder.getARGB();
    HackerTheme::borderStrong = toneBorderStrong.getARGB();
    HackerTheme::grid = toneGrid.getARGB();
    HackerTheme::surface = baseSurface.interpolatedWith(tone, 0.07f).getARGB();
    HackerTheme::surfaceAlt = baseSurfaceAlt.interpolatedWith(tone, 0.09f).getARGB();
    HackerTheme::wash = juce::Colour(0xff112114).interpolatedWith(tone, 0.20f).getARGB();

    if (gDevPanelTextColourMode == 1 || highContrastTheme)
    {
        HackerTheme::text = juce::Colours::white.getARGB();
        HackerTheme::textDim = juce::Colour(0xffd6d6d6).getARGB();
        HackerTheme::textMuted = juce::Colour(0xffb3b3b3).getARGB();
    }
    else if (gDevPanelTextColourMode == 2)
    {
        const auto t = juce::Colours::white;
        HackerTheme::text = t.getARGB();
        HackerTheme::textDim = t.darker(0.22f).getARGB();
        HackerTheme::textMuted = t.darker(0.38f).getARGB();
    }
    else if (gDevPanelTextColourMode == 3)
    {
        const auto t = juce::Colour(0xff9ae6ff);
        HackerTheme::text = t.getARGB();
        HackerTheme::textDim = t.darker(0.24f).getARGB();
        HackerTheme::textMuted = t.darker(0.40f).getARGB();
    }
    else if (gDevPanelTextColourMode == 4)
    {
        const auto t = juce::Colour(0xffffc978);
        HackerTheme::text = t.getARGB();
        HackerTheme::textDim = t.darker(0.26f).getARGB();
        HackerTheme::textMuted = t.darker(0.42f).getARGB();
    }
}

inline void setCurrentEngineSkinColour(int engineIndex)
{
    gCurrentEngineSkinColour = resolveEffectiveThemeAccent(engineIndex);
    applyHackerThemeAccent(gCurrentEngineSkinColour);
}

inline juce::Colour blendSectionWithSkin(juce::Colour sectionBase)
{
    return sectionBase.interpolatedWith(gCurrentEngineSkinColour, 0.10f);
}

inline void setLinkedGroup(LinkGroup group)
{
    gLinkedGroup = group;
}

inline void setActiveEngineSectionPrefix(juce::String prefix, bool shouldStrip)
{
    gActiveEngineSectionPrefix = std::move(prefix);
    gStripActiveEngineSectionPrefix = shouldStrip;
}

inline juce::String normalizeSectionHeader(const juce::String& sectionName)
{
    if (gStripActiveEngineSectionPrefix
        && gActiveEngineSectionPrefix.isNotEmpty()
        && sectionName.startsWithIgnoreCase(gActiveEngineSectionPrefix))
    {
        const juce::String stripped = sectionName.substring(gActiveEngineSectionPrefix.length()).trimStart();
        if (stripped.isNotEmpty())
            return stripped;
    }
    return sectionName;
}

inline LinkGroup linkedGroupForSectionName(const juce::String& sectionName)
{
    if (sectionName.startsWithIgnoreCase("Active Parameters"))
        return LinkGroup::overview;
    if (sectionName.startsWithIgnoreCase("LFO Controls"))
        return LinkGroup::modulation;
    if (sectionName.startsWithIgnoreCase("LFO Readouts"))
        return LinkGroup::modulation;
    if (sectionName.startsWithIgnoreCase("Tone Readouts"))
        return LinkGroup::tone;
    if (sectionName.startsWithIgnoreCase("Active Engine"))
        return LinkGroup::engine;
    if (sectionName.startsWithIgnoreCase("Live Telemetry"))
        return LinkGroup::validation;
    return LinkGroup::none;
}

inline juce::Colour sectionAccent(LinkGroup group)
{
    juce::ignoreUnused(group);
    return juce::Colour(HackerTheme::text);
}

inline juce::Colour hackerText() { return juce::Colour(HackerTheme::text); }
inline juce::Colour hackerTextDim() { return juce::Colour(HackerTheme::textDim); }
inline juce::Colour hackerTextMuted() { return juce::Colour(HackerTheme::textMuted); }
inline juce::Colour hackerBg() { return juce::Colour(HackerTheme::bg); }
inline juce::Colour hackerBgElevated() { return juce::Colour(HackerTheme::bgElevated); }
inline juce::Colour hackerBgField() { return juce::Colour(HackerTheme::bgField); }
inline juce::Colour hackerBgActive() { return juce::Colour(HackerTheme::bgActive); }
inline juce::Colour hackerBorder() { return juce::Colour(HackerTheme::border); }
inline juce::Colour hackerBorderStrong() { return juce::Colour(HackerTheme::borderStrong); }
inline juce::Colour hackerGrid() { return juce::Colour(HackerTheme::grid); }
inline juce::Colour hackerSurface() { return juce::Colour(HackerTheme::surface); }
inline juce::Colour hackerSurfaceAlt() { return juce::Colour(HackerTheme::surfaceAlt); }
inline juce::Colour hackerWash() { return juce::Colour(HackerTheme::wash); }
inline juce::Colour visualOverview() { return juce::Colour(VisualizerPalette::overview); }
inline juce::Colour visualModulation() { return juce::Colour(VisualizerPalette::modulation); }
inline juce::Colour visualTone() { return juce::Colour(VisualizerPalette::tone); }
inline juce::Colour visualEngine() { return juce::Colour(VisualizerPalette::engine); }
inline juce::Colour visualValidation() { return juce::Colour(VisualizerPalette::validation); }
inline juce::Colour visualLayout() { return juce::Colour(VisualizerPalette::layout); }
inline juce::Colour visualNeutral() { return juce::Colour(VisualizerPalette::neutral); }

inline void styleHackerTextButton(juce::TextButton& button, bool active)
{
    button.setColour(juce::TextButton::buttonColourId, active ? hackerBgActive() : hackerBgElevated());
    button.setColour(juce::TextButton::buttonOnColourId, hackerBgActive());
    button.setColour(juce::TextButton::textColourOffId, active ? hackerText() : hackerTextDim());
    button.setColour(juce::TextButton::textColourOnId, hackerText());
}

inline void styleHackerToggleButton(juce::ToggleButton& toggle)
{
    toggle.setColour(juce::ToggleButton::tickColourId, hackerText());
    toggle.setColour(juce::ToggleButton::tickDisabledColourId, hackerTextMuted());
    toggle.setColour(juce::ToggleButton::textColourId, hackerText());
}

inline void styleHackerComboBox(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, hackerBgField());
    box.setColour(juce::ComboBox::buttonColourId, hackerBgField());
    box.setColour(juce::ComboBox::textColourId, hackerText());
    box.setColour(juce::ComboBox::outlineColourId, hackerBorderStrong());
    box.setColour(juce::ComboBox::arrowColourId, hackerText());
}

inline juce::Colour profileSelectorColourForEngineIndex(int engineIndex)
{
    return engineSkinColourForIndex(engineIndex);
}

inline juce::Colour profileSelectorColourForText(const juce::String& text)
{
    if (text.equalsIgnoreCase("Green")) return profileSelectorColourForEngineIndex(0);
    if (text.equalsIgnoreCase("Blue")) return profileSelectorColourForEngineIndex(1);
    if (text.equalsIgnoreCase("Red")) return profileSelectorColourForEngineIndex(2);
    if (text.equalsIgnoreCase("Purple")) return profileSelectorColourForEngineIndex(3);
    if (text.equalsIgnoreCase("Black")) return profileSelectorColourForEngineIndex(4);
    return {};
}

inline void styleProfileSelectorComboBox(juce::ComboBox& box, juce::Colour accent)
{
    styleHackerComboBox(box);
    box.setColour(juce::ComboBox::textColourId, accent);
    box.setColour(juce::ComboBox::outlineColourId, accent.withAlpha(0.88f));
    box.setColour(juce::ComboBox::arrowColourId, accent);
}

inline void styleHackerEditor(juce::TextEditor& editor)
{
    editor.setColour(juce::TextEditor::backgroundColourId, hackerBgField());
    editor.setColour(juce::TextEditor::outlineColourId, hackerBorder());
    editor.setColour(juce::TextEditor::focusedOutlineColourId, hackerBorderStrong());
    editor.setColour(juce::TextEditor::textColourId, hackerText());
    editor.setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.85f));
}

inline void drawStatusBadge(juce::Graphics& g, juce::Rectangle<float> header, const juce::String& text, juce::Colour colour)
{
    const auto pad = 6.0f;
    const auto badgeW = juce::jmax(46.0f, static_cast<float>(text.length() * 7) + 16.0f);
    auto badge = juce::Rectangle<float>(header.getRight() - badgeW - 2.0f, header.getY() + 1.0f, badgeW, 14.0f);
    g.setColour(colour.withAlpha(0.24f));
    g.fillRoundedRectangle(badge, 7.0f);
    g.setColour(colour.withAlpha(0.75f));
    g.drawRoundedRectangle(badge, 7.0f, 1.0f);
    badge.reduce(pad * 0.5f, 0.0f);
    g.setColour(colour.brighter(0.35f));
    g.setFont(makeRetroFont(Typography::vizSmall, true));
    g.drawText(text, badge.toNearestInt(), juce::Justification::centred, false);
}

class DevPanelSectionLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DevPanelSectionLookAndFeel()
    {
        refreshThemeColours();
    }

    void refreshThemeColours()
    {
        setColour(juce::PropertyComponent::backgroundColourId, hackerSurfaceAlt());
        setColour(juce::PropertyComponent::labelTextColourId, hackerText());
        setColour(juce::Label::textColourId, hackerText());
        setColour(juce::TextButton::buttonColourId, hackerBgField());
        setColour(juce::TextButton::buttonOnColourId, hackerBgActive());
        setColour(juce::TextButton::textColourOffId, hackerText());
        setColour(juce::TextButton::textColourOnId, hackerText());
        setColour(juce::ToggleButton::textColourId, hackerText());
        setColour(juce::ToggleButton::tickColourId, hackerText());
        setColour(juce::ToggleButton::tickDisabledColourId, hackerTextMuted());
        setColour(juce::ComboBox::backgroundColourId, hackerBgField());
        setColour(juce::ComboBox::buttonColourId, hackerBgField());
        setColour(juce::ComboBox::textColourId, hackerText());
        setColour(juce::ComboBox::outlineColourId, hackerBorderStrong());
        setColour(juce::ComboBox::arrowColourId, hackerText());
        setColour(juce::PopupMenu::backgroundColourId, hackerBgElevated());
        setColour(juce::PopupMenu::textColourId, hackerText());
        setColour(juce::PopupMenu::highlightedBackgroundColourId, hackerBgActive());
        setColour(juce::PopupMenu::highlightedTextColourId, hackerText());
        setColour(juce::TextEditor::backgroundColourId, hackerBgField());
        setColour(juce::TextEditor::textColourId, hackerText());
        setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.85f));
        setColour(juce::TextEditor::outlineColourId, hackerBorder());
        setColour(juce::TextEditor::focusedOutlineColourId, hackerBorderStrong());
    }

    int getPropertyPanelSectionHeaderHeight(const juce::String&) override
    {
        return 32;
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        juce::ignoreUnused(label);
        return makeLabelFont(Typography::label, false);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        juce::ignoreUnused(buttonHeight);
        return makeTitleFont(Typography::button, true);
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        juce::ignoreUnused(box);
        return makeLabelFont(Typography::combo, false);
    }

    juce::Font getPopupMenuFont() override
    {
        return makeLabelFont(Typography::label, false);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& /*backgroundColour*/,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto base = button.findColour(button.getToggleState()
                                          ? juce::TextButton::buttonOnColourId
                                          : juce::TextButton::buttonColourId);
        if (shouldDrawButtonAsDown)
            base = base.brighter(0.20f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter(0.08f);

        const float alpha = button.isEnabled() ? 1.0f : 0.55f;
        g.setColour(base.withMultipliedAlpha(alpha));
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(hackerBorderStrong().withMultipliedAlpha((button.hasKeyboardFocus(true) ? 1.0f : 0.78f) * alpha));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        const auto boxSize = juce::jmin(16.0f, button.getHeight() * 0.62f);
        const auto box = juce::Rectangle<float>(4.0f, (button.getHeight() - boxSize) * 0.5f, boxSize, boxSize);
        auto boxFill = hackerBgField();
        if (button.getToggleState())
            boxFill = hackerBgActive();
        if (shouldDrawButtonAsDown)
            boxFill = boxFill.brighter(0.20f);
        else if (shouldDrawButtonAsHighlighted)
            boxFill = boxFill.brighter(0.08f);

        const float alpha = button.isEnabled() ? 1.0f : 0.55f;
        g.setColour(boxFill.withMultipliedAlpha(alpha));
        g.fillRoundedRectangle(box, 3.0f);
        g.setColour((button.getToggleState() ? hackerBorderStrong() : hackerBorder()).withMultipliedAlpha(alpha));
        g.drawRoundedRectangle(box, 3.0f, 1.0f);

        if (button.getToggleState())
        {
            juce::Path check;
            check.startNewSubPath(box.getX() + box.getWidth() * 0.20f, box.getCentreY());
            check.lineTo(box.getX() + box.getWidth() * 0.43f, box.getBottom() - box.getHeight() * 0.20f);
            check.lineTo(box.getRight() - box.getWidth() * 0.18f, box.getY() + box.getHeight() * 0.22f);
            g.setColour(hackerText().withMultipliedAlpha(alpha));
            g.strokePath(check, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour(button.findColour(juce::ToggleButton::textColourId)
                        .withMultipliedAlpha(alpha));
        g.setFont(makeTitleFont(Typography::button, true));
        g.drawFittedText(button.getButtonText(),
                         juce::Rectangle<int>(static_cast<int>(box.getRight() + 6.0f),
                                              0,
                                              juce::jmax(1, button.getWidth() - static_cast<int>(box.getRight() + 8.0f)),
                                              button.getHeight()),
                         juce::Justification::centredLeft,
                         1);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1));
        auto fill = box.findColour(juce::ComboBox::backgroundColourId);
        if (isButtonDown)
            fill = fill.brighter(0.20f);
        else if (box.isMouseOverOrDragging())
            fill = fill.brighter(0.08f);
        if (!box.isEnabled())
            fill = fill.withMultipliedAlpha(0.55f);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(box.findColour(juce::ComboBox::outlineColourId).withMultipliedAlpha(box.isEnabled() ? 0.92f : 0.55f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        auto arrowZone = juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                                static_cast<float>(buttonW), static_cast<float>(buttonH)).reduced(4.0f, 4.0f);
        juce::Path arrow;
        const float cx = arrowZone.getCentreX();
        const float cy = arrowZone.getCentreY();
        const float aw = juce::jmin(8.0f, arrowZone.getWidth() * 0.35f);
        const float ah = juce::jmin(5.0f, arrowZone.getHeight() * 0.35f);
        arrow.startNewSubPath(cx - aw, cy - ah * 0.5f);
        arrow.lineTo(cx, cy + ah * 0.5f);
        arrow.lineTo(cx + aw, cy - ah * 0.5f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withMultipliedAlpha(box.isEnabled() ? 0.95f : 0.55f));
        g.strokePath(arrow, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(3, 1, juce::jmax(1, box.getWidth() - box.getHeight() - 4), juce::jmax(1, box.getHeight() - 2));
        label.setFont(getComboBoxFont(box));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setBorderSize(juce::BorderSize<int>(0, 6, 0, 4));
        label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label.setColour(juce::Label::textColourId,
                        box.findColour(juce::ComboBox::textColourId).withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.55f));
    }

    void drawPropertyComponentBackground(juce::Graphics& g, int width, int height, juce::PropertyComponent& component) override
    {
        const auto base = component.isMouseOverOrDragging() ? hackerBgActive().withAlpha(0.45f) : hackerSurfaceAlt();
        g.setColour(base);
        g.fillRect(0, 0, width, height - 1);
        g.setColour(hackerBorder().withAlpha(0.60f));
        g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));
    }

    void drawPropertyComponentLabel(juce::Graphics& g, int, int height, juce::PropertyComponent& component) override
    {
        const auto indent = juce::jmin(10, component.getWidth() / 10);
        const auto r = getPropertyComponentContentPosition(component);
        g.setColour((component.isEnabled() ? hackerText() : hackerTextMuted())
                        .withMultipliedAlpha(component.isMouseOverOrDragging() ? 1.0f : 0.94f));
        g.setFont(makeLabelFont(Typography::propertyLabel, true));
        g.drawFittedText(component.getName(),
                         indent, r.getY(), juce::jmax(1, r.getX() - indent - 6), r.getHeight(),
                         juce::Justification::centredLeft, 1);
    }

    juce::Rectangle<int> getPropertyComponentContentPosition(juce::PropertyComponent& component) override
    {
        auto bounds = component.getLocalBounds();
        const int valueWidth = juce::jlimit(106, 156, juce::roundToInt(static_cast<float>(bounds.getWidth()) * 0.40f));
        return bounds.removeFromRight(valueWidth).reduced(1, 1);
    }

    void drawPropertyPanelSectionHeader(juce::Graphics& g, const juce::String& name,
                                        bool isOpen, int width, int height) override
    {
        const juce::String displayName = normalizeSectionHeader(name);
        const LinkGroup sectionGroup = linkedGroupForSectionName(name);
        const bool linkedActive = sectionGroup != LinkGroup::none && sectionGroup == gLinkedGroup;
        auto headerBounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

        g.setColour(hackerBgElevated());
        g.fillRect(headerBounds);

        if (linkedActive && isOpen)
        {
            auto bg = juce::Rectangle<float>(1.0f, 1.0f, static_cast<float>(width - 2), static_cast<float>(height - 2));
            const auto accent = sectionAccent(sectionGroup);
            g.setColour(accent.withAlpha(0.14f));
            g.fillRoundedRectangle(bg, 4.0f);
            g.setColour(accent.withAlpha(0.42f));
            g.drawRoundedRectangle(bg, 4.0f, 1.0f);
        }

        const auto arrowArea = juce::Rectangle<float>(7.0f, (static_cast<float>(height) - 10.0f) * 0.5f, 10.0f, 10.0f);
        juce::Path arrow;
        if (isOpen)
        {
            arrow.startNewSubPath(arrowArea.getX(), arrowArea.getY());
            arrow.lineTo(arrowArea.getRight(), arrowArea.getY());
            arrow.lineTo(arrowArea.getCentreX(), arrowArea.getBottom());
        }
        else
        {
            arrow.startNewSubPath(arrowArea.getX(), arrowArea.getY());
            arrow.lineTo(arrowArea.getRight(), arrowArea.getCentreY());
            arrow.lineTo(arrowArea.getX(), arrowArea.getBottom());
        }
        arrow.closeSubPath();
        g.setColour(hackerText().withAlpha(0.96f));
        g.fillPath(arrow);

        const int textX = static_cast<int>(arrowArea.getRight()) + 8;
        g.setColour(hackerText().withAlpha(linkedActive ? 1.0f : 0.96f));
        g.setFont(makeTitleFont(Typography::sectionHeader, true));
        g.drawText(displayName, textX, 0, juce::jmax(1, width - textX - 4), height, juce::Justification::centredLeft, true);

        g.setColour(hackerBorder().withAlpha(0.72f));
        g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));

        if (linkedActive && isOpen)
        {
            auto outline = juce::Rectangle<float>(1.0f, 1.0f, static_cast<float>(width - 2), static_cast<float>(height - 2));
            g.setColour(sectionAccent(sectionGroup).withAlpha(0.48f));
            g.drawRoundedRectangle(outline, 4.0f, 1.0f);
        }
    }
};

inline DevPanelSectionLookAndFeel& getDevPanelSectionLookAndFeel()
{
    static DevPanelSectionLookAndFeel lf;
    return lf;
}

class DevPanelThemeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DevPanelThemeLookAndFeel()
    {
        refreshThemeColours();
    }

    void refreshThemeColours()
    {
        setColour(juce::PropertyComponent::backgroundColourId, hackerSurfaceAlt());
        setColour(juce::PropertyComponent::labelTextColourId, hackerText());
        setColour(juce::Label::textColourId, hackerText());
        setColour(juce::TextButton::buttonColourId, hackerBgElevated());
        setColour(juce::TextButton::buttonOnColourId, hackerBgActive());
        setColour(juce::TextButton::textColourOffId, hackerText());
        setColour(juce::TextButton::textColourOnId, hackerText());
        setColour(juce::ToggleButton::textColourId, hackerText());
        setColour(juce::ToggleButton::tickColourId, hackerText());
        setColour(juce::ToggleButton::tickDisabledColourId, hackerTextMuted());
        setColour(juce::ComboBox::backgroundColourId, hackerBgField());
        setColour(juce::ComboBox::buttonColourId, hackerBgField());
        setColour(juce::ComboBox::textColourId, hackerText());
        setColour(juce::ComboBox::outlineColourId, hackerBorderStrong());
        setColour(juce::ComboBox::arrowColourId, hackerText());
        setColour(juce::PopupMenu::backgroundColourId, hackerBgElevated());
        setColour(juce::PopupMenu::textColourId, hackerText());
        setColour(juce::PopupMenu::highlightedBackgroundColourId, hackerBgActive());
        setColour(juce::PopupMenu::highlightedTextColourId, hackerText());
        setColour(juce::TextEditor::backgroundColourId, hackerBgField());
        setColour(juce::TextEditor::textColourId, hackerText());
        setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.85f));
        setColour(juce::TextEditor::outlineColourId, hackerBorder());
        setColour(juce::TextEditor::focusedOutlineColourId, hackerBorderStrong());
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        juce::ignoreUnused(label);
        return makeLabelFont(Typography::label, false);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        juce::ignoreUnused(buttonHeight);
        return makeTitleFont(Typography::button, true);
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        juce::ignoreUnused(box);
        return makeLabelFont(Typography::combo, false);
    }

    juce::Font getPopupMenuFont() override
    {
        return makeLabelFont(Typography::label, false);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& /*backgroundColour*/,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto base = button.findColour(button.getToggleState()
                                          ? juce::TextButton::buttonOnColourId
                                          : juce::TextButton::buttonColourId);
        if (shouldDrawButtonAsDown)
            base = base.brighter(0.20f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter(0.08f);

        const float alpha = button.isEnabled() ? 1.0f : 0.55f;
        g.setColour(base.withMultipliedAlpha(alpha));
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(hackerBorderStrong().withMultipliedAlpha((button.hasKeyboardFocus(true) ? 1.0f : 0.78f) * alpha));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        const auto boxSize = juce::jmin(16.0f, button.getHeight() * 0.62f);
        const auto box = juce::Rectangle<float>(4.0f, (button.getHeight() - boxSize) * 0.5f, boxSize, boxSize);
        auto boxFill = hackerBgField();
        if (button.getToggleState())
            boxFill = hackerBgActive();
        if (shouldDrawButtonAsDown)
            boxFill = boxFill.brighter(0.20f);
        else if (shouldDrawButtonAsHighlighted)
            boxFill = boxFill.brighter(0.08f);

        const float alpha = button.isEnabled() ? 1.0f : 0.55f;
        g.setColour(boxFill.withMultipliedAlpha(alpha));
        g.fillRoundedRectangle(box, 3.0f);
        g.setColour((button.getToggleState() ? hackerBorderStrong() : hackerBorder()).withMultipliedAlpha(alpha));
        g.drawRoundedRectangle(box, 3.0f, 1.0f);

        if (button.getToggleState())
        {
            juce::Path check;
            check.startNewSubPath(box.getX() + box.getWidth() * 0.20f, box.getCentreY());
            check.lineTo(box.getX() + box.getWidth() * 0.43f, box.getBottom() - box.getHeight() * 0.20f);
            check.lineTo(box.getRight() - box.getWidth() * 0.18f, box.getY() + box.getHeight() * 0.22f);
            g.setColour(hackerText().withMultipliedAlpha(alpha));
            g.strokePath(check, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour(button.findColour(juce::ToggleButton::textColourId)
                        .withMultipliedAlpha(alpha));
        g.setFont(makeTitleFont(Typography::button, true));
        g.drawFittedText(button.getButtonText(),
                         juce::Rectangle<int>(static_cast<int>(box.getRight() + 6.0f),
                                              0,
                                              juce::jmax(1, button.getWidth() - static_cast<int>(box.getRight() + 8.0f)),
                                              button.getHeight()),
                         juce::Justification::centredLeft,
                         1);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1));
        auto fill = box.findColour(juce::ComboBox::backgroundColourId);
        if (isButtonDown)
            fill = fill.brighter(0.20f);
        else if (box.isMouseOverOrDragging())
            fill = fill.brighter(0.08f);
        if (!box.isEnabled())
            fill = fill.withMultipliedAlpha(0.55f);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(box.findColour(juce::ComboBox::outlineColourId).withMultipliedAlpha(box.isEnabled() ? 0.92f : 0.55f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        auto arrowZone = juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                                static_cast<float>(buttonW), static_cast<float>(buttonH)).reduced(4.0f, 4.0f);
        juce::Path arrow;
        const float cx = arrowZone.getCentreX();
        const float cy = arrowZone.getCentreY();
        const float aw = juce::jmin(8.0f, arrowZone.getWidth() * 0.35f);
        const float ah = juce::jmin(5.0f, arrowZone.getHeight() * 0.35f);
        arrow.startNewSubPath(cx - aw, cy - ah * 0.5f);
        arrow.lineTo(cx, cy + ah * 0.5f);
        arrow.lineTo(cx + aw, cy - ah * 0.5f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withMultipliedAlpha(box.isEnabled() ? 0.95f : 0.55f));
        g.strokePath(arrow, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(3, 1, juce::jmax(1, box.getWidth() - box.getHeight() - 4), juce::jmax(1, box.getHeight() - 2));
        label.setFont(getComboBoxFont(box));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setBorderSize(juce::BorderSize<int>(0, 6, 0, 4));
        label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label.setColour(juce::Label::textColourId,
                        box.findColour(juce::ComboBox::textColourId).withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.55f));
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool /*shouldDrawButtonAsHighlighted*/,
                        bool /*shouldDrawButtonAsDown*/) override
    {
        auto textColour = button.findColour(button.getToggleState()
                                                ? juce::TextButton::textColourOnId
                                                : juce::TextButton::textColourOffId)
                             .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.55f);
        g.setColour(textColour);
        g.setFont(makeTitleFont(Typography::button, true));
        g.drawFittedText(button.getButtonText(),
                         button.getLocalBounds().reduced(6, 2),
                         juce::Justification::centred,
                         1);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColourToUse) override
    {
        if (isSeparator)
        {
            juce::LookAndFeel_V4::drawPopupMenuItem(g, area, isSeparator, isActive, isHighlighted,
                                                    isTicked, hasSubMenu, text, shortcutKeyText, icon, textColourToUse);
            return;
        }

        const auto profileColour = profileSelectorColourForText(text);
        if (profileColour == juce::Colour())
        {
            juce::LookAndFeel_V4::drawPopupMenuItem(g, area, isSeparator, isActive, isHighlighted,
                                                    isTicked, hasSubMenu, text, shortcutKeyText, icon, textColourToUse);
            return;
        }

        // Draw the standard item chrome/tick/hover state without drawing default text.
        juce::LookAndFeel_V4::drawPopupMenuItem(g, area, isSeparator, isActive, isHighlighted,
                                                isTicked, hasSubMenu, juce::String(), shortcutKeyText, icon, textColourToUse);

        auto textArea = area.reduced(7, 0);
        textArea.removeFromLeft(24); // Tick/icon space.
        textArea.removeFromRight(8);
        if (hasSubMenu)
            textArea.removeFromRight(12);

        auto colour = profileColour;
        if (!isActive)
            colour = colour.withAlpha(0.45f);

        g.setColour(colour);
        g.setFont(getPopupMenuFont());
        g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
    }

    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        auto area = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1));
        g.setColour(hackerBgElevated());
        g.fillRoundedRectangle(area, 4.0f);
        g.setColour(hackerBorderStrong());
        g.drawRoundedRectangle(area, 4.0f, 1.0f);
        g.setColour(hackerText());
        g.setFont(makeTooltipFont(Typography::tooltip));
        g.drawFittedText(text, juce::Rectangle<int>(2, 2, width - 4, height - 4), juce::Justification::centredLeft, 3);
    }
};

inline DevPanelThemeLookAndFeel& getDevPanelThemeLookAndFeel()
{
    static DevPanelThemeLookAndFeel lf;
    return lf;
}

class FloatValueSource : public juce::Value::ValueSource
{
public:
    FloatValueSource(std::function<float()> getterIn, std::function<void(float)> setterIn)
        : getter(std::move(getterIn)), setter(std::move(setterIn)) {}

    juce::var getValue() const override { return getter(); }

    void setValue(const juce::var& newValue) override
    {
        const float value = static_cast<float>(newValue);
        setter(value);
    }

private:
    std::function<float()> getter;
    std::function<void(float)> setter;
};

inline juce::Value makeFloatValue(std::function<float()> getter, std::function<void(float)> setter)
{
    return juce::Value(new FloatValueSource(std::move(getter), std::move(setter)));
}

class BoolValueSource : public juce::Value::ValueSource
{
public:
    BoolValueSource(std::function<bool()> getterIn, std::function<void(bool)> setterIn)
        : getter(std::move(getterIn)), setter(std::move(setterIn)) {}

    juce::var getValue() const override { return getter(); }

    void setValue(const juce::var& newValue) override
    {
        setter(static_cast<bool>(newValue));
    }

private:
    std::function<bool()> getter;
    std::function<void(bool)> setter;
};

inline juce::Value makeBoolValue(std::function<bool()> getter, std::function<void(bool)> setter)
{
    return juce::Value(new BoolValueSource(std::move(getter), std::move(setter)));
}

class IntValueSource : public juce::Value::ValueSource
{
public:
    IntValueSource(std::function<int()> getterIn, std::function<void(int)> setterIn)
        : getter(std::move(getterIn)), setter(std::move(setterIn)) {}

    juce::var getValue() const override { return getter(); }

    void setValue(const juce::var& newValue) override
    {
        setter(static_cast<int>(newValue));
    }

private:
    std::function<int()> getter;
    std::function<void(int)> setter;
};

inline juce::Value makeIntValue(std::function<int()> getter, std::function<void(int)> setter)
{
    return juce::Value(new IntValueSource(std::move(getter), std::move(setter)));
}

constexpr int kDevPanelTimerHz = 30;
constexpr int kDenseReadoutHeight = 156;
constexpr int kTraceMatrixHeight = 520;
constexpr int kRowHeightMicro = 42;
constexpr int kRowHeightCompact = 46;
constexpr int kRowHeightStandard = 52;
constexpr int kRowHeightControl = 48;
inline juce::String formatFloat(float value)
{
    return juce::String(value, 4);
}

inline bool varsEquivalent(const juce::var& a, const juce::var& b, double numberTolerance = 1.0e-6)
{
    if (a.isVoid() || b.isVoid())
        return a.isVoid() == b.isVoid();

    if ((a.isInt() || a.isInt64() || a.isDouble()) && (b.isInt() || b.isInt64() || b.isDouble()))
        return std::abs(static_cast<double>(a) - static_cast<double>(b)) <= numberTolerance;

    if (a.isBool() || b.isBool())
        return static_cast<bool>(a) == static_cast<bool>(b);

    if (a.isString() || b.isString())
        return a.toString() == b.toString();

    if (const auto* arrA = a.getArray())
    {
        const auto* arrB = b.getArray();
        if (arrB == nullptr || arrA->size() != arrB->size())
            return false;
        for (int i = 0; i < arrA->size(); ++i)
        {
            if (!varsEquivalent((*arrA)[i], (*arrB)[i], numberTolerance))
                return false;
        }
        return true;
    }

    const auto* objA = a.getDynamicObject();
    const auto* objB = b.getDynamicObject();
    if (objA != nullptr || objB != nullptr)
    {
        if (objA == nullptr || objB == nullptr)
            return false;
        const auto propsA = objA->getProperties();
        const auto propsB = objB->getProperties();
        if (propsA.size() != propsB.size())
            return false;
        for (int i = 0; i < propsA.size(); ++i)
        {
            const auto key = propsA.getName(i);
            if (!objB->hasProperty(key))
                return false;
            if (!varsEquivalent(propsA.getValueAt(i), objB->getProperty(key), numberTolerance))
                return false;
        }
        return true;
    }

    return a == b;
}

inline juce::Typeface::Ptr loadTypefaceFromNamedResources(std::initializer_list<const char*> resourceNames)
{
    for (const auto* resourceName : resourceNames)
    {
        if (resourceName == nullptr)
            continue;
        int dataSize = 0;
        const auto* data = BinaryData::getNamedResource(resourceName, dataSize);
        if (data != nullptr && dataSize > 0)
            return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(dataSize));
    }
    return {};
}

inline juce::Typeface::Ptr loadTypefaceFromBinary(const char* data, int dataSize)
{
    if (data != nullptr && dataSize > 0)
        return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(dataSize));
    return {};
}

inline juce::Font makeLabelFont(float height, bool bold, bool italic)
{
    const float scaledHeight = height * devPanelEffectiveTextScale();
    static juce::Typeface::Ptr thinTypeface = loadTypefaceFromBinary(BinaryData::JetBrainsMonoNLThin_ttf,
                                                                     BinaryData::JetBrainsMonoNLThin_ttfSize);
    static juce::Typeface::Ptr italicTypeface = loadTypefaceFromBinary(BinaryData::JetBrainsMonoThinItalic_ttf,
                                                                       BinaryData::JetBrainsMonoThinItalic_ttfSize);
    static juce::Typeface::Ptr boldTypeface = loadTypefaceFromBinary(BinaryData::JetBrainsMonoExtraBold_ttf,
                                                                     BinaryData::JetBrainsMonoExtraBold_ttfSize);

    juce::Typeface::Ptr chosen;
    if (italic)
        chosen = italicTypeface != nullptr ? italicTypeface : thinTypeface;
    else if (bold)
        chosen = boldTypeface != nullptr ? boldTypeface : thinTypeface;
    else
        chosen = thinTypeface;

    if (chosen != nullptr)
        return juce::Font { juce::FontOptions { chosen }.withHeight(scaledHeight) };

    // Final fallback only if embedded fonts failed to load.
    return juce::Font { juce::FontOptions { scaledHeight, juce::Font::plain } };
}

inline juce::Font makeTitleFont(float height, bool bold)
{
    const float scaledHeight = height * devPanelEffectiveTextScale();
    juce::ignoreUnused(bold);
    static juce::Typeface::Ptr titleTypeface = loadTypefaceFromBinary(BinaryData::JetBrainsMonoExtraBold_ttf,
                                                                      BinaryData::JetBrainsMonoExtraBold_ttfSize);
    if (titleTypeface != nullptr)
        return juce::Font { juce::FontOptions { titleTypeface }.withHeight(scaledHeight) };
    return makeLabelFont(height, true, false);
}

inline juce::Font makeTooltipFont(float height, bool italic)
{
    const float scaledHeight = height * devPanelEffectiveTextScale();
    static juce::Typeface::Ptr tooltipTypeface = loadTypefaceFromBinary(BinaryData::JetBrainsMonoNLExtraLight_ttf,
                                                                        BinaryData::JetBrainsMonoNLExtraLight_ttfSize);
    static juce::Typeface::Ptr tooltipItalicTypeface = loadTypefaceFromBinary(BinaryData::JetBrainsMonoNLExtraLightItalic_ttf,
                                                                              BinaryData::JetBrainsMonoNLExtraLightItalic_ttfSize);
    auto chosen = italic && tooltipItalicTypeface != nullptr ? tooltipItalicTypeface : tooltipTypeface;
    if (chosen != nullptr)
        return juce::Font { juce::FontOptions { chosen }.withHeight(scaledHeight) };
    return makeLabelFont(height, false, italic);
}

inline juce::Font makeRetroFont(float height, bool bold)
{
    return makeLabelFont(height, bold, false);
}

class LockableFloatPropertyComponent : public juce::PropertyComponent
{
public:
    using ContextMenuHandler = std::function<void(LockableFloatPropertyComponent&, const juce::MouseEvent&)>;
    using UnlockAttemptHandler = std::function<bool(LockableFloatPropertyComponent&)>;

    LockableFloatPropertyComponent(const juce::Value& valueToControl, const juce::String& name,
                                   double min, double max, double step, double skew,
                                   const juce::String& tooltipText)
        : juce::PropertyComponent(name),
          controlledValue(valueToControl),
          minValue(min),
          maxValue(max),
          stepValue(step > 0.0 ? step : (max - min) / 1000.0),
          skewFactor(skew)
    {
        setTooltip(tooltipText);

        valueEditor.setTooltip(tooltipText);
        valueEditor.setMultiLine(false);
        valueEditor.setReturnKeyStartsNewLine(false);
        valueEditor.setSelectAllWhenFocused(true);
        valueEditor.setJustification(juce::Justification::centredRight);
        valueEditor.setScrollbarsShown(false);
        valueEditor.setFont(makeLabelFont(Typography::inspectorInput, false));
        valueEditor.setBorder(juce::BorderSize<int>(2, 4, 2, 2));
        styleHackerEditor(valueEditor);
        valueEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x00000000));
        valueEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x00000000));
        valueEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0x00000000));
        valueEditor.setColour(juce::TextEditor::textColourId, hackerText());
        valueEditor.setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.85f));
        valueEditor.onReturnKey = [this] { commitEditorText(); };
        valueEditor.onFocusLost = [this] { commitEditorText(); };
        valueEditor.onEscapeKey = [this]
        {
            refreshEditorText();
            valueEditor.giveAwayKeyboardFocus();
        };
        valueEditor.onMouseDownCallback = [this](const juce::MouseEvent& e) { return beginDrag(e); };
        valueEditor.onMouseDragCallback = [this](const juce::MouseEvent& e) { dragAdjust(e); };
        valueEditor.onMouseUpCallback = [this](const juce::MouseEvent& e) { endDrag(e); };
        valueEditor.onWheelCallback = [this](const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
        {
            wheelAdjust(event, wheel);
        };
        addAndMakeVisible(inputContainer);
        inputContainer.addAndMakeVisible(valueEditor);

        stepUpButton.setTooltip("Increase");
        stepUpButton.setTriggeredOnMouseDown(true);
        // Pro Tools-like autorepeat cadence: quick first nudge, then controlled hold-repeat.
        stepUpButton.setRepeatSpeed(420, 110, 45);
        stepUpButton.onClick = [this]
        {
            if (!locked)
                nudgeBySteps(1.0 * modifierStepMultiplier(juce::ModifierKeys::getCurrentModifiersRealtime()));
        };
        inputContainer.addAndMakeVisible(stepUpButton);

        stepDownButton.setTooltip("Decrease");
        stepDownButton.setTriggeredOnMouseDown(true);
        stepDownButton.setRepeatSpeed(420, 110, 45);
        stepDownButton.onClick = [this]
        {
            if (!locked)
                nudgeBySteps(-1.0 * modifierStepMultiplier(juce::ModifierKeys::getCurrentModifiersRealtime()));
        };
        inputContainer.addAndMakeVisible(stepDownButton);

        lockButton.setClickingTogglesState(true);
        lockButton.setToggleState(true, juce::dontSendNotification);
        lockButton.onClick = [this]
        {
            const bool shouldLock = lockButton.getToggleState();
            if (!shouldLock && locked && unlockAttemptHandler)
            {
                if (!unlockAttemptHandler(*this))
                {
                    lockButton.setToggleState(true, juce::dontSendNotification);
                    return;
                }
            }

            setLocked(shouldLock);
        };
        addAndMakeVisible(lockButton);

        refreshThemeColours();
        setLocked(true);
        refresh();
    }

    void refresh() override
    {
        currentValue = sanitizeValue(static_cast<double>(controlledValue.getValue()));
        if (!valueEditor.hasKeyboardFocus(true) && !isDragging)
            refreshEditorText();
        applyEnablement();
    }

    void lookAndFeelChanged() override
    {
        juce::PropertyComponent::lookAndFeelChanged();
        refreshThemeColours();
    }

    void resized() override
    {
        auto content = getLookAndFeel().getPropertyComponentContentPosition(*this);
        const int rowHeight = content.getHeight();
        const int legacyLockSide = juce::jlimit(15, 19, rowHeight - 4);
        const int legacyInputHeight = juce::jlimit(24, 32, rowHeight - 2);
        // Keep input and lock at a shared mid-size to avoid oversized fields + tiny lock icons.
        const int controlSide = juce::jlimit(21, 27, juce::roundToInt((legacyLockSide + legacyInputHeight) * 0.5f));
        const int lockSide = juce::jmin(controlSide, juce::jmax(0, content.getWidth()));
        auto lockBounds = content.removeFromRight(lockSide).withSizeKeepingCentre(lockSide, lockSide);
        lockButton.setBounds(lockBounds);
        content.removeFromRight(2);

        const int availableWidth = juce::jmax(0, content.getWidth());
        const int preferredInputWidth = juce::jlimit(52, 104, juce::roundToInt(static_cast<float>(availableWidth) * 1.00f));
        const int inputWidth = juce::jlimit(0, availableWidth, preferredInputWidth);
        auto inputBounds = content.removeFromLeft(inputWidth);
        inputBounds = inputBounds.withSizeKeepingCentre(inputBounds.getWidth(), controlSide);
        inputContainer.setBounds(inputBounds);

        auto inputArea = inputContainer.getLocalBounds().reduced(1, 1);
        const int kStepperWidth = juce::jlimit(8, 10, juce::jmax(8, inputArea.getWidth() / 3));
        inputContainer.setStepperColumnWidth(kStepperWidth);
        auto stepperArea = inputArea.removeFromRight(kStepperWidth);
        auto upBounds = stepperArea.removeFromTop(stepperArea.getHeight() / 2);
        stepUpButton.setBounds(upBounds);
        stepDownButton.setBounds(stepperArea);
        valueEditor.setBounds(inputArea);
        valueEditor.setFont(makeLabelFont(Typography::inspectorInput, false));
    }

    void setLocked(bool shouldLock)
    {
        locked = shouldLock;
        lockButton.setTooltip(locked ? "Locked: click to unlock this control"
                                     : "Unlocked: click to lock this control");
        lockButton.setToggleState(locked, juce::dontSendNotification);
        lockButton.repaint();
        applyEnablement();
    }

    bool isLocked() const { return locked; }
    double getCurrentValueForCommand() const { return currentValue; }
    double getMinimumForCommand() const { return minValue; }
    double getMaximumForCommand() const { return maxValue; }
    double getStepForCommand() const { return stepValue; }
    void setContextMenuHandler(ContextMenuHandler handler) { contextMenuHandler = std::move(handler); }
    void setUnlockAttemptHandler(UnlockAttemptHandler handler) { unlockAttemptHandler = std::move(handler); }

    void setValueFromCommand(double value)
    {
        applyValueFromUserInput(value);
    }

private:
    void refreshThemeColours()
    {
        styleHackerEditor(valueEditor);
        valueEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x00000000));
        valueEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x00000000));
        valueEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0x00000000));
        valueEditor.setColour(juce::TextEditor::textColourId, hackerText());
        valueEditor.setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.85f));
        // TextEditor keeps existing text runs; force current value text to adopt new theme colour immediately.
        valueEditor.applyColourToAllText(hackerText(), false);
        valueEditor.repaint();
        inputContainer.repaint();
        stepUpButton.repaint();
        stepDownButton.repaint();
        repaint();
    }

    class StepArrowButton : public juce::Button
    {
    public:
        explicit StepArrowButton(bool pointsUpIn)
            : juce::Button(pointsUpIn ? "up" : "down"), pointsUp(pointsUpIn)
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }

        void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            auto r = getLocalBounds().toFloat();
            const auto accent = hackerText();
            const auto baseBg = hackerBgField();
            juce::Colour arrowColour = isEnabled() ? accent : hackerTextMuted();
            if (isButtonDown)
            {
                g.setColour(accent.withAlpha(0.88f));
                g.fillRect(r);
                arrowColour = baseBg;
            }
            else if (isMouseOverButton)
            {
                g.setColour(accent.withAlpha(0.72f));
                g.fillRect(r);
                arrowColour = baseBg;
            }

            juce::Path arrow;
            const float cx = r.getCentreX();
            const float cy = r.getCentreY();
            const float w = juce::jmax(2.0f, r.getWidth() * 0.25f);
            const float h = juce::jmax(2.0f, r.getHeight() * 0.22f);
            if (pointsUp)
            {
                arrow.startNewSubPath(cx - w, cy + (h * 0.55f));
                arrow.lineTo(cx, cy - h);
                arrow.lineTo(cx + w, cy + (h * 0.55f));
            }
            else
            {
                arrow.startNewSubPath(cx - w, cy - (h * 0.55f));
                arrow.lineTo(cx, cy + h);
                arrow.lineTo(cx + w, cy - (h * 0.55f));
            }

            g.setColour(arrowColour);
            g.strokePath(arrow, juce::PathStrokeType(1.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

    private:
        bool pointsUp = false;
    };

    class LockIconButton : public juce::Button
    {
    public:
        LockIconButton() : juce::Button("lockIcon")
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }

        void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            auto r = getLocalBounds().toFloat().reduced(0.5f);
            const bool lockedState = getToggleState();
            const auto lockedOutlineBase = hackerBorderStrong();
            const auto unlockedOutlineBase = juce::Colour(0xffd24646);
            auto fill = lockedState ? hackerBgField() : juce::Colour(0xff1a0a0a);
            auto outline = lockedState ? lockedOutlineBase : unlockedOutlineBase;
            auto icon = lockedState ? juce::Colours::white.withAlpha(0.96f) : juce::Colour(0xffff5f5f);

            if (isMouseOverButton)
            {
                fill = lockedState ? hackerWash() : juce::Colour(0xff2a0f0f);
                outline = outline.brighter(0.18f);
                icon = icon.brighter(0.12f);
            }
            if (isButtonDown)
            {
                fill = lockedState ? hackerBgActive() : juce::Colour(0xff331111);
                outline = outline.brighter(0.28f);
            }

            if (!lockedState)
            {
                g.setColour(outline.withAlpha(isMouseOverButton ? 0.50f : 0.38f));
                g.drawRoundedRectangle(r.expanded(1.1f), 3.4f, isMouseOverButton ? 1.8f : 1.4f);
            }

            g.setColour(fill);
            g.fillRoundedRectangle(r, 3.0f);
            g.setColour(outline);
            g.drawRoundedRectangle(r, 3.0f, 1.0f);

            auto iconArea = r.reduced(1.8f, 1.8f);
            iconArea = iconArea.withSizeKeepingCentre(iconArea.getWidth() * 0.8f,
                                                      iconArea.getHeight() * 0.8f);
            if (const auto image = getLockImage(lockedState, icon); image.isValid())
            {
                g.setOpacity(isEnabled() ? 0.95f : 0.45f);
                g.drawImageWithin(image,
                                  juce::roundToInt(iconArea.getX()),
                                  juce::roundToInt(iconArea.getY()),
                                  juce::roundToInt(iconArea.getWidth()),
                                  juce::roundToInt(iconArea.getHeight()),
                                  juce::RectanglePlacement::centred,
                                  false);
                g.setOpacity(1.0f);
            }
            else
            {
                g.setColour(icon);
                g.setFont(makeRetroFont(Typography::vizBody, true));
                g.drawText(lockedState ? "L" : "U",
                           iconArea.toNearestInt(),
                           juce::Justification::centred,
                           false);
            }
        }

    private:
        static juce::Image decodeEmbeddedPngFromSvg(const void* svgData, int svgSize)
        {
            if (svgData == nullptr || svgSize <= 0)
                return {};

            const juce::String svgText = juce::String::fromUTF8(static_cast<const char*>(svgData), svgSize);
            static const juce::String marker { "data:image/png;base64," };
            // These exported SVGs contain two embedded PNGs (mask + painted layer).
            // We want the first one (mask/silhouette), which is high-contrast and
            // remains readable at small button sizes.
            const int markerPos = svgText.indexOf(marker);
            if (markerPos < 0)
                return {};

            const int payloadStart = markerPos + marker.length();
            const int payloadEnd = svgText.indexOfChar(payloadStart, '"');
            if (payloadEnd <= payloadStart)
                return {};

            const juce::String encoded = svgText.substring(payloadStart, payloadEnd);
            juce::MemoryOutputStream decoded;
            if (!juce::Base64::convertFromBase64(decoded, encoded))
                return {};

            return juce::ImageFileFormat::loadFrom(decoded.getData(), decoded.getDataSize());
        }

        static juce::Image getLockImage(bool lockedState, juce::Colour tint)
        {
            struct Cache
            {
                juce::Image lockedWhite;
                juce::Image unlockedRed;

                Cache()
                {
                    const auto lockRaw = decodeEmbeddedPngFromSvg(BinaryData::lock_svg, BinaryData::lock_svgSize);
                    const auto unlockRaw = decodeEmbeddedPngFromSvg(BinaryData::unlock_svg, BinaryData::unlock_svgSize);
                    lockedWhite = normalizeDarkIcon(lockRaw, juce::Colours::white);
                    unlockedRed = normalizeDarkIcon(unlockRaw, juce::Colour(0xffff4e4e));
                }

                static juce::Image normalizeDarkIcon(const juce::Image& source, juce::Colour tint)
                {
                    if (!source.isValid())
                        return {};

                    juce::Image out(juce::Image::ARGB, source.getWidth(), source.getHeight(), true);
                    const int w = source.getWidth();
                    const int h = source.getHeight();
                    for (int y = 0; y < h; ++y)
                    {
                        for (int x = 0; x < w; ++x)
                        {
                            const auto c = source.getPixelAt(x, y);
                            const auto r = static_cast<float>(c.getRed());
                            const auto g = static_cast<float>(c.getGreen());
                            const auto b = static_cast<float>(c.getBlue());
                            const auto maxChan = juce::jmax(r, juce::jmax(g, b));

                            // The supplied icons are very dark RGB-on-black (no alpha).
                            // Treat near-black as transparent and remap remaining signal to
                            // a visible tinted glyph.
                            float strength = (maxChan - 2.0f) / 14.0f;
                            strength = juce::jlimit(0.0f, 1.0f, strength);
                            if (strength <= 0.0f)
                            {
                                out.setPixelAt(x, y, juce::Colours::transparentBlack);
                                continue;
                            }

                            strength = 0.20f + 0.80f * std::sqrt(strength);
                            out.setPixelAt(x, y, tint.withAlpha(strength));
                        }
                    }
                    return out;
                }
            };

            static Cache cache;
            const auto& source = lockedState ? cache.lockedWhite : cache.unlockedRed;
            if (!source.isValid())
                return {};

            juce::Image out(juce::Image::ARGB, source.getWidth(), source.getHeight(), true);
            const int w = source.getWidth();
            const int h = source.getHeight();
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    const auto px = source.getPixelAt(x, y);
                    const float alpha = static_cast<float>(px.getAlpha()) / 255.0f;
                    if (alpha <= 0.0f)
                    {
                        out.setPixelAt(x, y, juce::Colours::transparentBlack);
                        continue;
                    }
                    out.setPixelAt(x, y, tint.withAlpha(alpha));
                }
            }
            return out;
        }
    };

    class InputContainer : public juce::Component
    {
    public:
        void setEnabledVisual(bool isEnabledIn)
        {
            if (enabledVisual != isEnabledIn)
            {
                enabledVisual = isEnabledIn;
                repaint();
            }
        }

        void setStepperColumnWidth(int width)
        {
            stepperColumnWidth = juce::jmax(8, width);
        }

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat().reduced(0.5f);
            const auto bg = enabledVisual ? hackerBgField() : hackerBgElevated();
            const auto outline = enabledVisual ? hackerBorder() : hackerTextMuted();
            g.setColour(bg);
            g.fillRoundedRectangle(area, 2.5f);
            g.setColour(outline);
            g.drawRoundedRectangle(area, 2.5f, 1.0f);

            const float dividerX = area.getRight() - static_cast<float>(stepperColumnWidth);
            g.setColour(outline.withAlpha(0.85f));
            g.drawLine(dividerX, area.getY() + 1.0f, dividerX, area.getBottom() - 1.0f, 1.0f);
            g.drawLine(dividerX, area.getCentreY(), area.getRight() - 1.0f, area.getCentreY(), 1.0f);
        }

    private:
        int stepperColumnWidth = 12;
        bool enabledVisual = true;
    };

    class DragTextEditor : public juce::TextEditor
    {
    public:
        std::function<bool(const juce::MouseEvent&)> onMouseDownCallback;
        std::function<void(const juce::MouseEvent&)> onMouseDragCallback;
        std::function<void(const juce::MouseEvent&)> onMouseUpCallback;
        std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onWheelCallback;

        void mouseDown(const juce::MouseEvent& e) override
        {
            bool consumeEvent = false;
            if (onMouseDownCallback)
                consumeEvent = onMouseDownCallback(e);
            if (consumeEvent)
                return;
            juce::TextEditor::mouseDown(e);
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (onMouseDragCallback)
                onMouseDragCallback(e);
            // Avoid text-selection drag; vertical drag is used for stepped value adjustment.
        }

        void mouseUp(const juce::MouseEvent& e) override
        {
            if (onMouseUpCallback)
                onMouseUpCallback(e);
            juce::TextEditor::mouseUp(e);
        }

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
        {
            if (onWheelCallback)
            {
                onWheelCallback(event, wheel);
                return;
            }
            juce::TextEditor::mouseWheelMove(event, wheel);
        }
    };

    bool beginDrag(const juce::MouseEvent& e)
    {
        if (e.mods.isPopupMenu())
        {
            if (contextMenuHandler)
            {
                contextMenuHandler(*this, e);
                return true;
            }
            return false;
        }

        if (locked || !e.mods.isLeftButtonDown())
            return false;
        dragStartScreenY = e.getScreenPosition().getY();
        dragStartValue = currentValue;
        isDragging = false;
        return false;
    }

    void dragAdjust(const juce::MouseEvent& e)
    {
        if (locked || !e.mods.isLeftButtonDown())
            return;

        const int currentScreenY = e.getScreenPosition().getY();
        const double deltaY = static_cast<double>(dragStartScreenY - currentScreenY);
        if (!isDragging && std::abs(deltaY) < 2.0)
            return;

        isDragging = true;

        double stepDelta = deltaY / 6.0;
        const bool fineModifier = e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isCtrlDown();
        if (!fineModifier)
        {
            const double accel = juce::jlimit(1.0, 6.0, 1.0 + (std::abs(deltaY) / 220.0));
            stepDelta *= accel;
        }
        stepDelta *= modifierStepMultiplier(e.mods);
        applyValueFromUserInput(dragStartValue + stepDelta * stepValue);
    }

    void endDrag(const juce::MouseEvent&)
    {
        isDragging = false;
    }

    void wheelAdjust(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
    {
        if (locked || std::abs(wheel.deltaY) <= 0.0f)
            return;

        // PT-like wheel feel:
        // - smooth devices (trackpad): granular motion
        // - stepped wheels: deliberate nudges
        double wheelSteps = wheel.isSmooth
            ? static_cast<double>(wheel.deltaY) * 16.0
            : static_cast<double>(wheel.deltaY) * 4.0;
        if (std::abs(wheelSteps) < 1.0)
            wheelSteps = wheel.deltaY > 0.0f ? 1.0 : -1.0;

        wheelSteps *= modifierStepMultiplier(event.mods);
        nudgeBySteps(wheelSteps);
    }

    static double modifierStepMultiplier(const juce::ModifierKeys& mods)
    {
        // Mirrors common DAW conventions:
        // Shift = fine, Cmd/Ctrl = ultra-fine, Alt = coarse.
        double multiplier = 1.0;
        if (mods.isShiftDown())
            multiplier *= 0.2;
        if (mods.isCommandDown() || mods.isCtrlDown())
            multiplier *= 0.25;
        if (mods.isAltDown())
            multiplier *= 5.0;
        return multiplier;
    }

    void nudgeBySteps(double stepCount)
    {
        applyValueFromUserInput(currentValue + (stepValue * stepCount));
    }

    void applyValueFromUserInput(double value)
    {
        const double sanitized = sanitizeValue(value);
        currentValue = sanitized;
        controlledValue = static_cast<float>(sanitized);
        refreshEditorText();
    }

    void commitEditorText()
    {
        if (locked)
        {
            refreshEditorText();
            return;
        }

        const auto text = valueEditor.getText().trim();
        if (!text.containsAnyOf("0123456789"))
        {
            refreshEditorText();
            return;
        }

        applyValueFromUserInput(text.getDoubleValue());
    }

    void applyEnablement()
    {
        valueEditor.setEnabled(!locked);
        inputContainer.setEnabledVisual(!locked);
        stepUpButton.setEnabled(!locked);
        stepDownButton.setEnabled(!locked);
    }

    void refreshEditorText()
    {
        constexpr int decimals = 3;
        valueEditor.setText(juce::String(currentValue, decimals), juce::dontSendNotification);
    }

    double sanitizeValue(double value) const
    {
        juce::NormalisableRange<double> range(minValue, maxValue, stepValue, skewFactor);
        const double normalized = juce::jlimit(0.0, 1.0, range.convertTo0to1(value));
        const double withSkew = range.convertFrom0to1(normalized);
        if (stepValue <= 0.0)
            return juce::jlimit(minValue, maxValue, withSkew);

        const double snapped = minValue + std::round((withSkew - minValue) / stepValue) * stepValue;
        return juce::jlimit(minValue, maxValue, snapped);
    }

    juce::Value controlledValue;
    InputContainer inputContainer;
    DragTextEditor valueEditor;
    StepArrowButton stepUpButton { true };
    StepArrowButton stepDownButton { false };
    LockIconButton lockButton;
    double minValue = 0.0;
    double maxValue = 1.0;
    double stepValue = 0.01;
    double skewFactor = 1.0;
    double currentValue = 0.0;
    int dragStartScreenY = 0;
    double dragStartValue = 0.0;
    bool isDragging = false;
    bool locked = true;
    ContextMenuHandler contextMenuHandler;
    UnlockAttemptHandler unlockAttemptHandler;
};

class ReadOnlyDiagnosticPropertyComponent : public juce::PropertyComponent
{
public:
    ReadOnlyDiagnosticPropertyComponent(const juce::String& name,
                                        std::function<juce::String()> valueProviderIn,
                                        const juce::String& tooltipText)
        : juce::PropertyComponent(name),
          valueProvider(std::move(valueProviderIn))
    {
        setTooltip(tooltipText);
        valueLabel.setTooltip(tooltipText);
        valueLabel.setJustificationType(juce::Justification::centredRight);
        valueLabel.setFont(makeRetroFont(Typography::inspectorReadout, false));
        addAndMakeVisible(valueLabel);
        refreshThemeColours();
        refresh();
    }

    void refresh() override
    {
        valueLabel.setText(valueProvider(), juce::dontSendNotification);
    }

    void resized() override
    {
        valueLabel.setFont(makeRetroFont(Typography::inspectorReadout, false));
        valueLabel.setBounds(getLookAndFeel().getPropertyComponentContentPosition(*this).reduced(2, 2));
    }

    void lookAndFeelChanged() override
    {
        juce::PropertyComponent::lookAndFeelChanged();
        refreshThemeColours();
    }

private:
    void refreshThemeColours()
    {
        valueLabel.setColour(juce::Label::textColourId, hackerText());
        repaint();
    }

    std::function<juce::String()> valueProvider;
    juce::Label valueLabel;
};

class MultiLineReadOnlyPropertyComponent : public juce::PropertyComponent
{
public:
    MultiLineReadOnlyPropertyComponent(const juce::String& name,
                                       std::function<juce::String()> valueProviderIn,
                                       const juce::String& tooltipText)
        : juce::PropertyComponent(""),
          valueProvider(std::move(valueProviderIn))
    {
        setTooltip(tooltipText);
        consoleEditor.setTooltip(tooltipText);
        consoleEditor.setMultiLine(true);
        consoleEditor.setReturnKeyStartsNewLine(false);
        consoleEditor.setReadOnly(true);
        consoleEditor.setCaretVisible(false);
        consoleEditor.setPopupMenuEnabled(false);
        consoleEditor.setScrollbarsShown(true);
        consoleEditor.setJustification(juce::Justification::topLeft);
        consoleEditor.setFont(makeLabelFont(Typography::consoleLog, false));
        consoleEditor.setBorder(juce::BorderSize<int>(6, 8, 6, 8));
        addAndMakeVisible(consoleEditor);
        refreshThemeColours();
        setPreferredHeight(220);
        refresh();
    }

    void refresh() override
    {
        consoleEditor.setText(valueProvider(), juce::dontSendNotification);
    }

    void resized() override
    {
        consoleEditor.setFont(makeLabelFont(Typography::consoleLog, false));
        consoleEditor.setBounds(getLocalBounds().reduced(4, 4));
    }

    void lookAndFeelChanged() override
    {
        juce::PropertyComponent::lookAndFeelChanged();
        refreshThemeColours();
    }

private:
    void refreshThemeColours()
    {
        consoleEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff010301));
        consoleEditor.setColour(juce::TextEditor::textColourId, hackerText());
        consoleEditor.setColour(juce::TextEditor::outlineColourId, hackerBorder().withAlpha(0.85f));
        consoleEditor.setColour(juce::TextEditor::focusedOutlineColourId, hackerBorderStrong());
        consoleEditor.setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.82f));
        repaint();
    }

    std::function<juce::String()> valueProvider;
    juce::TextEditor consoleEditor;
};

class FlatSectionHeaderPropertyComponent : public juce::PropertyComponent
{
public:
    explicit FlatSectionHeaderPropertyComponent(const juce::String& titleText)
        : juce::PropertyComponent("")
    {
        titleLabel.setText(titleText, juce::dontSendNotification);
        titleLabel.setInterceptsMouseClicks(false, false);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(titleLabel);
        setPreferredHeight(30);
        refreshThemeColours();
    }

    void refresh() override {}

    void resized() override
    {
        titleLabel.setFont(makeTitleFont(Typography::sectionHeader, true));
        titleLabel.setBounds(getLocalBounds().reduced(8, 4));
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(hackerBgElevated().withAlpha(0.95f));
        g.fillRoundedRectangle(area, 3.0f);
        g.setColour(hackerBorder().withAlpha(0.75f));
        g.drawRoundedRectangle(area, 3.0f, 1.0f);
    }

    void lookAndFeelChanged() override
    {
        juce::PropertyComponent::lookAndFeelChanged();
        refreshThemeColours();
    }

private:
    void refreshThemeColours()
    {
        titleLabel.setColour(juce::Label::textColourId, hackerText());
        repaint();
    }

    juce::Label titleLabel;
};

class ActionButtonPropertyComponent : public juce::PropertyComponent
{
public:
    ActionButtonPropertyComponent(const juce::String& name,
                                  const juce::String& buttonText,
                                  const juce::String& tooltipText,
                                  std::function<void()> onClickIn)
        : juce::PropertyComponent(name),
          onClick(std::move(onClickIn))
    {
        setTooltip(tooltipText);
        actionButton.setButtonText(buttonText);
        actionButton.setTooltip(tooltipText);
        actionButton.onClick = [this]
        {
            if (onClick)
                onClick();
        };
        addAndMakeVisible(actionButton);
        setPreferredHeight(40);
        refreshThemeColours();
    }

    void refresh() override {}

    void resized() override
    {
        auto content = getLookAndFeel().getPropertyComponentContentPosition(*this);
        actionButton.setBounds(content.reduced(0, 2));
    }

    void lookAndFeelChanged() override
    {
        juce::PropertyComponent::lookAndFeelChanged();
        refreshThemeColours();
    }

private:
    void refreshThemeColours()
    {
        styleHackerTextButton(actionButton, false);
    }

    juce::TextButton actionButton;
    std::function<void()> onClick;
};

class EmbeddedPropertyPanelCard : public juce::PropertyComponent
{
public:
    EmbeddedPropertyPanelCard(const juce::String& tooltipText)
        : juce::PropertyComponent("")
    {
        setTooltip(tooltipText);
        panel.setMessageWhenEmpty("No controls available.");
        addAndMakeVisible(panel);
        setPreferredHeight(180);
    }

    void setSection(const juce::String& sectionName, juce::Array<juce::PropertyComponent*>& props, bool initiallyOpen)
    {
        panel.clear();
        panel.addSection(sectionName, props);
        if (panel.getSectionNames().size() > 0)
            panel.setSectionOpen(0, initiallyOpen);
        setPreferredHeight(juce::jmax(148, panel.getTotalContentHeight() + 24));
        resized();
    }

    void refresh() override
    {
        panel.refreshAll();
    }

    void resized() override
    {
        panel.setBounds(getLocalBounds().reduced(4, 4));
    }

private:
    juce::PropertyPanel panel;
};

class HorizontalControlStripCard : public juce::PropertyComponent
{
public:
    HorizontalControlStripCard(const juce::String& titleTextIn, const juce::String& tooltipText)
        : juce::PropertyComponent(""),
          titleText(titleTextIn)
    {
        setTooltip(tooltipText);
        titleLabel.setTooltip(tooltipText);
        titleLabel.setInterceptsMouseClicks(false, false);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(titleLabel);
        setPreferredHeight(126);
        refreshThemeColours();
    }

    ~HorizontalControlStripCard() override
    {
        for (auto* control : controls)
        {
            if (control != nullptr)
                control->setLookAndFeel(nullptr);
        }
    }

    void addControl(const juce::String& labelText, juce::PropertyComponent* control)
    {
        if (control == nullptr)
            return;

        auto* label = labels.add(new juce::Label());
        label->setText(labelText, juce::dontSendNotification);
        label->setInterceptsMouseClicks(false, false);
        label->setJustificationType(juce::Justification::centred);
        label->setTooltip(control->getTooltip());
        addAndMakeVisible(label);

        control->setLookAndFeel(&inlineLookAndFeel);
        addAndMakeVisible(control);
        controls.add(control);

        setPreferredHeight(juce::jmax(118, controls.size() > 5 ? 130 : 122));
        refreshThemeColours();
        resized();
    }

    void refresh() override
    {
        for (auto* control : controls)
        {
            if (control != nullptr)
                control->refresh();
        }
    }

    void lookAndFeelChanged() override
    {
        juce::PropertyComponent::lookAndFeelChanged();
        refreshThemeColours();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(hackerSurfaceAlt().withAlpha(0.96f));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(hackerBorder().withAlpha(0.78f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        auto header = bounds.removeFromTop(24.0f);
        g.setColour(hackerBgElevated().withAlpha(0.92f));
        g.fillRoundedRectangle(header.reduced(1.0f, 1.0f), 3.0f);
        g.setColour(hackerBorderStrong().withAlpha(0.55f));
        g.drawLine(header.getX() + 1.0f, header.getBottom() - 0.5f,
                   header.getRight() - 1.0f, header.getBottom() - 0.5f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 6);
        titleLabel.setFont(makeTitleFont(Typography::sectionHeader, true));
        titleLabel.setBounds(area.removeFromTop(20));
        area.removeFromTop(6);

        const int columnCount = controls.size();
        if (columnCount <= 0)
            return;

        constexpr int nominalGap = 8;
        auto labelsRow = area.removeFromTop(14);
        area.removeFromTop(1);
        auto controlsRow = area.removeFromTop(juce::jmax(24, area.getHeight()));

        const int availableWidth = juce::jmax(1, controlsRow.getWidth());
        const int gap = (availableWidth >= columnCount * 64) ? nominalGap : 4;
        const int totalGap = gap * juce::jmax(0, columnCount - 1);
        const int baseColumnWidth = juce::jmax(1, (availableWidth - totalGap) / columnCount);
        int x = controlsRow.getX();

        for (int i = 0; i < columnCount; ++i)
        {
            const int isLast = (i == columnCount - 1) ? 1 : 0;
            const int remainingRight = controlsRow.getRight() - x;
            const int width = isLast ? remainingRight : juce::jmin(baseColumnWidth, remainingRight);

            auto labelBounds = juce::Rectangle<int>(x, labelsRow.getY(), width, labelsRow.getHeight());
            labels[i]->setFont(makeLabelFont(Typography::labelSmall, true));
            labels[i]->setBounds(labelBounds);

            auto controlBounds = juce::Rectangle<int>(x, controlsRow.getY(), width, controlsRow.getHeight());
            controls[i]->setBounds(controlBounds);
            x += width + gap;
        }
    }

private:
    class InlineControlLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        InlineControlLookAndFeel()
        {
            refreshThemeColours();
        }

        void refreshThemeColours()
        {
            setColour(juce::PropertyComponent::backgroundColourId, juce::Colours::transparentBlack);
            setColour(juce::PropertyComponent::labelTextColourId, hackerText());
            setColour(juce::Label::textColourId, hackerText());
            setColour(juce::TextButton::buttonColourId, hackerBgField());
            setColour(juce::TextButton::buttonOnColourId, hackerBgActive());
            setColour(juce::TextButton::textColourOffId, hackerText());
            setColour(juce::TextButton::textColourOnId, hackerText());
            setColour(juce::ToggleButton::textColourId, hackerText());
            setColour(juce::ToggleButton::tickColourId, hackerText());
            setColour(juce::ToggleButton::tickDisabledColourId, hackerTextMuted());
            setColour(juce::ComboBox::backgroundColourId, hackerBgField());
            setColour(juce::ComboBox::buttonColourId, hackerBgField());
            setColour(juce::ComboBox::textColourId, hackerText());
            setColour(juce::ComboBox::outlineColourId, hackerBorderStrong());
            setColour(juce::ComboBox::arrowColourId, hackerText());
            setColour(juce::PopupMenu::backgroundColourId, hackerBgElevated());
            setColour(juce::PopupMenu::textColourId, hackerText());
            setColour(juce::PopupMenu::highlightedBackgroundColourId, hackerBgActive());
            setColour(juce::PopupMenu::highlightedTextColourId, hackerText());
            setColour(juce::TextEditor::backgroundColourId, hackerBgField());
            setColour(juce::TextEditor::textColourId, hackerText());
            setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.85f));
            setColour(juce::TextEditor::outlineColourId, hackerBorder());
            setColour(juce::TextEditor::focusedOutlineColourId, hackerBorderStrong());
        }

        juce::Rectangle<int> getPropertyComponentContentPosition(juce::PropertyComponent& component) override
        {
            return component.getLocalBounds().reduced(1, 1);
        }

        void drawPropertyComponentBackground(juce::Graphics&, int, int, juce::PropertyComponent&) override {}
        void drawPropertyComponentLabel(juce::Graphics&, int, int, juce::PropertyComponent&) override {}

        juce::Font getLabelFont(juce::Label&) override
        {
            return makeLabelFont(Typography::label, false);
        }

        juce::Font getComboBoxFont(juce::ComboBox& box) override
        {
            juce::ignoreUnused(box);
            return makeLabelFont(Typography::combo, false);
        }

        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                  const juce::Colour& /*backgroundColour*/,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override
        {
            auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
            auto base = button.findColour(button.getToggleState()
                                              ? juce::TextButton::buttonOnColourId
                                              : juce::TextButton::buttonColourId);
            if (shouldDrawButtonAsDown)
                base = base.brighter(0.20f);
            else if (shouldDrawButtonAsHighlighted)
                base = base.brighter(0.08f);

            const float alpha = button.isEnabled() ? 1.0f : 0.55f;
            g.setColour(base.withMultipliedAlpha(alpha));
            g.fillRoundedRectangle(bounds, 4.0f);
            g.setColour(hackerBorderStrong().withMultipliedAlpha((button.hasKeyboardFocus(true) ? 1.0f : 0.78f) * alpha));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        }

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
        {
            const auto boxSize = juce::jmin(14.0f, button.getHeight() * 0.58f);
            const auto box = juce::Rectangle<float>(4.0f, (button.getHeight() - boxSize) * 0.5f, boxSize, boxSize);
            auto boxFill = hackerBgField();
            if (button.getToggleState())
                boxFill = hackerBgActive();
            if (shouldDrawButtonAsDown)
                boxFill = boxFill.brighter(0.20f);
            else if (shouldDrawButtonAsHighlighted)
                boxFill = boxFill.brighter(0.08f);

            const float alpha = button.isEnabled() ? 1.0f : 0.55f;
            g.setColour(boxFill.withMultipliedAlpha(alpha));
            g.fillRoundedRectangle(box, 3.0f);
            g.setColour((button.getToggleState() ? hackerBorderStrong() : hackerBorder()).withMultipliedAlpha(alpha));
            g.drawRoundedRectangle(box, 3.0f, 1.0f);

            if (button.getToggleState())
            {
                juce::Path check;
                check.startNewSubPath(box.getX() + box.getWidth() * 0.20f, box.getCentreY());
                check.lineTo(box.getX() + box.getWidth() * 0.43f, box.getBottom() - box.getHeight() * 0.20f);
                check.lineTo(box.getRight() - box.getWidth() * 0.18f, box.getY() + box.getHeight() * 0.22f);
                g.setColour(hackerText().withMultipliedAlpha(alpha));
                g.strokePath(check, juce::PathStrokeType(1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            g.setColour(button.findColour(juce::ToggleButton::textColourId).withMultipliedAlpha(alpha));
            g.setFont(makeLabelFont(Typography::labelSmall, true));
            g.drawFittedText(button.getButtonText(),
                             juce::Rectangle<int>(static_cast<int>(box.getRight() + 6.0f),
                                                  0,
                                                  juce::jmax(1, button.getWidth() - static_cast<int>(box.getRight() + 8.0f)),
                                                  button.getHeight()),
                             juce::Justification::centredLeft,
                             1);
        }

        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox& box) override
        {
            auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1));
            auto fill = box.findColour(juce::ComboBox::backgroundColourId);
            if (isButtonDown)
                fill = fill.brighter(0.20f);
            else if (box.isMouseOverOrDragging())
                fill = fill.brighter(0.08f);
            if (!box.isEnabled())
                fill = fill.withMultipliedAlpha(0.55f);

            g.setColour(fill);
            g.fillRoundedRectangle(bounds, 4.0f);
            g.setColour(box.findColour(juce::ComboBox::outlineColourId).withMultipliedAlpha(box.isEnabled() ? 0.92f : 0.55f));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

            auto arrowZone = juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                                    static_cast<float>(buttonW), static_cast<float>(buttonH)).reduced(4.0f, 4.0f);
            juce::Path arrow;
            const float cx = arrowZone.getCentreX();
            const float cy = arrowZone.getCentreY();
            const float aw = juce::jmin(7.0f, arrowZone.getWidth() * 0.33f);
            const float ah = juce::jmin(4.0f, arrowZone.getHeight() * 0.30f);
            arrow.startNewSubPath(cx - aw, cy - ah * 0.5f);
            arrow.lineTo(cx, cy + ah * 0.5f);
            arrow.lineTo(cx + aw, cy - ah * 0.5f);
            g.setColour(box.findColour(juce::ComboBox::arrowColourId).withMultipliedAlpha(box.isEnabled() ? 0.95f : 0.55f));
            g.strokePath(arrow, juce::PathStrokeType(1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
        {
            label.setBounds(3, 1, juce::jmax(1, box.getWidth() - box.getHeight() - 4), juce::jmax(1, box.getHeight() - 2));
            label.setFont(getComboBoxFont(box));
            label.setJustificationType(juce::Justification::centredLeft);
            label.setBorderSize(juce::BorderSize<int>(0, 6, 0, 4));
            label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            label.setColour(juce::Label::textColourId,
                            box.findColour(juce::ComboBox::textColourId).withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.55f));
        }
    };

    void refreshThemeColours()
    {
        inlineLookAndFeel.refreshThemeColours();
        titleLabel.setText(titleText, juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, hackerText());
        for (auto* label : labels)
        {
            if (label == nullptr)
                continue;
            label->setColour(juce::Label::textColourId, hackerTextDim());
        }
        repaint();
    }

    juce::String titleText;
    juce::Label titleLabel;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<juce::PropertyComponent> controls;
    InlineControlLookAndFeel inlineLookAndFeel;
};

struct ConsoleCommandResult
{
    juce::String output;
    bool clearOutput = false;
};

class CommandConsolePropertyComponent : public juce::PropertyComponent
{
public:
    using CommandHandler = std::function<ConsoleCommandResult(const juce::String&)>;
    using HudProvider = std::function<juce::String()>;

    CommandConsolePropertyComponent(const juce::String& name,
                                    CommandHandler commandHandlerIn,
                                    const juce::String& tooltipText,
                                    HudProvider hudProviderIn = {})
        : juce::PropertyComponent(""),
          commandHandler(std::move(commandHandlerIn)),
          hudProvider(std::move(hudProviderIn))
    {
        setTooltip(tooltipText);

        hudLabel.setTooltip("Pinned watch list");
        hudLabel.setJustificationType(juce::Justification::centredLeft);
        hudLabel.setFont(makeLabelFont(Typography::labelSmall, false));
        addAndMakeVisible(hudLabel);

        autocompleteLabel.setTooltip("Command suggestions (Tab/Shift+Tab to cycle)");
        autocompleteLabel.setJustificationType(juce::Justification::centredLeft);
        autocompleteLabel.setFont(makeLabelFont(Typography::labelSmall, false));
        autocompleteLabel.setInterceptsMouseClicks(false, false);
        autocompleteLabel.setVisible(false);
        addAndMakeVisible(autocompleteLabel);

        outputEditor.setTooltip(tooltipText);
        outputEditor.setMultiLine(true);
        outputEditor.setReturnKeyStartsNewLine(false);
        outputEditor.setReadOnly(true);
        outputEditor.setCaretVisible(false);
        outputEditor.setPopupMenuEnabled(true);
        outputEditor.setScrollbarsShown(true);
        outputEditor.setJustification(juce::Justification::topLeft);
        outputEditor.setFont(makeLabelFont(Typography::consoleLog, false));
        outputEditor.setBorder(juce::BorderSize<int>(6, 8, 6, 8));
        addAndMakeVisible(outputEditor);

        inputEditor.setTooltip("Type command and press Enter. Use Tab for autocomplete and Up/Down for history.");
        inputEditor.setMultiLine(false);
        inputEditor.setReturnKeyStartsNewLine(false);
        inputEditor.setScrollbarsShown(false);
        inputEditor.setJustification(juce::Justification::centredLeft);
        inputEditor.setFont(makeLabelFont(Typography::inspectorInput, false));
        inputEditor.setBorder(juce::BorderSize<int>(4, 8, 4, 8));
        inputEditor.setTextToShowWhenEmpty("Type command + Enter (help for command list)", hackerTextMuted());
        inputEditor.onTextChange = [this]
        {
            historyCursor = -1;
            updateAutocompleteSuggestions();
        };
        inputEditor.onReturnKey = [this] { submitCommand(); };
        inputEditor.keyInterceptor = [this](const juce::KeyPress& key) { return handleHistoryKey(key); };
        addAndMakeVisible(inputEditor);

        refreshThemeColours();
        updateHudLine();
        updateAutocompleteSuggestions();
        appendOutputLine(name + " ready. Type `help` and press Enter.");
        setPreferredHeight(256);
    }

    void setAutocompleteCommands(const juce::StringArray& commands)
    {
        autocompleteCommands.clear();
        std::unordered_set<std::string> seenLower;
        seenLower.reserve(static_cast<size_t>(juce::jmax(16, commands.size())));
        for (const auto& command : commands)
        {
            const juce::String trimmed = command.trim();
            if (trimmed.isEmpty())
                continue;

            const std::string key = trimmed.toLowerCase().toStdString();
            if (seenLower.insert(key).second)
                autocompleteCommands.add(trimmed);
        }
        updateAutocompleteSuggestions();
    }

    void setAutoScroll(bool enabled) { autoScrollEnabled = enabled; }
    void setShowTimestamps(bool enabled) { showTimestamps = enabled; }
    void setWrapLongLines(bool enabled)
    {
        wrapLongLines = enabled;
        outputEditor.setMultiLine(true, wrapLongLines);
    }
    void setMaxOutputLines(int maxLines)
    {
        maxOutputLines = juce::jlimit(120, 4000, maxLines);
        const int overflow = outputLines.size() - maxOutputLines;
        if (overflow > 0)
            outputLines.removeRange(0, overflow);
        outputEditor.setText(outputLines.joinIntoString("\n"), juce::dontSendNotification);
        outputEditorHasText = !outputLines.isEmpty();
        if (autoScrollEnabled)
            outputEditor.moveCaretToEnd();
    }

    void refresh() override
    {
        updateHudLine();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4, 4);
        constexpr int hudHeight = 22;
        auto hudArea = area.removeFromTop(hudHeight);
        hudLabel.setFont(makeLabelFont(Typography::labelSmall, false));
        hudLabel.setBounds(hudArea.reduced(2, 2));
        area.removeFromTop(2);
        constexpr int autocompleteHeight = 18;
        constexpr int inputHeight = 34;
        auto inputArea = area.removeFromBottom(inputHeight + autocompleteHeight + 4);
        autocompleteLabel.setFont(makeLabelFont(Typography::labelSmall, false));
        autocompleteLabel.setBounds(inputArea.removeFromTop(autocompleteHeight).reduced(2, 1));
        inputEditor.setFont(makeLabelFont(Typography::inspectorInput, false));
        outputEditor.setFont(makeLabelFont(Typography::consoleLog, false));
        outputEditor.setBounds(area);
        inputEditor.setBounds(inputArea.withTrimmedTop(4));
    }

    void lookAndFeelChanged() override
    {
        juce::PropertyComponent::lookAndFeelChanged();
        refreshThemeColours();
    }

    ConsoleCommandResult submitCommandForTesting(const juce::String& command,
                                                 bool recordInHistory = true,
                                                 bool mirrorToConsoleOutput = false)
    {
        const juce::String trimmed = command.trim();
        if (trimmed.isEmpty())
            return {};

        if (mirrorToConsoleOutput)
            appendOutputLine("> " + trimmed);

        if (recordInHistory && (commandHistory.isEmpty() || !commandHistory[commandHistory.size() - 1].equalsIgnoreCase(trimmed)))
            commandHistory.add(trimmed);
        while (commandHistory.size() > 200)
            commandHistory.remove(0);
        historyCursor = -1;

        ConsoleCommandResult result;
        if (commandHandler)
            result = commandHandler(trimmed);

        if (result.clearOutput)
        {
            if (mirrorToConsoleOutput)
                clearOutputBuffer();
            else
                outputLines.clear();
        }

        if (mirrorToConsoleOutput && result.output.isNotEmpty())
        {
            juce::StringArray lines;
            lines.addLines(result.output);
            if (lines.isEmpty())
                lines.add(result.output);
            appendOutputLines(lines);
        }

        if (mirrorToConsoleOutput)
            updateHudLine();
        return result;
    }

private:
    class HistoryTextEditor final : public juce::TextEditor
    {
    public:
        std::function<bool(const juce::KeyPress&)> keyInterceptor;

        bool keyPressed(const juce::KeyPress& key) override
        {
            if (keyInterceptor && keyInterceptor(key))
                return true;
            return juce::TextEditor::keyPressed(key);
        }
    };

    void refreshThemeColours()
    {
        hudLabel.setColour(juce::Label::textColourId, hackerTextDim());
        hudLabel.setColour(juce::Label::backgroundColourId, hackerBgField().withAlpha(0.4f));
        autocompleteLabel.setColour(juce::Label::textColourId, hackerTextMuted());
        autocompleteLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        outputEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff010301));
        outputEditor.setColour(juce::TextEditor::textColourId, hackerText());
        outputEditor.setColour(juce::TextEditor::outlineColourId, hackerBorder().withAlpha(0.85f));
        outputEditor.setColour(juce::TextEditor::focusedOutlineColourId, hackerBorderStrong());
        outputEditor.setColour(juce::TextEditor::highlightColourId, hackerBgActive().withAlpha(0.82f));
        styleHackerEditor(inputEditor);
        repaint();
    }

    bool handleHistoryKey(const juce::KeyPress& key)
    {
        if (key.getKeyCode() == juce::KeyPress::tabKey)
        {
            const int direction = key.getModifiers().isShiftDown() ? -1 : 1;
            applyAutocompleteMatch(direction);
            return true;
        }

        if (key.getKeyCode() == juce::KeyPress::upKey)
        {
            if (commandHistory.isEmpty())
                return true;
            if (historyCursor < 0)
                historyCursor = commandHistory.size() - 1;
            else
                historyCursor = juce::jmax(0, historyCursor - 1);
            inputEditor.setText(commandHistory[historyCursor], juce::dontSendNotification);
            inputEditor.moveCaretToEnd();
            updateAutocompleteSuggestions();
            return true;
        }

        if (key.getKeyCode() == juce::KeyPress::downKey)
        {
            if (commandHistory.isEmpty())
                return true;
            if (historyCursor < 0)
                return true;
            ++historyCursor;
            if (historyCursor >= commandHistory.size())
            {
                historyCursor = -1;
                inputEditor.clear();
            }
            else
            {
                inputEditor.setText(commandHistory[historyCursor], juce::dontSendNotification);
                inputEditor.moveCaretToEnd();
            }
            updateAutocompleteSuggestions();
            return true;
        }

        return false;
    }

    void submitCommand()
    {
        const juce::String command = inputEditor.getText().trim();
        if (command.isEmpty())
            return;

        appendOutputLine("> " + command);
        if (commandHistory.isEmpty() || !commandHistory[commandHistory.size() - 1].equalsIgnoreCase(command))
            commandHistory.add(command);
        while (commandHistory.size() > 200)
            commandHistory.remove(0);
        historyCursor = -1;

        ConsoleCommandResult result;
        maybeShowResetWaitIndicator(command);
        if (commandHandler)
            result = commandHandler(command);

        if (result.clearOutput)
            clearOutputBuffer();

        if (result.output.isNotEmpty())
        {
            juce::StringArray lines;
            lines.addLines(result.output);
            if (lines.isEmpty())
                lines.add(result.output);
            appendOutputLines(lines);
        }

        updateHudLine();
        inputEditor.clear();
        updateAutocompleteSuggestions();
    }

    void updateAutocompleteSuggestions()
    {
        const juce::String query = inputEditor.getText().trim().toLowerCase();
        if (query != autocompleteLastQuery)
            autocompleteMatchCursor = -1;
        autocompleteLastQuery = query;

        autocompleteMatches.clear();
        if (query.isEmpty() || autocompleteCommands.isEmpty())
        {
            autocompleteLabel.setText({}, juce::dontSendNotification);
            autocompleteLabel.setVisible(false);
            return;
        }

        struct Candidate
        {
            juce::String command;
            int score = 0;
        };

        std::vector<Candidate> candidates;
        candidates.reserve(static_cast<size_t>(autocompleteCommands.size()));
        for (const auto& command : autocompleteCommands)
        {
            const juce::String lower = command.toLowerCase();
            int score = -1;
            if (lower.startsWith(query))
                score = 0;
            else if (lower.contains(" " + query))
                score = 1;
            else if (lower.contains(query))
                score = 2;

            if (score >= 0)
                candidates.push_back({ command, score });
        }

        std::sort(candidates.begin(), candidates.end(),
                  [](const Candidate& a, const Candidate& b)
                  {
                      if (a.score != b.score)
                          return a.score < b.score;
                      if (a.command.length() != b.command.length())
                          return a.command.length() < b.command.length();
                      return a.command.compareIgnoreCase(b.command) < 0;
                  });

        constexpr int kMaxAutocompleteSuggestions = 8;
        const int visible = juce::jmin(kMaxAutocompleteSuggestions, static_cast<int>(candidates.size()));
        for (int i = 0; i < visible; ++i)
            autocompleteMatches.add(candidates[static_cast<size_t>(i)].command);

        if (autocompleteMatches.isEmpty())
        {
            autocompleteLabel.setText({}, juce::dontSendNotification);
            autocompleteLabel.setVisible(false);
            return;
        }

        autocompleteLabel.setText("Autocomplete: " + autocompleteMatches.joinIntoString("  | "),
                                  juce::dontSendNotification);
        autocompleteLabel.setVisible(true);
    }

    bool applyAutocompleteMatch(int direction)
    {
        if (autocompleteMatches.isEmpty())
            updateAutocompleteSuggestions();
        if (autocompleteMatches.isEmpty())
            return false;

        const int count = autocompleteMatches.size();
        if (autocompleteMatchCursor < 0 || autocompleteMatchCursor >= count)
            autocompleteMatchCursor = direction >= 0 ? 0 : (count - 1);
        else
            autocompleteMatchCursor = (autocompleteMatchCursor + direction + count) % count;

        inputEditor.setText(autocompleteMatches[autocompleteMatchCursor], juce::dontSendNotification);
        inputEditor.moveCaretToEnd();
        updateAutocompleteSuggestions();
        return true;
    }

    void maybeShowResetWaitIndicator(const juce::String& command)
    {
        juce::StringArray tokens;
        tokens.addTokens(command, " \t", "\"'");
        tokens.trim();
        tokens.removeEmptyStrings();
        if (tokens.size() < 2)
            return;
        if (!tokens[0].equalsIgnoreCase("reset"))
            return;

        const juce::String target = tokens[1].trim().unquoted();
        if (target.isEmpty())
            return;

        static constexpr const char* frames[] = { "::", ".:", ":." };
        const auto frame = juce::String(frames[spinnerFrameIndex % 3]);
        ++spinnerFrameIndex;

        appendOutputLine("resetting: '" + target + "' " + frame);
        if (auto* peer = outputEditor.getPeer())
            peer->performAnyPendingRepaintsNow();
    }

    void appendOutputLine(const juce::String& line)
    {
        juce::StringArray singleLine;
        if (showTimestamps)
            singleLine.add(juce::Time::getCurrentTime().formatted("[%H:%M:%S] ") + line);
        else
            singleLine.add(line);
        appendOutputLines(singleLine);
    }

    void appendOutputLines(const juce::StringArray& lines)
    {
        if (lines.isEmpty())
            return;

        outputLines.addArray(lines);
        const int overflow = outputLines.size() - maxOutputLines;
        if (overflow > 0)
        {
            outputLines.removeRange(0, overflow);
            outputEditor.setText(outputLines.joinIntoString("\n"), juce::dontSendNotification);
            outputEditorHasText = !outputLines.isEmpty();
        }
        else
        {
            juce::String appended = lines.joinIntoString("\n");
            if (outputEditorHasText)
                appended = "\n" + appended;

            const bool wasReadOnly = outputEditor.isReadOnly();
            outputEditor.setReadOnly(false);
            outputEditor.moveCaretToEnd();
            outputEditor.insertTextAtCaret(appended);
            outputEditor.setReadOnly(wasReadOnly);
            outputEditorHasText = true;
        }

        if (autoScrollEnabled)
            outputEditor.moveCaretToEnd();
    }

    void clearOutputBuffer()
    {
        outputLines.clear();
        outputEditor.setText({}, juce::dontSendNotification);
        outputEditorHasText = false;
    }

    void updateHudLine()
    {
        const juce::String hudText = hudProvider ? hudProvider() : juce::String();
        hudLabel.setText(hudText.isNotEmpty() ? hudText : "Watch: (none)", juce::dontSendNotification);
    }

    CommandHandler commandHandler;
    HudProvider hudProvider;
    juce::Label hudLabel;
    juce::Label autocompleteLabel;
    juce::TextEditor outputEditor;
    HistoryTextEditor inputEditor;
    juce::StringArray outputLines;
    juce::StringArray commandHistory;
    juce::StringArray autocompleteCommands;
    juce::StringArray autocompleteMatches;
    juce::String autocompleteLastQuery;
    int historyCursor = -1;
    int autocompleteMatchCursor = -1;
    int spinnerFrameIndex = 0;
    bool outputEditorHasText = false;
    bool autoScrollEnabled = true;
    bool showTimestamps = true;
    bool wrapLongLines = true;
    int maxOutputLines = 600;
};

class SparklinePropertyComponent : public juce::PropertyComponent
{
public:
    SparklinePropertyComponent(const juce::String& name,
                               std::function<std::vector<float>()> valueProviderIn,
                               const juce::String& tooltipText,
                               std::function<void(bool)> hoverCallbackIn = {})
        : juce::PropertyComponent(""),
          valueProvider(std::move(valueProviderIn)),
          hoverCallback(std::move(hoverCallbackIn))
    {
        setTooltip(tooltipText);
        plot.title = name;
        addAndMakeVisible(plot);
        setPreferredHeight(kSparklineLayout.visualMinPx);
        refresh();
    }

    void refresh() override
    {
        plot.values = valueProvider();
        plot.repaint();
    }

    void resized() override
    {
        plot.setBounds(getLocalBounds().reduced(4, 4));
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(true);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(false);
    }

private:
    struct Plot final : juce::Component
    {
        juce::String title;
        std::vector<float> values;

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.fillAll(juce::Colour(HackerTheme::bgElevated));
            g.setColour(juce::Colour(HackerTheme::border));
            g.drawRoundedRectangle(area.reduced(0.5f), 3.0f, 1.0f);

            auto content = area.reduced(6.0f, 4.0f);
            auto header = content.removeFromTop(18.0f);
            g.setColour(juce::Colour(HackerTheme::text));
            g.setFont(makeRetroFont(Typography::vizTitle, true));
            g.drawText(title, header.toNearestInt(), juce::Justification::centredLeft, false);

            const float midX = content.getCentreX();
            const float midY = content.getCentreY();
            g.setColour(juce::Colour(HackerTheme::grid));
            g.drawLine(content.getX() + 2.0f, midY, content.getRight() - 2.0f, midY, 1.0f);
            g.drawLine(midX, content.getY() + 2.0f, midX, content.getBottom() - 2.0f, 1.0f);

            if (values.size() < 2)
                return;

            float minV = values[0];
            float maxV = values[0];
            for (float v : values)
            {
                minV = std::min(minV, v);
                maxV = std::max(maxV, v);
            }
            const float span = juce::jmax(0.0001f, maxV - minV);

            juce::Path path;
            for (size_t i = 0; i < values.size(); ++i)
            {
                const float x = juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(values.size() - 1),
                                           content.getX() + 2.0f, content.getRight() - 2.0f);
                const float norm = (values[i] - minV) / span;
                const float y = juce::jmap(norm, 0.0f, 1.0f, content.getBottom() - 2.0f, content.getY() + 2.0f);
                if (i == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }

            g.setColour(juce::Colour(HackerTheme::text));
            g.strokePath(path, juce::PathStrokeType(1.75f));

            const float currentV = values.back();
            auto legendArea = content.reduced(4.0f);
            g.setFont(makeRetroFont(Typography::vizBody, false));
            g.setColour(juce::Colour(HackerTheme::textDim));
            g.drawText("min " + juce::String(minV, 3),
                       legendArea.removeFromLeft(legendArea.getWidth() * 0.33f).toNearestInt(),
                       juce::Justification::bottomLeft, false);
            g.drawText("max " + juce::String(maxV, 3),
                       legendArea.removeFromLeft(legendArea.getWidth() * 0.5f).toNearestInt(),
                       juce::Justification::centredBottom, false);
            g.drawText("now " + juce::String(currentV, 3),
                       legendArea.toNearestInt(),
                       juce::Justification::bottomRight, false);
        }
    };

    std::function<std::vector<float>()> valueProvider;
    std::function<void(bool)> hoverCallback;
    Plot plot;
};

class TransferCurvePropertyComponent : public juce::PropertyComponent
{
public:
    struct State
    {
        float drive = 1.0f;
        bool measured = false;
        bool frozen = false;
        bool clipped = false;
        float outputPeakDb = -100.0f;
        std::array<float, ChoroborosAudioProcessor::ANALYZER_TRANSFER_POINTS> input {};
        std::array<float, ChoroborosAudioProcessor::ANALYZER_TRANSFER_POINTS> output {};
    };

    TransferCurvePropertyComponent(const juce::String& name,
                                   std::function<State()> stateProviderIn,
                                   const juce::String& tooltipText,
                                   std::function<void(bool)> hoverCallbackIn = {})
        : juce::PropertyComponent(""),
          stateProvider(std::move(stateProviderIn)),
          hoverCallback(std::move(hoverCallbackIn))
    {
        setTooltip(tooltipText);
        plot.title = name;
        addAndMakeVisible(plot);
        setPreferredHeight(kTransferLayout.visualMinPx);
        refresh();
    }

    void refresh() override
    {
        plot.state = stateProvider();
        plot.state.drive = juce::jmax(1.0f, plot.state.drive);
        plot.repaint();
    }

    void resized() override
    {
        plot.setBounds(getLocalBounds().reduced(4, 4));
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(true);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(false);
    }

private:
    struct Plot final : public juce::Component
    {
        juce::String title;
        State state;
        std::array<float, ChoroborosAudioProcessor::ANALYZER_TRANSFER_POINTS> smoothedOutput {};
        bool smoothedOutputPrimed = false;

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.fillAll(juce::Colour(HackerTheme::bgElevated));
            g.setColour(juce::Colour(HackerTheme::border));
            g.drawRoundedRectangle(area.reduced(0.5f), 3.0f, 1.0f);

            auto content = area.reduced(6.0f);
            auto header = content.removeFromTop(18.0f);
            g.setColour(juce::Colour(HackerTheme::text));
            g.setFont(makeRetroFont(Typography::vizTitle, true));
            g.drawText(title, header.toNearestInt(), juce::Justification::centredLeft, false);

            if (!state.measured)
                drawStatusBadge(g, header, "NO DATA", juce::Colour(StagePalette::nodata));
            else if (state.clipped)
                drawStatusBadge(g, header, "CLIPPED", juce::Colour(StagePalette::danger));
            else if (state.frozen)
                drawStatusBadge(g, header, "FROZEN", juce::Colour(StagePalette::frozen));
            else
                drawStatusBadge(g, header, "LIVE", juce::Colour(StagePalette::live));

            // Linear reference
            g.setColour(juce::Colour(HackerTheme::textMuted));
            g.drawLine(content.getX() + 2.0f, content.getBottom() - 2.0f,
                       content.getRight() - 2.0f, content.getY() + 2.0f, 1.0f);

            const float clipXNeg = juce::jmap(-0.8f, -1.0f, 1.0f, content.getX() + 2.0f, content.getRight() - 2.0f);
            const float clipXPos = juce::jmap(0.8f, -1.0f, 1.0f, content.getX() + 2.0f, content.getRight() - 2.0f);
            const float clipYTop = juce::jmap(0.8f, -1.0f, 1.0f, content.getBottom() - 2.0f, content.getY() + 2.0f);
            const float clipYBottom = juce::jmap(-0.8f, -1.0f, 1.0f, content.getBottom() - 2.0f, content.getY() + 2.0f);
            g.setColour(juce::Colour(StagePalette::danger).withAlpha(0.07f));
            g.fillRect(juce::Rectangle<float>(content.getX() + 2.0f, content.getY() + 2.0f,
                                              juce::jmax(0.0f, clipXNeg - (content.getX() + 2.0f)),
                                              content.getHeight() - 4.0f));
            g.fillRect(juce::Rectangle<float>(clipXPos, content.getY() + 2.0f,
                                              juce::jmax(0.0f, content.getRight() - 2.0f - clipXPos),
                                              content.getHeight() - 4.0f));
            g.fillRect(juce::Rectangle<float>(content.getX() + 2.0f, content.getY() + 2.0f,
                                              content.getWidth() - 4.0f,
                                              juce::jmax(0.0f, clipYTop - (content.getY() + 2.0f))));
            g.fillRect(juce::Rectangle<float>(content.getX() + 2.0f, clipYBottom,
                                              content.getWidth() - 4.0f,
                                              juce::jmax(0.0f, content.getBottom() - 2.0f - clipYBottom)));

            g.setColour(juce::Colour(StagePalette::warning).withAlpha(0.45f));
            g.drawLine(clipXNeg, content.getY() + 2.0f, clipXNeg, content.getBottom() - 2.0f, 0.8f);
            g.drawLine(clipXPos, content.getY() + 2.0f, clipXPos, content.getBottom() - 2.0f, 0.8f);
            g.drawLine(content.getX() + 2.0f, clipYTop, content.getRight() - 2.0f, clipYTop, 0.8f);
            g.drawLine(content.getX() + 2.0f, clipYBottom, content.getRight() - 2.0f, clipYBottom, 0.8f);

            juce::Path measuredCurve;
            bool hasMeasuredCurve = false;
            if (state.measured)
            {
                if (!smoothedOutputPrimed)
                {
                    smoothedOutput = state.output;
                    smoothedOutputPrimed = true;
                }
                else
                {
                    for (size_t i = 0; i < smoothedOutput.size(); ++i)
                        smoothedOutput[i] = smoothedOutput[i] * 0.72f + state.output[i] * 0.28f;
                }

                std::vector<juce::Point<float>> points;
                points.reserve(state.input.size());
                for (size_t i = 0; i < state.input.size(); ++i)
                {
                    const float x = juce::jmap(state.input[i], -1.0f, 1.0f, content.getX() + 2.0f, content.getRight() - 2.0f);
                    const float y = juce::jmap(smoothedOutput[i], -1.0f, 1.0f, content.getBottom() - 2.0f, content.getY() + 2.0f);
                    points.emplace_back(x, y);
                }

                if (points.size() >= 2)
                {
                    measuredCurve.startNewSubPath(points.front());
                    for (size_t i = 1; i + 1 < points.size(); ++i)
                    {
                        const auto& current = points[i];
                        const auto& next = points[i + 1];
                        const juce::Point<float> midpoint((current.x + next.x) * 0.5f, (current.y + next.y) * 0.5f);
                        measuredCurve.quadraticTo(current, midpoint);
                    }
                    measuredCurve.lineTo(points.back());
                    hasMeasuredCurve = true;
                }
            }
            else
            {
                smoothedOutputPrimed = false;
            }

            if (hasMeasuredCurve)
            {
                g.setColour(juce::Colour(HackerTheme::text));
                g.strokePath(measuredCurve,
                             juce::PathStrokeType(2.0f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
            }
            else
            {
                g.setColour(juce::Colour(HackerTheme::textMuted));
                g.setFont(makeRetroFont(Typography::vizBody, false));
                g.drawText("Awaiting analyzer frame...", content.toNearestInt(), juce::Justification::centred, false);
            }

            const float clampedDrive = juce::jmax(1.0f, state.drive);
            const float halfOut = 0.5f * std::tanh(clampedDrive);
            const float halfIn = std::atanh(juce::jlimit(-0.999f, 0.999f, halfOut)) / clampedDrive;
            if (hasMeasuredCurve)
            {
                const float kneeX = juce::jmap(halfIn, -1.0f, 1.0f, content.getX() + 2.0f, content.getRight() - 2.0f);
                const float kneeY = juce::jmap(0.5f, -1.0f, 1.0f, content.getBottom() - 2.0f, content.getY() + 2.0f);
                g.setColour(juce::Colour(HackerTheme::textDim));
                g.fillEllipse(kneeX - 2.0f, kneeY - 2.0f, 4.0f, 4.0f);
            }

            g.setFont(makeRetroFont(Typography::vizBody, false));
            g.setColour(juce::Colour(HackerTheme::textDim));
            g.drawText("drive x" + juce::String(clampedDrive, 2),
                       juce::Rectangle<float>(content.getX() + 6.0f, content.getY() + 3.0f, 80.0f, 12.0f).toNearestInt(),
                       juce::Justification::centredLeft, false);
            g.drawText("knee ~" + juce::String(halfIn, 2),
                       juce::Rectangle<float>(content.getRight() - 88.0f, content.getY() + 3.0f, 82.0f, 12.0f).toNearestInt(),
                       juce::Justification::centredRight, false);
            g.drawText("peak " + juce::String(state.outputPeakDb, 1) + " dBFS",
                       juce::Rectangle<float>(content.getX() + 6.0f, content.getBottom() - 14.0f, 100.0f, 11.0f).toNearestInt(),
                       juce::Justification::centredLeft, false);
        }
    };

    std::function<State()> stateProvider;
    std::function<void(bool)> hoverCallback;
    Plot plot;
};

class SpectrumOverlayPropertyComponent : public juce::PropertyComponent
{
public:
    struct State
    {
        double sampleRate = 48000.0;
        float hpfHz = 30.0f;
        float lpfHz = 18000.0f;
        float preEmphasisGain = 1.0f;
        bool hasMeasuredSpectrum = false;
        bool frozen = false;
        bool clipped = false;
        float outputPeakDb = -100.0f;
        std::array<float, ChoroborosAudioProcessor::ANALYZER_FFT_SIZE / 2> spectrumNorm {};
    };

    SpectrumOverlayPropertyComponent(const juce::String& name,
                                     std::function<State()> stateProviderIn,
                                     const juce::String& tooltipText,
                                     std::function<void(bool)> hoverCallbackIn = {})
        : juce::PropertyComponent(""),
          stateProvider(std::move(stateProviderIn)),
          hoverCallback(std::move(hoverCallbackIn))
    {
        setTooltip(tooltipText);
        plot.title = name;
        addAndMakeVisible(plot);
        setPreferredHeight(kSpectrumLayout.visualMinPx);
        refresh();
    }

    void refresh() override
    {
        plot.state = stateProvider();
        plot.repaint();
    }

    void resized() override
    {
        plot.setBounds(getLocalBounds().reduced(4, 4));
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(true);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(false);
    }

private:
    struct Plot final : public juce::Component
    {
        juce::String title;
        State state;
        std::array<float, ChoroborosAudioProcessor::ANALYZER_FFT_SIZE / 2> smoothedSpectrumNorm {};
        bool smoothedSpectrumPrimed = false;
        std::array<float, ChoroborosAudioProcessor::ANALYZER_FFT_SIZE / 2> peakHoldNorm {};
        bool peakHoldPrimed = false;

        static float mapMagnitudeToNorm(float mag)
        {
            const float clamped = juce::jmax(1.0e-7f, mag);
            const float db = juce::Decibels::gainToDecibels(clamped, -96.0f);
            return juce::jlimit(0.0f, 1.0f, (db + 96.0f) / 96.0f);
        }

        static float xForFrequency(float hz, float sampleRate, float left, float right)
        {
            const float nyquist = juce::jmax(1000.0f, static_cast<float>(sampleRate * 0.5));
            const float minHz = 20.0f;
            const float maxHz = juce::jmax(minHz + 1.0f, nyquist);
            const float clamped = juce::jlimit(minHz, maxHz, hz);
            const float t = std::log(clamped / minHz) / std::log(maxHz / minHz);
            return juce::jmap(t, 0.0f, 1.0f, left, right);
        }

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.fillAll(juce::Colour(HackerTheme::bgElevated));
            g.setColour(juce::Colour(HackerTheme::border));
            g.drawRoundedRectangle(area.reduced(0.5f), 3.0f, 1.0f);

            auto content = area.reduced(6.0f, 4.0f);
            auto header = content.removeFromTop(18.0f);
            g.setColour(juce::Colour(HackerTheme::text));
            g.setFont(makeRetroFont(Typography::vizTitle, true));
            g.drawText(title, header.toNearestInt(), juce::Justification::centredLeft, false);

            if (!state.hasMeasuredSpectrum)
                drawStatusBadge(g, header, "NO DATA", juce::Colour(StagePalette::nodata));
            else if (state.clipped)
                drawStatusBadge(g, header, "CLIPPED", juce::Colour(StagePalette::danger));
            else if (state.frozen)
                drawStatusBadge(g, header, "FROZEN", juce::Colour(StagePalette::frozen));
            else
                drawStatusBadge(g, header, "LIVE", juce::Colour(StagePalette::live));

            if (state.hasMeasuredSpectrum)
            {
                if (!smoothedSpectrumPrimed)
                {
                    smoothedSpectrumNorm = state.spectrumNorm;
                    smoothedSpectrumPrimed = true;
                }
                else
                {
                    for (size_t i = 0; i < smoothedSpectrumNorm.size(); ++i)
                    {
                        const float target = juce::jlimit(0.0f, 1.0f, state.spectrumNorm[i]);
                        const float current = smoothedSpectrumNorm[i];
                        const float coeff = target > current ? 0.34f : 0.14f; // Fast attack, slower release.
                        smoothedSpectrumNorm[i] = current + (target - current) * coeff;
                    }
                }

                if (!peakHoldPrimed)
                {
                    peakHoldNorm = smoothedSpectrumNorm;
                    peakHoldPrimed = true;
                }
                else
                {
                    for (size_t i = 0; i < peakHoldNorm.size(); ++i)
                    {
                        const float live = smoothedSpectrumNorm[i];
                        float held = peakHoldNorm[i] * 0.985f;
                        if (live > held)
                            held = live;
                        peakHoldNorm[i] = juce::jlimit(0.0f, 1.0f, held);
                    }
                }
            }
            else
            {
                smoothedSpectrumPrimed = false;
                peakHoldPrimed = false;
            }

            const float left = content.getX() + 2.0f;
            const float right = content.getRight() - 2.0f;
            const float top = content.getY() + 2.0f;
            const float bottom = content.getBottom() - 2.0f;

            g.setColour(juce::Colour(HackerTheme::grid));
            static const float gridHz[] { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
            for (float hz : gridHz)
            {
                const float x = xForFrequency(hz, static_cast<float>(state.sampleRate), left, right);
                g.drawLine(x, top, x, bottom, 0.5f);
            }

            for (int i = 1; i <= 5; ++i)
            {
                const float y = juce::jmap(static_cast<float>(i) / 5.0f, 0.0f, 1.0f, bottom, top);
                g.drawLine(left, y, right, y, 0.5f);
            }

            juce::Path spectrumPath;
            juce::Path holdPath;
            std::vector<juce::Point<float>> spectrumPoints;
            std::vector<juce::Point<float>> holdPoints;
            const size_t count = smoothedSpectrumNorm.size();
            spectrumPoints.reserve(count);
            holdPoints.reserve(count);
            const float nyquist = juce::jmax(1000.0f, static_cast<float>(state.sampleRate * 0.5));

            std::array<float, ChoroborosAudioProcessor::ANALYZER_FFT_SIZE / 2> displaySpectrum {};
            displaySpectrum = smoothedSpectrumNorm;
            if (count >= 3)
            {
                displaySpectrum[0] = displaySpectrum[1];
                displaySpectrum[count - 1] = displaySpectrum[count - 2];
                for (size_t i = 1; i + 1 < count; ++i)
                {
                    displaySpectrum[i] = juce::jlimit(0.0f, 1.0f,
                                                      smoothedSpectrumNorm[i - 1] * 0.18f
                                                      + smoothedSpectrumNorm[i] * 0.64f
                                                      + smoothedSpectrumNorm[i + 1] * 0.18f);
                }
            }

            for (size_t i = 1; i < count; ++i)
            {
                const float hz = (static_cast<float>(i) / static_cast<float>(count - 1)) * nyquist;
                if (hz < 20.0f)
                    continue;

                const float x = xForFrequency(hz, static_cast<float>(state.sampleRate), left, right);
                const float yLive = juce::jmap(displaySpectrum[i], 0.0f, 1.0f, bottom, top);
                const float yHold = juce::jmap(peakHoldNorm[i], 0.0f, 1.0f, bottom, top);
                spectrumPoints.emplace_back(x, yLive);
                holdPoints.emplace_back(x, yHold);
            }

            auto makeSmoothPath = [](const std::vector<juce::Point<float>>& points, juce::Path& path) -> bool
            {
                if (points.size() < 2)
                    return false;

                path.startNewSubPath(points.front());
                for (size_t i = 1; i + 1 < points.size(); ++i)
                {
                    const auto& current = points[i];
                    const auto& next = points[i + 1];
                    const juce::Point<float> midpoint((current.x + next.x) * 0.5f,
                                                      (current.y + next.y) * 0.5f);
                    path.quadraticTo(current, midpoint);
                }
                path.lineTo(points.back());
                return true;
            };

            const bool spectrumStarted = makeSmoothPath(spectrumPoints, spectrumPath);
            const bool holdStarted = makeSmoothPath(holdPoints, holdPath);

            if (spectrumStarted)
            {
                g.setColour(juce::Colour(HackerTheme::textDim).withAlpha(0.95f));
                g.strokePath(spectrumPath, juce::PathStrokeType(1.8f));
            }

            if (holdStarted)
            {
                g.setColour(juce::Colour(StagePalette::warning).withAlpha(0.42f));
                g.strokePath(holdPath, juce::PathStrokeType(1.1f));
            }

            const float hpfX = xForFrequency(state.hpfHz, static_cast<float>(state.sampleRate), left, right);
            const float lpfX = xForFrequency(state.lpfHz, static_cast<float>(state.sampleRate), left, right);
            g.setColour(juce::Colour(HackerTheme::text).withAlpha(0.75f));
            g.drawLine(hpfX, top, hpfX, bottom, 1.2f);
            g.drawLine(lpfX, top, lpfX, bottom, 1.2f);

            g.setFont(makeRetroFont(Typography::vizSmall, false));
            g.setColour(juce::Colour(HackerTheme::textDim));
            g.drawText("HP " + juce::String(state.hpfHz, 0) + " Hz",
                       juce::Rectangle<float>(left + 3.0f, top + 3.0f, 96.0f, 11.0f).toNearestInt(),
                       juce::Justification::centredLeft, false);
            g.drawText("LP " + juce::String(state.lpfHz, 0) + " Hz",
                       juce::Rectangle<float>(right - 108.0f, top + 3.0f, 104.0f, 11.0f).toNearestInt(),
                       juce::Justification::centredRight, false);
            g.drawText("pre x" + juce::String(state.preEmphasisGain, 2),
                       juce::Rectangle<float>(left + 3.0f, bottom - 14.0f, 90.0f, 11.0f).toNearestInt(),
                       juce::Justification::centredLeft, false);
            g.drawText("peak " + juce::String(state.outputPeakDb, 1) + " dBFS",
                       juce::Rectangle<float>(right - 118.0f, bottom - 14.0f, 114.0f, 11.0f).toNearestInt(),
                       juce::Justification::centredRight, false);
        }
    };

    std::function<State()> stateProvider;
    std::function<void(bool)> hoverCallback;
    Plot plot;
};

class SignalFlowPropertyComponent : public juce::PropertyComponent
{
public:
    struct Stage
    {
        juce::String name;
        juce::String value;
        bool active = false;
    };

    struct State
    {
        juce::String modeLabel;
        std::array<Stage, 7> stages {};
        float preLevel = 0.0f;
        float wetLevel = 0.0f;
        float postLevel = 0.0f;
        bool live = false;
        bool clipped = false;
    };

    SignalFlowPropertyComponent(const juce::String& name,
                                std::function<State()> stateProviderIn,
                                const juce::String& tooltipText,
                                std::function<void(bool)> hoverCallbackIn = {})
        : juce::PropertyComponent(""),
          stateProvider(std::move(stateProviderIn)),
          hoverCallback(std::move(hoverCallbackIn))
    {
        setTooltip(tooltipText);
        plot.title = name;
        addAndMakeVisible(plot);
        setPreferredHeight(kSignalFlowLayout.visualMinPx);
        refresh();
    }

    void refresh() override
    {
        plot.state = stateProvider();
        plot.repaint();
    }

    void resized() override
    {
        plot.setBounds(getLocalBounds().reduced(4, 4));
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(true);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoverCallback)
            hoverCallback(false);
    }

private:
    struct Plot final : public juce::Component
    {
        juce::String title;
        State state;

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.fillAll(juce::Colour(HackerTheme::bgElevated));
            g.setColour(juce::Colour(HackerTheme::border));
            g.drawRoundedRectangle(area.reduced(0.5f), 3.0f, 1.0f);

            auto content = area.reduced(6.0f, 4.0f);
            auto header = content.removeFromTop(20.0f);
            g.setColour(juce::Colour(HackerTheme::text));
            g.setFont(makeRetroFont(Typography::signalFlowHeader, true));
            g.drawText(title + "  [" + state.modeLabel + "]", header.toNearestInt(), juce::Justification::centredLeft, false);
            if (state.clipped)
                drawStatusBadge(g, header, "CLIP", juce::Colour(StagePalette::danger));
            else if (state.live)
                drawStatusBadge(g, header, "LIVE", juce::Colour(StagePalette::live));
            else
                drawStatusBadge(g, header, "EST", juce::Colour(StagePalette::warning));

            auto meterArea = content.removeFromTop(18.0f);
            auto drawMeter = [&](float x, float w, float level, juce::Colour colour)
            {
                auto meter = juce::Rectangle<float>(x, meterArea.getY(), w, meterArea.getHeight());
                g.setColour(juce::Colour(HackerTheme::bgField));
                g.fillRoundedRectangle(meter, 2.0f);
                const float clamped = juce::jlimit(0.0f, 1.0f, level);
                auto fill = meter.reduced(1.0f);
                fill.setWidth(fill.getWidth() * clamped);
                g.setColour(colour.withAlpha(0.85f));
                g.fillRoundedRectangle(fill, 1.5f);
                g.setColour(colour.withAlpha(0.55f));
                g.drawRoundedRectangle(meter, 2.0f, 1.0f);
            };

            const float meterGap = 8.0f;
            const float meterW = (meterArea.getWidth() - meterGap * 2.0f) / 3.0f;
            drawMeter(meterArea.getX(), meterW, state.preLevel, visualOverview());
            drawMeter(meterArea.getX() + meterW + meterGap, meterW, state.wetLevel, visualModulation());
            drawMeter(meterArea.getX() + (meterW + meterGap) * 2.0f, meterW, state.postLevel,
                      state.clipped ? juce::Colour(StagePalette::danger) : visualValidation());

            const float rowHeight = juce::jmax(18.0f, (content.getHeight() - 4.0f) / static_cast<float>(state.stages.size()));
            float y = content.getY() + 2.0f;
            g.setFont(makeRetroFont(Typography::signalFlowRow, false));
            const std::array<juce::Colour, 7> stageColours
            {{
                visualOverview(),   // input
                visualTone(),       // tone
                visualLayout(),     // filters
                visualEngine(),     // core
                visualModulation(), // color
                juce::Colour(0xffd8b66e), // saturate
                visualValidation()  // output
            }};
            int stageIndex = 0;
            for (const auto& stage : state.stages)
            {
                const juce::Colour stageColour = stageColours[static_cast<size_t>(juce::jlimit(0, 6, stageIndex))];
                auto row = juce::Rectangle<float>(content.getX(), y, content.getWidth(), rowHeight - 1.0f);
                g.setColour(stage.active ? stageColour.withAlpha(0.22f) : juce::Colour(HackerTheme::bg));
                g.fillRect(row);
                g.setColour(stage.active ? stageColour.brighter(0.30f) : juce::Colour(HackerTheme::textMuted));
                g.drawText(stage.name, row.withWidth(content.getWidth() * 0.34f).toNearestInt(), juce::Justification::centredLeft, false);
                auto valueRect = row.withX(content.getX() + content.getWidth() * 0.35f);
                g.drawText(stage.value, valueRect.toNearestInt(), juce::Justification::centredLeft, false);
                y += rowHeight;
                ++stageIndex;
            }
        }
    };

    std::function<State()> stateProvider;
    std::function<void(bool)> hoverCallback;
    Plot plot;
};

class LayoutPreviewPropertyComponent : public juce::PropertyComponent
{
public:
    struct Element
    {
        juce::String label;
        juce::Rectangle<float> bounds;
        juce::Colour colour { juce::Colour(HackerTheme::textDim) };
        bool highlighted = true;
        bool showCoordinates = true;
    };

    struct State
    {
        juce::String profileLabel;
        juce::String subtitle;
        juce::String footer;
        int canvasWidth = 700;
        int canvasHeight = 363;
        int gridMinorStep = 25;
        int gridMajorStep = 100;
        std::vector<Element> elements;
    };

    LayoutPreviewPropertyComponent(const juce::String& name,
                                   std::function<State()> stateProviderIn,
                                   const juce::String& tooltipText)
        : juce::PropertyComponent(""),
          stateProvider(std::move(stateProviderIn))
    {
        setTooltip(tooltipText);
        plot.title = name;
        addAndMakeVisible(plot);
        setPreferredHeight(472);
        refresh();
    }

    void refresh() override
    {
        plot.state = stateProvider();
        plot.repaint();
    }

    void resized() override
    {
        plot.setBounds(getLocalBounds().reduced(4, 4));
    }

private:
    struct Plot final : public juce::Component
    {
        juce::String title;
        State state;

        static juce::Rectangle<float> mapRectToCanvas(const juce::Rectangle<float>& source,
                                                      const juce::Rectangle<float>& canvasRect,
                                                      float scale)
        {
            return juce::Rectangle<float>(canvasRect.getX() + source.getX() * scale,
                                          canvasRect.getY() + source.getY() * scale,
                                          source.getWidth() * scale,
                                          source.getHeight() * scale);
        }

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.fillAll(juce::Colour(HackerTheme::bgElevated));
            g.setColour(juce::Colour(HackerTheme::border));
            g.drawRoundedRectangle(area.reduced(0.5f), 3.0f, 1.0f);

            auto content = area.reduced(6.0f);
            auto header = content.removeFromTop(20.0f);
            auto footer = content.removeFromBottom(18.0f);
            g.setColour(juce::Colour(HackerTheme::text));
            g.setFont(makeRetroFont(Typography::vizTitle, true));
            g.drawText(title, header.toNearestInt(), juce::Justification::centredLeft, false);
            if (state.profileLabel.isNotEmpty())
                drawStatusBadge(g, header, state.profileLabel, hackerText());

            if (state.subtitle.isNotEmpty())
            {
                auto subtitle = content.removeFromTop(14.0f);
                g.setColour(juce::Colour(HackerTheme::textDim));
                g.setFont(makeRetroFont(Typography::vizSmall, false));
                g.drawText(state.subtitle, subtitle.toNearestInt(), juce::Justification::centredLeft, false);
            }

            if (state.footer.isNotEmpty())
            {
                g.setColour(juce::Colour(HackerTheme::textMuted));
                g.setFont(makeRetroFont(Typography::vizSmall, false));
                g.drawText(state.footer, footer.toNearestInt(), juce::Justification::centredLeft, false);
            }

            auto plotArea = content.reduced(2.0f);
            auto axisArea = plotArea.reduced(34.0f, 8.0f);
            axisArea.removeFromBottom(16.0f);
            axisArea.removeFromLeft(18.0f);
            if (axisArea.getWidth() <= 8.0f || axisArea.getHeight() <= 8.0f)
                return;

            const float canvasW = static_cast<float>(juce::jmax(1, state.canvasWidth));
            const float canvasH = static_cast<float>(juce::jmax(1, state.canvasHeight));
            const float scale = juce::jmin(axisArea.getWidth() / canvasW, axisArea.getHeight() / canvasH);
            const float drawW = canvasW * scale;
            const float drawH = canvasH * scale;
            const float canvasX = axisArea.getX() + (axisArea.getWidth() - drawW) * 0.5f;
            const float canvasY = axisArea.getY() + (axisArea.getHeight() - drawH) * 0.5f;
            const auto canvasRect = juce::Rectangle<float>(canvasX, canvasY, drawW, drawH);

            juce::ColourGradient canvasBg(juce::Colour(HackerTheme::bgField),
                                          canvasRect.getX(), canvasRect.getY(),
                                          juce::Colour(HackerTheme::bgElevated),
                                          canvasRect.getRight(), canvasRect.getBottom(),
                                          false);
            g.setGradientFill(canvasBg);
            g.fillRect(canvasRect);

            const int minorStep = juce::jmax(5, state.gridMinorStep);
            const int majorStep = juce::jmax(minorStep, state.gridMajorStep);

            auto drawVertical = [&](int x, bool major)
            {
                const float px = canvasRect.getX() + static_cast<float>(x) * scale;
                g.setColour((major ? juce::Colour(HackerTheme::border) : juce::Colour(HackerTheme::grid)).withAlpha(major ? 0.55f : 0.40f));
                g.drawLine(px, canvasRect.getY(), px, canvasRect.getBottom(), major ? 0.9f : 0.5f);
                if (major)
                {
                    g.setFont(makeRetroFont(Typography::vizTiny, false));
                    g.setColour(juce::Colour(HackerTheme::textDim));
                    g.drawText(juce::String(x),
                               juce::Rectangle<float>(px - 18.0f, canvasRect.getBottom() + 2.0f, 36.0f, 10.0f).toNearestInt(),
                               juce::Justification::centred, false);
                }
            };

            auto drawHorizontal = [&](int y, bool major)
            {
                const float py = canvasRect.getY() + static_cast<float>(y) * scale;
                g.setColour((major ? juce::Colour(HackerTheme::border) : juce::Colour(HackerTheme::grid)).withAlpha(major ? 0.55f : 0.40f));
                g.drawLine(canvasRect.getX(), py, canvasRect.getRight(), py, major ? 0.9f : 0.5f);
                if (major)
                {
                    g.setFont(makeRetroFont(Typography::vizTiny, false));
                    g.setColour(juce::Colour(HackerTheme::textDim));
                    g.drawText(juce::String(y),
                               juce::Rectangle<float>(canvasRect.getX() - 30.0f, py - 5.0f, 28.0f, 10.0f).toNearestInt(),
                               juce::Justification::centredRight, false);
                }
            };

            for (int x = 0; x <= state.canvasWidth; x += minorStep)
                drawVertical(x, (x % majorStep) == 0);
            for (int y = 0; y <= state.canvasHeight; y += minorStep)
                drawHorizontal(y, (y % majorStep) == 0);

            g.setColour(juce::Colour(HackerTheme::borderStrong).withAlpha(0.85f));
            g.drawRect(canvasRect, 1.2f);
            g.setFont(makeRetroFont(Typography::vizTiny, false));
            g.drawText("X", juce::Rectangle<float>(canvasRect.getRight() + 4.0f, canvasRect.getBottom() + 1.0f, 10.0f, 10.0f).toNearestInt(),
                       juce::Justification::centred, false);
            g.drawText("Y", juce::Rectangle<float>(canvasRect.getX() - 14.0f, canvasRect.getY() - 11.0f, 10.0f, 10.0f).toNearestInt(),
                       juce::Justification::centred, false);

            auto drawElement = [&](const Element& element, bool drawHighlighted)
            {
                if (element.highlighted != drawHighlighted)
                    return;
                auto rect = mapRectToCanvas(element.bounds, canvasRect, scale).getIntersection(canvasRect);
                if (rect.getWidth() < 2.0f || rect.getHeight() < 2.0f)
                    return;

                const juce::Colour base = element.highlighted ? hackerText() : hackerTextDim();
                const float fillAlpha = element.highlighted ? 0.24f : 0.10f;
                const float strokeAlpha = element.highlighted ? 0.90f : 0.45f;
                g.setColour(base.withAlpha(fillAlpha));
                g.fillRoundedRectangle(rect, 2.0f);
                g.setColour(base.withAlpha(strokeAlpha));
                g.drawRoundedRectangle(rect, 2.0f, element.highlighted ? 1.35f : 0.85f);

                const auto center = rect.getCentre();
                g.setColour(base.withAlpha(element.highlighted ? 0.95f : 0.55f));
                g.drawLine(center.x - 3.0f, center.y, center.x + 3.0f, center.y, 0.9f);
                g.drawLine(center.x, center.y - 3.0f, center.x, center.y + 3.0f, 0.9f);

                if (element.label.isNotEmpty())
                {
                    const auto labelHeight = element.showCoordinates ? 19.0f : 11.0f;
                    auto labelRect = juce::Rectangle<float>(rect.getX() + 2.0f, rect.getY() + 2.0f,
                                                            juce::jmax(72.0f, juce::jmin(168.0f, rect.getWidth() - 4.0f)),
                                                            labelHeight);
                    g.setColour(juce::Colour(HackerTheme::bgField).withAlpha(0.82f));
                    g.fillRoundedRectangle(labelRect, 2.0f);
                    g.setColour(base.withAlpha(0.62f));
                    g.drawRoundedRectangle(labelRect, 2.0f, 0.9f);
                    g.setFont(makeRetroFont(Typography::vizTiny, true));
                    g.setColour(base.brighter(0.55f));
                    g.drawText(element.label, labelRect.removeFromTop(10.0f).toNearestInt(),
                               juce::Justification::centredLeft, false);

                    if (element.showCoordinates)
                    {
                        const auto x = juce::roundToInt(element.bounds.getX());
                        const auto y = juce::roundToInt(element.bounds.getY());
                        const auto w = juce::roundToInt(element.bounds.getWidth());
                        const auto h = juce::roundToInt(element.bounds.getHeight());
                        g.setFont(makeRetroFont(Typography::vizTiny, false));
                        g.setColour(juce::Colour(HackerTheme::textDim).withAlpha(0.88f));
                        g.drawText("x" + juce::String(x) + " y" + juce::String(y) + "  " + juce::String(w) + "x" + juce::String(h),
                                   labelRect.toNearestInt(),
                                   juce::Justification::centredLeft,
                                   false);
                    }
                }
            };

            for (const auto& element : state.elements)
                drawElement(element, false);
            for (const auto& element : state.elements)
                drawElement(element, true);
        }
    };

    std::function<State()> stateProvider;
    Plot plot;
};

class ValidationTraceMatrixPropertyComponent : public juce::PropertyComponent
{
public:
    struct Row
    {
        juce::String control;
        float uiRaw = 0.0f;
        float mapped = 0.0f;
        float snapshotRaw = 0.0f;
        float snapshotMapped = 0.0f;
        juce::String effective;
        bool inSync = false;
        bool profileValid = false;
    };

    struct State
    {
        juce::String profile;
        juce::String mode;
        juce::String snapshotSource;
        std::vector<Row> rows;
    };

    ValidationTraceMatrixPropertyComponent(const juce::String& name,
                                           std::function<State()> stateProviderIn,
                                           const juce::String& tooltipText)
        : juce::PropertyComponent(""),
          stateProvider(std::move(stateProviderIn))
    {
        setTooltip(tooltipText);
        addAndMakeVisible(plot);
        setPreferredHeight(kTraceMatrixHeight);
        refresh();
    }

    void refresh() override
    {
        plot.state = stateProvider();
        const int rowCount = juce::jmax(1, static_cast<int>(plot.state.rows.size()));
        const int sourceRow = plot.state.snapshotSource.isNotEmpty() ? 1 : 0;
        const int desiredHeight = 24 + ((rowCount + sourceRow + 1) * 22) + 18;
        setPreferredHeight(juce::jlimit(220, 420, desiredHeight));
        plot.repaint();
    }

    void resized() override
    {
        plot.setBounds(getLocalBounds().reduced(4, 4));
    }

private:
    struct Plot final : public juce::Component
    {
        State state;

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.fillAll(juce::Colour(HackerTheme::bgElevated));
            g.setColour(juce::Colour(HackerTheme::border));
            g.drawRoundedRectangle(area.reduced(0.5f), 3.0f, 1.0f);

            auto content = area.reduced(6.0f, 4.0f);
            const auto header = content.removeFromTop(18.0f);
            g.setColour(juce::Colour(HackerTheme::text));
            g.setFont(makeRetroFont(Typography::vizTitle, true));
            g.drawText("Trace Matrix  [" + state.profile + " / " + state.mode + "]",
                       header.toNearestInt(), juce::Justification::centredLeft, false);

            if (state.snapshotSource.isNotEmpty())
            {
                auto sourceLine = content.removeFromTop(12.0f);
                g.setColour(juce::Colour(HackerTheme::textMuted));
                g.setFont(makeRetroFont(Typography::vizSmall, false));
                g.drawText("snapshot source: " + state.snapshotSource,
                           sourceLine.toNearestInt(),
                           juce::Justification::centredLeft,
                           false);
            }

            const float rowCount = juce::jmax(1.0f, static_cast<float>(state.rows.size()));
            constexpr float traceMatrixCellFontSize = 14.0f;
            constexpr float traceMatrixCellGap = 1.5f;
            constexpr float syncWidth = 34.0f;
            const float xControl = content.getX() + 8.0f;
            const float xUi = content.getX() + content.getWidth() * 0.2225f;
            const float xMapped = content.getX() + content.getWidth() * 0.3525f;
            const float xSnapshot = content.getX() + content.getWidth() * 0.4975f;
            const float xEffective = content.getX() + content.getWidth() * 0.68f;
            const float xStatus = content.getRight() - syncWidth - 8.0f;
            const float columnHeaderY = content.getY() + 0.5f;
            const float availableRowsHeight = juce::jmax(24.0f, content.getBottom() - columnHeaderY - 1.0f);
            const float idealRowHeight = availableRowsHeight / (rowCount + 1.0f);
            const float rowHeight = juce::jlimit(traceMatrixCellFontSize + 2.0f,
                                                 28.0f,
                                                 idealRowHeight);

            auto columnWidth = [traceMatrixCellGap](float xStart, float xEnd)
            {
                return juce::jmax(20.0f, xEnd - xStart - traceMatrixCellGap);
            };

            auto drawHeaderCell = [&](const juce::String& text, float x, float width)
            {
                g.setColour(juce::Colour(HackerTheme::textMuted));
                g.setFont(makeRetroFont(traceMatrixCellFontSize, true));
                g.drawText(text, juce::Rectangle<float>(x, columnHeaderY, width, rowHeight).toNearestInt(),
                           juce::Justification::centredLeft, false);
            };

            drawHeaderCell("Control", xControl, columnWidth(xControl, xUi));
            drawHeaderCell("UI Raw", xUi, columnWidth(xUi, xMapped));
            drawHeaderCell("Mapped", xMapped, columnWidth(xMapped, xSnapshot));
            drawHeaderCell("Snapshot", xSnapshot, columnWidth(xSnapshot, xEffective));
            drawHeaderCell("Effective", xEffective, columnWidth(xEffective, xStatus));
            drawHeaderCell("Sync", xStatus, syncWidth);

            float y = columnHeaderY + rowHeight;
            g.setFont(makeRetroFont(traceMatrixCellFontSize, false));
            for (size_t i = 0; i < state.rows.size(); ++i)
            {
                const auto& row = state.rows[i];
                const auto rowArea = juce::Rectangle<float>(content.getX(), y, content.getWidth(), rowHeight);
                if ((i % 2) != 0)
                {
                    g.setColour(juce::Colour(HackerTheme::bg));
                    g.fillRect(rowArea);
                }

                const juce::Colour rowColour = !row.profileValid ? juce::Colour(HackerTheme::text)
                                           : (row.inSync ? juce::Colour(HackerTheme::text) : juce::Colour(HackerTheme::textDim));
                g.setColour(rowColour);

                g.drawText(row.control, juce::Rectangle<float>(xControl, y, columnWidth(xControl, xUi), rowHeight), juce::Justification::centredLeft, false);
                g.drawText(juce::String(row.uiRaw, 3), juce::Rectangle<float>(xUi, y, columnWidth(xUi, xMapped), rowHeight), juce::Justification::centredLeft, false);
                g.drawText(juce::String(row.mapped, 3), juce::Rectangle<float>(xMapped, y, columnWidth(xMapped, xSnapshot), rowHeight), juce::Justification::centredLeft, false);
                const juce::String snapshotText = row.profileValid
                    ? (juce::String(row.snapshotRaw, 3) + " -> " + juce::String(row.snapshotMapped, 3))
                    : "n/a";
                g.drawText(snapshotText, juce::Rectangle<float>(xSnapshot, y, columnWidth(xSnapshot, xEffective), rowHeight), juce::Justification::centredLeft, false);
                g.drawText(row.effective, juce::Rectangle<float>(xEffective, y, columnWidth(xEffective, xStatus), rowHeight), juce::Justification::centredLeft, false);
                g.drawText(row.profileValid && row.inSync ? "Y" : "N",
                           juce::Rectangle<float>(xStatus, y, syncWidth, rowHeight),
                           juce::Justification::centredRight, false);
                y += rowHeight;
            }
        }
    };

    std::function<State()> stateProvider;
    Plot plot;
};

inline void styleTitle(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(makeTitleFont(Typography::title, true));
    label.setJustificationType(juce::Justification::left);
    label.setColour(juce::Label::textColourId, hackerText());
}

inline void styleDescription(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(makeLabelFont(Typography::description, false));
    label.setJustificationType(juce::Justification::left);
    label.setColour(juce::Label::textColourId, hackerTextDim());
}

inline void stylePanel(juce::PropertyPanel& panel)
{
    panel.setOpaque(false);
    panel.setColour(juce::PropertyComponent::backgroundColourId, hackerSurfaceAlt());
    panel.setColour(juce::PropertyComponent::labelTextColourId, hackerText());
    panel.setColour(juce::Label::textColourId, hackerText());
    if (auto* vp = dynamic_cast<juce::Viewport*>(panel.getChildComponent(0)))
    {
        vp->setOpaque(false);
        if (auto* viewed = vp->getViewedComponent())
            viewed->setOpaque(false);
    }
    panel.setLookAndFeel(&getDevPanelSectionLookAndFeel());
}

inline void addPanelSection(juce::PropertyPanel& panel, const juce::String& name,
                     juce::Array<juce::PropertyComponent*>& props, bool shouldBeOpen)
{
    const int minRowHeight = isDevPanelLargeHitTargetsEnabled() ? 46 : 38;
    for (auto* prop : props)
        prop->setPreferredHeight(juce::jmax(prop->getPreferredHeight(), minRowHeight));
    panel.addSection(name, props, shouldBeOpen);
    stylePanel(panel);
}

inline void addFlatPanelGroup(juce::PropertyPanel& panel, const juce::String& title,
                              juce::Array<juce::PropertyComponent*>& props)
{
    const int minRowHeight = isDevPanelLargeHitTargetsEnabled() ? 46 : 38;
    juce::Array<juce::PropertyComponent*> rows;
    rows.add(new FlatSectionHeaderPropertyComponent(title));
    for (auto* prop : props)
    {
        if (prop == nullptr)
            continue;
        prop->setPreferredHeight(juce::jmax(prop->getPreferredHeight(), minRowHeight));
        rows.add(prop);
    }
    panel.addProperties(rows);
    stylePanel(panel);
}

inline void setSectionRowHeight(juce::Array<juce::PropertyComponent*>& props, int minHeight)
{
    for (auto* prop : props)
    {
        if (prop != nullptr)
            prop->setPreferredHeight(juce::jmax(prop->getPreferredHeight(), minHeight));
    }
}
} // namespace devpanel
