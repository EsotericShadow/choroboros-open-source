/*
 * Choroboros - A chorus that eats its own tail
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * Isolated window: scrollable lab — reference, premium, and experiment grids.
 * Build: cmake --build build --target ChoroborosKnobPhysicsLab
 * Run:   ./build/ChoroborosKnobPhysicsLab
 */

#include "UI/CustomLookAndFeel.h"
#include "UI/KnobPhysicsLabSlider.h"
#include "UI/PluginEditorSetup.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
constexpr int kNumSliders = 19;

struct CellDesc
{
    const char* title;
    const char* subtitle;
    KnobPhysicsLabSlider::PhysicsMode mode;
    /** -1 = green reference LnF; 0–5 = premium lane index; 6 = rotate theme by row. */
    int lookKind;
    bool useMixGraphic;
};

constexpr CellDesc kCells[kNumSliders] = {
    { "1 · Linear", "Stock JUCE drag", KnobPhysicsLabSlider::PhysicsMode::linearReference, -1, false },
    { "2 · Stiff → snap", "Heavy first 10 px, then stock P", KnobPhysicsLabSlider::PhysicsMode::stiffThenLinear, -1, false },
    { "3 · Stiff → ease", "P eases over 22 px after 8 px stiff", KnobPhysicsLabSlider::PhysicsMode::stiffEaseLinear, -1, false },
    { "4 · Always heavy", "Uniform ~2.35× px / unit", KnobPhysicsLabSlider::PhysicsMode::alwaysHeavy, -1, false },
    { "5 · Quick start", "Sensitive first 8 px, then stock", KnobPhysicsLabSlider::PhysicsMode::quickStart, -1, false },
    { "6 · Edge heavy", "Stiffer within 10% of min/max", KnobPhysicsLabSlider::PhysicsMode::edgeHeavy, -1, false },
    { "7 · Tight snap", "Stiff band 6 px", KnobPhysicsLabSlider::PhysicsMode::stiffSnapTight, -1, false },
    { "8 · Fast ease", "12 px ease after 6 px stiff", KnobPhysicsLabSlider::PhysicsMode::stiffEaseFast, -1, false },
    { "9 · Linear B", "Second stock reference", KnobPhysicsLabSlider::PhysicsMode::linearDuplicate, -1, false },
    { "10 · Velvet clutch", "Purple · peak-stable velvet (~56 px)", KnobPhysicsLabSlider::PhysicsMode::velvetClutch, 0, false },
    { "11 · Centre vault", "Blue · magnetic drag at 50%", KnobPhysicsLabSlider::PhysicsMode::centerVault, 1, false },
    { "12 · End magnets", "Red · vise in outer 25%", KnobPhysicsLabSlider::PhysicsMode::endMagnets, 2, false },
    { "13 · Sprint boost", "Green · flick gains extra throw", KnobPhysicsLabSlider::PhysicsMode::sprintBoost, 3, false },
    { "14 · Ice → fire", "Black · light, ramp heavy, then stock", KnobPhysicsLabSlider::PhysicsMode::iceThenFire, 4, false },
    { "15 · Showcase mix", "Green · Mix dial · stock linear", KnobPhysicsLabSlider::PhysicsMode::mixShowcase, 5, true },
    { "16 · Speed sense A", "Blue · smoothstep + EMA · balanced", KnobPhysicsLabSlider::PhysicsMode::speedSensitiveA, 1, false },
    { "17 · Speed sense B", "Blue · gentler, longer blend", KnobPhysicsLabSlider::PhysicsMode::speedSensitiveB, 1, false },
    { "18 · Speed sense C", "Blue · stronger slow-heavy / fast-light", KnobPhysicsLabSlider::PhysicsMode::speedSensitiveC, 1, false },
    { "19 · Speed sense D", "Blue · very long silky ramp", KnobPhysicsLabSlider::PhysicsMode::speedSensitiveD, 1, false },
};

constexpr int kPremiumEngineTheme[] = { 3, 1, 2, 0, 4, 0 };

constexpr juce::uint32 kValueColourRef = 0xffa8e6a8;
constexpr juce::uint32 kPremiumValueColours[] = {
    0xffdcc8ff, 0xffb8dcff, 0xffffc4c4, 0xffb8f0b8, 0xffd8d8d8, 0xffffe8a8
};

/** Scrollable inner surface (owned by Viewport). */
class KnobPhysicsLabScrollBody final : public juce::Component
{
public:
    KnobPhysicsLabScrollBody()
    {
        referenceLnF.setColorTheme (0);
        for (int pi = 0; pi < 6; ++pi)
            premiumLnF[static_cast<size_t> (pi)].setColorTheme (kPremiumEngineTheme[static_cast<size_t> (pi)]);

        for (int i = 0; i < kNumSliders; ++i)
        {
            auto& s = sliders[static_cast<size_t> (i)];
            s = std::make_unique<KnobPhysicsLabSlider> (kCells[i].mode);
            KnobPhysicsLabSlider& knob = *s;

            if (kCells[i].lookKind < 0)
                knob.setLookAndFeel (&referenceLnF);
            else
            {
                const int lane = juce::jlimit (0, 5, kCells[i].lookKind);
                knob.setLookAndFeel (&premiumLnF[static_cast<size_t> (lane)]);
            }

            knob.setRange (0.0, 100.0, 0.01);
            knob.setValue (50.0, juce::dontSendNotification);
            knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
            knob.setVelocityBasedMode (false);

            if (kCells[i].useMixGraphic)
            {
                knob.setComponentID ("Mix");
                knob.setName ("Mix");
            }
            else
            {
                knob.setComponentID ("Rate");
            }

            LayoutTuning layout;
            knob.getProperties().set ("knobSweepStartDeg", layout.knobSweepStartDeg);
            knob.getProperties().set ("knobSweepEndDeg", layout.knobSweepEndDeg);
            knob.getProperties().set ("knobFrameCount", layout.knobFrameCount);
            const float knobDragSensitivityScale = static_cast<float> (juce::jlimit (10, 400, layout.knobDragSensitivityPct)) * 0.01f;
            knob.setDragSensitivity (knobDragSensitivityScale);

            auto& title = titles[static_cast<size_t> (i)];
            title.setText (kCells[i].title, juce::dontSendNotification);
            title.setJustificationType (juce::Justification::centred);
            title.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
            title.setColour (juce::Label::textColourId, juce::Colours::white);

            auto& sub = subs[static_cast<size_t> (i)];
            sub.setText (kCells[i].subtitle, juce::dontSendNotification);
            sub.setJustificationType (juce::Justification::centredTop);
            sub.setFont (juce::Font (juce::FontOptions (10.5f)));
            sub.setColour (juce::Label::textColourId, juce::Colour (0xffbbbbbb));

            auto& val = values[static_cast<size_t> (i)];
            val.setText ("50.0", juce::dontSendNotification);
            val.setJustificationType (juce::Justification::centred);
            val.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::plain)));
            if (kCells[i].lookKind < 0)
                val.setColour (juce::Label::textColourId, juce::Colour (kValueColourRef));
            else
            {
                const int lane = juce::jlimit (0, 5, kCells[i].lookKind);
                val.setColour (juce::Label::textColourId,
                               juce::Colour (kPremiumValueColours[static_cast<size_t> (lane)]));
            }

            const int idx = i;
            knob.onValueChange = [this, idx]
            {
                values[static_cast<size_t> (idx)].setText (
                    juce::String (sliders[static_cast<size_t> (idx)]->getValue(), 2),
                    juce::dontSendNotification);
            };

            addAndMakeVisible (knob);
            addAndMakeVisible (title);
            addAndMakeVisible (sub);
            addAndMakeVisible (val);
        }

        hint.setText ("Scroll to see all rows · Cmd/Ctrl + drag = fine · No easing after mouse up",
                      juce::dontSendNotification);
        hint.setJustificationType (juce::Justification::centredTop);
        hint.setFont (juce::Font (juce::FontOptions (11.5f)));
        hint.setColour (juce::Label::textColourId, juce::Colour (0xffcccccc));
        addAndMakeVisible (hint);

        refSection.setText ("Reference — green · main Rate knob", juce::dontSendNotification);
        refSection.setJustificationType (juce::Justification::centredLeft);
        refSection.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        refSection.setColour (juce::Label::textColourId, juce::Colour (0xff88dd88));
        addAndMakeVisible (refSection);

        premiumSection.setText ("Premium — engine-colour motifs", juce::dontSendNotification);
        premiumSection.setJustificationType (juce::Justification::centredLeft);
        premiumSection.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        premiumSection.setColour (juce::Label::textColourId, juce::Colour (0xffddaa66));
        addAndMakeVisible (premiumSection);

        experimentSection.setText ("Speed sense — four blue tunings (same family)", juce::dontSendNotification);
        experimentSection.setJustificationType (juce::Justification::centredLeft);
        experimentSection.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        experimentSection.setColour (juce::Label::textColourId, juce::Colour (0xffaaaaff));
        addAndMakeVisible (experimentSection);
    }

    ~KnobPhysicsLabScrollBody() override
    {
        for (auto& s : sliders)
            if (s != nullptr)
                s->setLookAndFeel (nullptr);
    }

    int computeHeightForWidth (int contentWidth) const
    {
        juce::ignoreUnused (contentWidth);
        constexpr int knobSize = 128;
        constexpr int rowGap = 8;
        constexpr int titleH = 20;
        constexpr int subH = 44;
        constexpr int valH = 18;
        const int cellH = titleH + subH + knobSize + valH + rowGap;

        const int margin = 14;
        const int hintH = 40;
        const int sectionH = 24;
        const int betweenSections = 14;
        const int bottomPad = 24;

        return margin + hintH + 8 + sectionH + 6 + 3 * cellH + betweenSections + sectionH + 6 + 2 * cellH
               + betweenSections + sectionH + 6 + 2 * cellH + bottomPad + margin;
    }

    void resized() override
    {
        constexpr int knobSize = 128;
        constexpr int rowGap = 8;
        constexpr int titleH = 20;
        constexpr int subH = 44;
        constexpr int valH = 18;
        const int cellH = titleH + subH + knobSize + valH + rowGap;

        const int margin = 14;
        const int hintH = 40;
        const int sectionH = 24;
        const int betweenSections = 14;
        const int bottomPad = 24;

        auto r = getLocalBounds().reduced (margin);
        hint.setBounds (r.removeFromTop (hintH));
        r.removeFromTop (8);

        refSection.setBounds (r.removeFromTop (sectionH));
        r.removeFromTop (6);
        layoutGrid (r.removeFromTop (3 * cellH), 0, 9, 3, 3, cellH, knobSize, titleH, subH, valH);

        r.removeFromTop (betweenSections);
        premiumSection.setBounds (r.removeFromTop (sectionH));
        r.removeFromTop (6);
        layoutGrid (r.removeFromTop (2 * cellH), 9, 6, 2, 3, cellH, knobSize, titleH, subH, valH);

        r.removeFromTop (betweenSections);
        experimentSection.setBounds (r.removeFromTop (sectionH));
        r.removeFromTop (6);
        layoutGrid (r.removeFromTop (2 * cellH), 15, 4, 2, 3, cellH, knobSize, titleH, subH, valH);

        r.removeFromTop (bottomPad);
    }

private:
    void layoutGrid (juce::Rectangle<int> area, int firstIndex, int count,
                     int rows, int cols, int cellH, int knobSize,
                     int titleH, int subH, int valH)
    {
        const int colW = juce::jmax (1, area.getWidth() / cols);
        for (int row = 0; row < rows; ++row)
        {
            auto rowR = area.removeFromTop (cellH);
            for (int col = 0; col < cols; ++col)
            {
                const int i = firstIndex + row * cols + col;
                if (i >= firstIndex + count)
                    return;
                auto cell = rowR.removeFromLeft (colW).reduced (3, 0);
                titles[static_cast<size_t> (i)].setBounds (cell.removeFromTop (titleH));
                subs[static_cast<size_t> (i)].setBounds (cell.removeFromTop (subH));
                auto knobArea = cell.removeFromTop (knobSize);
                const int kx = knobArea.getCentreX() - knobSize / 2;
                sliders[static_cast<size_t> (i)]->setBounds (kx, knobArea.getY(), knobSize, knobSize);
                values[static_cast<size_t> (i)].setBounds (cell.removeFromTop (valH));
            }
        }
    }

    CustomLookAndFeel referenceLnF;
    std::array<CustomLookAndFeel, 6> premiumLnF {};
    std::array<std::unique_ptr<KnobPhysicsLabSlider>, kNumSliders> sliders {};
    std::array<juce::Label, kNumSliders> titles {};
    std::array<juce::Label, kNumSliders> subs {};
    std::array<juce::Label, kNumSliders> values {};
    juce::Label hint;
    juce::Label refSection;
    juce::Label premiumSection;
    juce::Label experimentSection;
};

class KnobPhysicsLabComponent final : public juce::Component
{
public:
    KnobPhysicsLabComponent()
    {
        scrollBody = std::make_unique<KnobPhysicsLabScrollBody>();
        viewport.setViewedComponent (scrollBody.get(), false);
        viewport.setScrollBarsShown (true, false);
        viewport.getVerticalScrollBar().setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xff666688));
        addAndMakeVisible (viewport);
    }

    ~KnobPhysicsLabComponent() override
    {
        viewport.setViewedComponent (nullptr, false);
        scrollBody.reset();
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());
        if (scrollBody != nullptr)
        {
            const int vw = juce::jmax (320, viewport.getViewWidth());
            const int vh = scrollBody->computeHeightForWidth (vw);
            scrollBody->setSize (vw, vh);
        }
    }

private:
    juce::Viewport viewport;
    std::unique_ptr<KnobPhysicsLabScrollBody> scrollBody;
};

class KnobPhysicsLabWindow final : public juce::DocumentWindow
{
public:
    KnobPhysicsLabWindow()
        : DocumentWindow ("Choroboros · Knob physics lab",
                          juce::Desktop::getInstance().getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
                          DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new KnobPhysicsLabComponent(), true);
        setResizable (true, true);
        setResizeLimits (480, 520, 2200, 2600);
        setSize (580, 760);
        centreWithSize (getWidth(), getHeight());
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};
}

struct KnobPhysicsLabApp final : public juce::JUCEApplication
{
    const juce::String getApplicationName() override { return "ChoroborosKnobPhysicsLab"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<KnobPhysicsLabWindow>();
        mainWindow->setVisible (true);
    }

    void shutdown() override { mainWindow.reset(); }

private:
    std::unique_ptr<KnobPhysicsLabWindow> mainWindow;
};

START_JUCE_APPLICATION (KnobPhysicsLabApp)
