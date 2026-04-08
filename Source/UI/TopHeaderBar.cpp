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

#include "TopHeaderBar.h"
#include "../Plugin/PluginProcessor.h"
#include "DevPanelSupport.h"
#include "BinaryData.h"

namespace
{
juce::Image makeSoftwareImage(juce::Image::PixelFormat format, int width, int height, bool clearImage)
{
    return juce::Image(format, width, height, clearImage, juce::SoftwareImageType());
}


//==============================================================================
// Colour helpers — extremely dark, barely tinted by the engine accent.
//==============================================================================

juce::Colour barBgTop (juce::Colour accent)
{
    // Nearly black with a whisper of engine colour.
    return juce::Colour (0xff0a0a0a).interpolatedWith (accent, 0.025f);
}

juce::Colour barBgBottom (juce::Colour accent)
{
    return juce::Colour (0xff111111).interpolatedWith (accent, 0.035f);
}

juce::Colour barSeparator (juce::Colour accent)
{
    return accent.withAlpha (0.35f);
}

juce::Colour btnHoverFill (juce::Colour accent)
{
    return accent.withAlpha (0.10f);
}

juce::Colour btnDownFill (juce::Colour accent)
{
    return accent.withAlpha (0.18f);
}

juce::Colour comboHoverFill (juce::Colour accent)
{
    return accent.withAlpha (0.06f);
}

//==============================================================================
// Custom LookAndFeel — invisible-background buttons, minimal combo.
//==============================================================================

class HeaderLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    HeaderLookAndFeel()
    {
        setAccentColour (devpanel::engineSkinColourForIndex (0));
    }

    void setAccentColour (juce::Colour newAccent)
    {
        accent_ = newAccent;

        // ComboBox: transparent bg, accent text, no visible outline by default.
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::buttonColourId,     juce::Colours::transparentBlack);
        setColour (juce::ComboBox::textColourId,        juce::Colours::white.withAlpha (0.92f));
        setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId,       accent_.withAlpha (0.70f));

        // PopupMenu
        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff111111));
        setColour (juce::PopupMenu::textColourId,                  juce::Colours::white.withAlpha (0.88f));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent_.withAlpha (0.18f));
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

        // TextButton — transparent default, accent text.
        setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        setColour (juce::TextButton::buttonOnColourId,  btnDownFill (accent_));
        setColour (juce::TextButton::textColourOffId,   accent_.withAlpha (0.70f));
        setColour (juce::TextButton::textColourOnId,    juce::Colours::white);

        // AlertWindow (for save dialog)
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff111111));
        setColour (juce::AlertWindow::textColourId,       juce::Colours::white.withAlpha (0.90f));
        setColour (juce::AlertWindow::outlineColourId,    accent_.withAlpha (0.50f));

        setColour (juce::TextEditor::backgroundColourId,       juce::Colour (0xff1a1a1a));
        setColour (juce::TextEditor::textColourId,             juce::Colours::white.withAlpha (0.90f));
        setColour (juce::TextEditor::outlineColourId,          accent_.withAlpha (0.30f));
        setColour (juce::TextEditor::focusedOutlineColourId,   accent_.withAlpha (0.60f));
        setColour (juce::TextEditor::highlightColourId,        accent_.withAlpha (0.25f));
    }

    //------------------------------------------------------------------
    // Fonts
    //------------------------------------------------------------------

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return devpanel::makeLabelFont (juce::jmax (11.0f, buttonHeight * 0.52f), true);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return devpanel::makeLabelFont (11.5f, false);
    }

    juce::Font getPopupMenuFont() override
    {
        return devpanel::makeLabelFont (11.5f, false);
    }

    //------------------------------------------------------------------
    // Button — invisible bg, shows fill only on hover/down.
    //------------------------------------------------------------------

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&,
                               bool isHighlighted, bool isDown) override
    {
        if (! button.isEnabled())
            return;

        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);

        if (isDown)
        {
            g.setColour (btnDownFill (accent_));
            g.fillRoundedRectangle (bounds, 4.0f);
        }
        else if (isHighlighted)
        {
            g.setColour (btnHoverFill (accent_));
            g.fillRoundedRectangle (bounds, 4.0f);
        }
        // Otherwise: nothing drawn — invisible background.
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool, bool) override
    {
        const float alpha = button.isEnabled() ? 1.0f : 0.30f;
        const auto col = button.findColour (button.getToggleState()
                                                ? juce::TextButton::textColourOnId
                                                : juce::TextButton::textColourOffId);
        g.setColour (col.withMultipliedAlpha (alpha));
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds(),
                          juce::Justification::centred, 1);
    }

    //------------------------------------------------------------------
    // ComboBox — borderless, shows hover fill.
    //------------------------------------------------------------------

    void drawComboBox (juce::Graphics& g, int width, int height,
                       bool isButtonDown,
                       int /*buttonX*/, int /*buttonY*/,
                       int /*buttonW*/, int /*buttonH*/,
                       juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                              static_cast<float> (width),
                                              static_cast<float> (height));

        // Subtle fill on hover/active only.
        if (isButtonDown)
        {
            g.setColour (btnDownFill (accent_));
            g.fillRoundedRectangle (bounds.reduced (0.5f), 4.0f);
        }
        else if (box.isMouseOverOrDragging())
        {
            g.setColour (comboHoverFill (accent_));
            g.fillRoundedRectangle (bounds.reduced (0.5f), 4.0f);
        }

        // Dropdown chevron — small, right-aligned.
        const float arrowSize = juce::jmin (8.0f, static_cast<float> (height) * 0.28f);
        const float arrowX = static_cast<float> (width) - arrowSize - 8.0f;
        const float arrowY = (static_cast<float> (height) - arrowSize * 0.5f) * 0.5f;

        juce::Path arrow;
        arrow.startNewSubPath (arrowX, arrowY);
        arrow.lineTo (arrowX + arrowSize * 0.5f, arrowY + arrowSize * 0.5f);
        arrow.lineTo (arrowX + arrowSize, arrowY);

        const float arrowAlpha = box.isEnabled() ? 0.65f : 0.25f;
        g.setColour (accent_.withAlpha (arrowAlpha));
        g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        const int arrowZone = juce::jmin (box.getHeight(), 24);
        label.setBounds (6, 0,
                         juce::jmax (1, box.getWidth() - arrowZone - 6),
                         box.getHeight());
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
        label.setBorderSize (juce::BorderSize<int> (0, 4, 0, 2));
        label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label.setColour (juce::Label::textColourId,
                         box.findColour (juce::ComboBox::textColourId)
                             .withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.35f));
    }

    //------------------------------------------------------------------
    // PopupMenu — dark, clean, consistent with the plugin.
    //------------------------------------------------------------------

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (juce::Colour (0xff111111));

        // Thin accent border.
        g.setColour (accent_.withAlpha (0.30f));
        g.drawRect (0, 0, width, height, 1);
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool /*hasSubMenu*/,
                            const juce::String& text, const juce::String& /*shortcutKeyText*/,
                            const juce::Drawable* /*icon*/, const juce::Colour* textColourToUse) override
    {
        if (isSeparator)
        {
            g.setColour (accent_.withAlpha (0.20f));
            g.fillRect (area.reduced (10, area.getHeight() / 2).withHeight (1));
            return;
        }

        auto row = area.reduced (4, 1).toFloat();
        if (isHighlighted && isActive)
        {
            g.setColour (accent_.withAlpha (0.15f));
            g.fillRoundedRectangle (row, 4.0f);
        }

        auto colour = textColourToUse != nullptr ? *textColourToUse
                                                 : juce::Colours::white.withAlpha (0.88f);
        if (! isActive)
            colour = colour.withAlpha (0.35f);

        auto textArea = area.reduced (14, 0);
        if (isTicked)
        {
            // Small check mark.
            g.setColour (accent_.withAlpha (isActive ? 0.80f : 0.35f));
            const float cx = static_cast<float> (textArea.getX()) + 2.0f;
            const float cy = static_cast<float> (textArea.getCentreY());
            juce::Path tick;
            tick.startNewSubPath (cx, cy);
            tick.lineTo (cx + 3.5f, cy + 3.5f);
            tick.lineTo (cx + 9.0f, cy - 4.0f);
            g.strokePath (tick, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
            textArea.removeFromLeft (16);
        }

        g.setColour (colour);
        g.setFont (getPopupMenuFont());
        g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
    }

private:
    juce::Colour accent_;
};

HeaderLookAndFeel& getHeaderLookAndFeel()
{
    static HeaderLookAndFeel lf;
    return lf;
}

} // namespace

//==============================================================================
TopHeaderBar::TopHeaderBar (PresetManager& presetManager, float uiScale)
    : presetManager_ (presetManager),
      uiScale_ (uiScale),
      barHeight_ (juce::roundToInt (static_cast<float> (kDesignHeight) * uiScale)),
      accentColour_ (devpanel::engineSkinColourForIndex (0))
{
    // Render the SVG logo to a raster image, then convert to white-on-transparent.
    // The source SVG (375x225 viewBox) has embedded raster data with a baked-in
    // black background — rendering to image and extracting brightness as alpha
    // gives us a clean white logo we can composite over the dark header.
    {
        auto drawable = juce::Drawable::createFromImageData (
            BinaryData::Kaizen_logo_svg,
            static_cast<size_t> (BinaryData::Kaizen_logo_svgSize));

        if (drawable != nullptr)
        {
            // Render at 3x the bar height for crisp scaling (logo is drawn at
            // 1.5× the bar-minus-padding height, so 3x gives good retina density).
            const int renderH = barHeight_ * 3;
            const int renderW = juce::roundToInt (static_cast<float> (renderH) * (375.0f / 225.0f));

            juce::Image rendered = makeSoftwareImage (juce::Image::ARGB, renderW, renderH, true);
            {
                juce::Graphics ig (rendered);
                drawable->drawWithin (ig,
                    rendered.getBounds().toFloat(),
                    juce::RectanglePlacement::centred, 1.0f);
            }

            // Convert: use pixel brightness as alpha, set colour to white.
            // This strips the black background (black → transparent) and turns
            // all visible content white.
            logoImage_ = makeSoftwareImage (juce::Image::ARGB, renderW, renderH, true);
            juce::Image::BitmapData src (rendered, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData dst (logoImage_, juce::Image::BitmapData::writeOnly);

            for (int y = 0; y < renderH; ++y)
            {
                for (int x = 0; x < renderW; ++x)
                {
                    auto px = src.getPixelColour (x, y);
                    // Brightness of the pixel becomes its alpha.
                    const float brightness = px.getBrightness();
                    const juce::uint8 alpha = static_cast<juce::uint8> (
                        juce::jlimit (0.0f, 255.0f, brightness * 255.0f * px.getFloatAlpha()));
                    dst.setPixelColour (x, y,
                        juce::Colour::fromRGBA (255, 255, 255, alpha));
                }
            }
        }
    }

    auto* lf = &getHeaderLookAndFeel();
    lf->setAccentColour (accentColour_);

    // Preset combo — shows "Load a preset" when nothing is selected.
    presetMenu_.setLookAndFeel (lf);
    presetMenu_.setScrollWheelEnabled (true);
    presetMenu_.setTextWhenNothingSelected ("Load a preset");
    presetMenu_.onChange = [this]
    {
        if (updatingPresetMenu_)
            return;
        const int selected = presetMenu_.getSelectedId();
        if (selected > 0)
            presetManager_.loadPreset (selected - 1);
    };
    addAndMakeVisible (presetMenu_);

    // Style all buttons with the header LF.
    for (auto* btn : { &prevButton_, &nextButton_, &saveButton_, &deleteButton_ })
    {
        btn->setLookAndFeel (lf);
        addAndMakeVisible (*btn);
    }

    prevButton_.onClick  = [this] { presetManager_.previousPreset(); };
    nextButton_.onClick  = [this] { presetManager_.nextPreset(); };
    saveButton_.onClick  = [this] { showSaveDialog(); };
    deleteButton_.onClick = [this] { showDeleteDialog(); };

    prevButton_.setTooltip  ("Previous preset");
    nextButton_.setTooltip  ("Next preset");
    saveButton_.setTooltip  ("Save as user preset");
    deleteButton_.setTooltip ("Delete user preset");

    presetManager_.addListener (this);
    setAccentColour (accentColour_);
    refreshPresetMenu();
}

TopHeaderBar::~TopHeaderBar()
{
    // Clear LookAndFeel on all children FIRST — if removeListener triggers a
    // callback that touches child components, they won't use a stale L&F.
    presetMenu_.setLookAndFeel (nullptr);
    if (engineSelector_ != nullptr)
        engineSelector_->setLookAndFeel (nullptr);
    for (auto* btn : { &prevButton_, &nextButton_, &saveButton_, &deleteButton_ })
        btn->setLookAndFeel (nullptr);

    presetManager_.removeListener (this);
}

//==============================================================================
void TopHeaderBar::setEngineSelector (juce::ComboBox* selector)
{
    engineSelector_ = selector;

    if (engineSelector_ != nullptr)
    {
        addAndMakeVisible (*engineSelector_);

        // Style it to match the header bar look.
        auto* lf = &getHeaderLookAndFeel();
        engineSelector_->setLookAndFeel (lf);
        engineSelector_->setColour (juce::ComboBox::backgroundColourId,  juce::Colours::transparentBlack);
        engineSelector_->setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
        engineSelector_->setColour (juce::ComboBox::textColourId,        juce::Colours::white.withAlpha (0.92f));
        engineSelector_->setColour (juce::ComboBox::arrowColourId,       accentColour_.withAlpha (0.70f));
    }

    resized();
}

//==============================================================================
void TopHeaderBar::setAccentColour (juce::Colour newAccent)
{
    accentColour_ = newAccent;

    auto& lf = getHeaderLookAndFeel();
    lf.setAccentColour (accentColour_);

    // Refresh combo colours.
    presetMenu_.setColour (juce::ComboBox::arrowColourId, accentColour_.withAlpha (0.70f));

    if (engineSelector_ != nullptr)
        engineSelector_->setColour (juce::ComboBox::arrowColourId, accentColour_.withAlpha (0.70f));

    // Button text colours.
    for (auto* btn : { &prevButton_, &nextButton_, &deleteButton_ })
        btn->setColour (juce::TextButton::textColourOffId, accentColour_.withAlpha (0.70f));

    saveButton_.setColour (juce::TextButton::textColourOffId, accentColour_.withAlpha (0.55f));

    repaint();
}

//==============================================================================
void TopHeaderBar::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // ---- Background: very dark gradient with minimal accent tint ----
    juce::ColourGradient bg (barBgTop (accentColour_), 0.0f, 0.0f,
                             barBgBottom (accentColour_), 0.0f, h, false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

    // ---- Bottom separator: shadow + accent line ----
    // Soft shadow.
    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.fillRect (0.0f, h - 2.0f, w, 2.0f);
    // Thin accent line.
    g.setColour (barSeparator (accentColour_));
    g.drawHorizontalLine (juce::roundToInt (h) - 1, 0.0f, w);

    // ---- Logo wordmark ----
    // The SVG (375×225) has dead space above the wordmark content.
    // Draw the full image at 1.5× bar height and nudge it down by half
    // the image height so the upper dead-space is clipped away by the
    // component bounds and the visible text lands centred in the bar.
    if (logoImage_.isValid())
    {
        const float logoH  = h * 1.5f;
        const float aspect = static_cast<float> (logoImage_.getWidth())
                           / static_cast<float> (juce::jmax (1, logoImage_.getHeight()));
        const float logoW  = logoH * aspect;
        const float logoX  = 8.0f * uiScale_;
        const float logoY  = h * 0.125f - 2.0f * uiScale_;  // nudged up 2 design-px

        g.setOpacity (0.78f);
        g.drawImage (logoImage_,
                     juce::roundToInt (logoX), juce::roundToInt (logoY),
                     juce::roundToInt (logoW), juce::roundToInt (logoH),
                     0, 0, logoImage_.getWidth(), logoImage_.getHeight());
        g.setOpacity (1.0f);
    }
}

void TopHeaderBar::resized()
{
    auto s = [this] (int v)
    {
        return juce::roundToInt (static_cast<float> (v) * uiScale_);
    };

    const int h = getHeight();
    const int btnH = juce::jmin (s (24), h - s (6));
    const int navBtnW = s (22);
    const int actionBtnW = s (22);
    const int comboW = s (150);
    const int engineW = s (80);
    const int gap = s (3);
    const int sectionGap = s (10);   // gap between preset cluster and engine selector

    // Total width of the preset cluster + engine selector:
    // [prev][gap][next][gap][combo][gap][save][gap][delete] [sectionGap] [engine]
    const int presetClusterW = navBtnW + gap + navBtnW + gap + comboW + gap + actionBtnW + gap + actionBtnW;
    const int totalW = presetClusterW + (engineSelector_ != nullptr ? sectionGap + engineW : 0);

    // Centre everything in the full window width.
    const int startX = (getWidth() - totalW) / 2;
    const int clusterY = (h - btnH) / 2;

    int x = startX;

    prevButton_.setBounds (x, clusterY, navBtnW, btnH);
    x += navBtnW + gap;

    nextButton_.setBounds (x, clusterY, navBtnW, btnH);
    x += navBtnW + gap;

    presetMenu_.setBounds (x, clusterY, comboW, btnH);
    x += comboW + gap;

    saveButton_.setBounds (x, clusterY, actionBtnW, btnH);
    x += actionBtnW + gap;

    deleteButton_.setBounds (x, clusterY, actionBtnW, btnH);
    x += actionBtnW;

    if (engineSelector_ != nullptr)
    {
        x += sectionGap;
        engineSelector_->setBounds (x, clusterY, engineW, btnH);
    }
}

//==============================================================================
void TopHeaderBar::presetChanged (const juce::String& name)
{
    DBG("presetChanged: name=" + name);
    refreshPresetMenu();
}

void TopHeaderBar::presetListChanged()
{
    refreshPresetMenu();
}

void TopHeaderBar::refreshPresetMenu()
{
    const auto names = presetManager_.getPresetNames();
    const int currentIdx = presetManager_.getCurrentIndex();

    DBG("refreshPresetMenu: numItems=" + juce::String(names.size()) + " currentIdx=" + juce::String(currentIdx) + " selectedIdBefore=" + juce::String(presetMenu_.getSelectedId()));

    updatingPresetMenu_ = true;
    presetMenu_.clear (juce::dontSendNotification);

    for (int i = 0; i < names.size(); ++i)
        presetMenu_.addItem (names[i], i + 1);   // ComboBox IDs are 1-based.

    if (currentIdx >= 0 && names.size() > 0)
    {
        // A valid preset is active — select it.
        const int selectedId = juce::jlimit (1, names.size(), currentIdx + 1);
        DBG("  setting selectedId=" + juce::String(selectedId) + " (clamped from " + juce::String(currentIdx + 1) + ")");
        presetMenu_.setSelectedId (selectedId, juce::dontSendNotification);
    }
    else
    {
        // No preset active — deselect so the placeholder text shows.
        DBG("  no preset active or no items, deselecting");
        presetMenu_.setSelectedId (0, juce::dontSendNotification);
    }

    DBG("  selectedIdAfter=" + juce::String(presetMenu_.getSelectedId()));

    updatingPresetMenu_ = false;

    deleteButton_.setEnabled (currentIdx >= 0
                              && presetManager_.isUserPreset (currentIdx));
}

//==============================================================================
void TopHeaderBar::showSaveDialog()
{
    auto* alertWindow = new juce::AlertWindow (
        "Save Preset",
        "Enter a name for the preset:",
        juce::AlertWindow::NoIcon);

    alertWindow->setLookAndFeel (&getHeaderLookAndFeel());
    alertWindow->addTextEditor ("name", presetManager_.getCurrentPresetName(),
                                "Preset name:");
    alertWindow->addButton ("Save", 1);
    alertWindow->addButton ("Cancel", 0);

    alertWindow->enterModalState (true,
        juce::ModalCallbackFunction::create (
            [this, alertWindow] (int result)
            {
                if (result == 1)
                {
                    auto name = alertWindow->getTextEditorContents ("name").trim();
                    if (name.isNotEmpty())
                        presetManager_.saveUserPreset (name);
                }

                alertWindow->setLookAndFeel (nullptr);
                delete alertWindow;
            }),
        false);
}

void TopHeaderBar::showDeleteDialog()
{
    const int index = presetManager_.getCurrentIndex();
    if (! presetManager_.isUserPreset (index))
        return;

    auto* alertWindow = new juce::AlertWindow (
        "Delete Preset",
        "Delete \"" + presetManager_.getCurrentPresetName() + "\"?\nThis cannot be undone.",
        juce::AlertWindow::WarningIcon);

    alertWindow->setLookAndFeel (&getHeaderLookAndFeel());
    alertWindow->addButton ("Delete", 1);
    alertWindow->addButton ("Cancel", 0);

    alertWindow->enterModalState (true,
        juce::ModalCallbackFunction::create (
            [this, alertWindow, index] (int result)
            {
                if (result == 1)
                    presetManager_.deleteUserPreset (index);

                alertWindow->setLookAndFeel (nullptr);
                delete alertWindow;
            }),
        false);
}
