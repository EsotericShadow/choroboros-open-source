/*
 * Choroboros - Custom engine data model and persistence
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include "../DSP/ChorusDSP.h"
#include "../DSP/CoreAssignments.h"
#include <vector>

namespace choroboros
{

// PLACEHOLDER - will be updated when white assets are created.
// These indices intentionally go beyond the current factory max to signal "white".
static constexpr int kWhiteKnobSprite = 12;
static constexpr int kWhiteMixKnobSprite = 5;
static constexpr int kWhiteSliderThumb = 5;
static constexpr int kWhiteBackgroundSet = 5;
static constexpr juce::uint32 kWhiteAccent = 0xffffffff;
static constexpr juce::uint32 kWhiteValueText = 0xffe0e0e0;

// Master knob sprite indices.
static constexpr int kMainKnobSpriteCount = 13;
static constexpr int kMixKnobSpriteCount  = 6;
static constexpr int kSliderThumbCount    = 6;
static constexpr int kBackgroundSetCount  = 6;

struct CustomEngineVisual
{
    int rateKnobSprite   = 0;
    int depthKnobSprite  = 1;
    int offsetKnobSprite = 2;
    int widthKnobSprite  = 3;

    int mixKnobSprite      = 0;
    int sliderThumbSprite  = 0;
    int backgroundSet      = 0;

    juce::Colour accentColour    { 0xff80ef80 };
    juce::Colour valueTextColour { 0xff80ef80 };

    juce::Colour rateValueColour   { 0 };
    juce::Colour depthValueColour  { 0 };
    juce::Colour offsetValueColour { 0 };
    juce::Colour widthValueColour  { 0 };
    juce::Colour colorValueColour  { 0 };
    juce::Colour mixValueColour    { 0 };

    bool hasCustomRateValueColour   = false;
    bool hasCustomDepthValueColour  = false;
    bool hasCustomOffsetValueColour = false;
    bool hasCustomWidthValueColour  = false;
    bool hasCustomColorValueColour  = false;
    bool hasCustomMixValueColour    = false;
};

inline CustomEngineVisual makeFactoryVisual(
    int rate, int depth, int offset, int width,
    int mix, int slider, int bg,
    juce::uint32 accent, juce::uint32 valueText)
{
    CustomEngineVisual v;
    v.rateKnobSprite = rate;
    v.depthKnobSprite = depth;
    v.offsetKnobSprite = offset;
    v.widthKnobSprite = width;
    v.mixKnobSprite = mix;
    v.sliderThumbSprite = slider;
    v.backgroundSet = bg;
    v.accentColour = juce::Colour(accent);
    v.valueTextColour = juce::Colour(valueText);
    return v;
}

inline const CustomEngineVisual& getFactoryVisual(int index)
{
    static const CustomEngineVisual configs[5] = {
        makeFactoryVisual(0, 1, 2, 3,   0, 0, 0, 0xff80ef80, 0xff9dbd78),
        makeFactoryVisual(9, 9, 9, 9,   2, 2, 1, 0xff7fb8ff, 0xff7fb8ff),
        makeFactoryVisual(4, 5, 6, 7,   1, 1, 2, 0xffff8d8b, 0xffff8d8b),
        makeFactoryVisual(10,10,10,10,  3, 3, 3, 0xffb88dd8, 0xffb88dd8),
        makeFactoryVisual(11,11,11,11,  4, 4, 4, 0xffffffff, 0xffd4d4d4),
    };
    return configs[juce::jlimit(0, 4, index)];
}

inline CustomEngineVisual getWhiteVisual()
{
    return makeFactoryVisual(
        kWhiteKnobSprite, kWhiteKnobSprite, kWhiteKnobSprite, kWhiteKnobSprite,
        kWhiteMixKnobSprite, kWhiteSliderThumb, kWhiteBackgroundSet,
        kWhiteAccent, kWhiteValueText);
}

void applyBaseThemeDefaults(CustomEngineVisual& visual, int factoryColorIndex);

struct CustomEngine
{
    juce::Uuid id;
    juce::String name;
    CoreId nqCore = CoreId::lagrange3;
    CoreId hqCore = CoreId::lagrange5;
    int knobTheme = 0;
    int accentTheme = 0;
    std::unique_ptr<ChorusDSP::RuntimeTuning> nqTuning = std::make_unique<ChorusDSP::RuntimeTuning>();
    std::unique_ptr<ChorusDSP::RuntimeTuning> hqTuning = std::make_unique<ChorusDSP::RuntimeTuning>();
    CustomEngineVisual visual;
    bool isFactory = false;
    int factoryIndex = -1;
};

class CustomEngineManager
{
public:
    CustomEngineManager();

    const std::vector<CustomEngine>& getEngines() const { return engines_; }
    int getNumEngines() const { return static_cast<int>(engines_.size()); }

    static constexpr int kNumFactoryEngines = 5;
    static constexpr int kMaxCustomEngines = 1;
    int getNumCustomEngines() const { return juce::jmax(0, getNumEngines() - kNumFactoryEngines); }

    const CustomEngine* getFactoryEngine(int colorIndex) const;
    CustomEngine* getFactoryEngine(int colorIndex);
    CustomEngine* getSingleCustomEngine();
    const CustomEngine* getSingleCustomEngine() const;

    CustomEngine* getEngineById(const juce::Uuid& id);
    const CustomEngine* getEngineById(const juce::Uuid& id) const;
    int getEngineIndex(const juce::Uuid& id) const;

    juce::Uuid createEngine(const juce::String& name);
    bool deleteEngine(const juce::Uuid& id);
    bool renameEngine(const juce::Uuid& id, const juce::String& newName);

    void saveEngine(const juce::Uuid& id);
    void saveRegistry();
    void loadAll();

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void engineListChanged() = 0;
    };

    void addListener(Listener* l)    { listeners_.add(l); }
    void removeListener(Listener* l) { listeners_.remove(l); }

private:
    std::vector<CustomEngine> engines_;
    juce::ListenerList<Listener> listeners_;

    juce::File getEnginesDirectory() const;
    juce::File getRegistryFile() const;
    juce::File getEngineFile(const juce::Uuid& id) const;

    void saveEngineToDisk(const CustomEngine& engine);
    bool loadEngineFromDisk(const juce::Uuid& id, CustomEngine& out);

    static void tuningToJson(juce::DynamicObject& obj, const ChorusDSP::RuntimeTuning& t);
    static void tuningFromJson(const juce::var& obj, ChorusDSP::RuntimeTuning& t);
    static void visualToJson(juce::DynamicObject& obj, const CustomEngineVisual& visual);
    static void visualFromJson(const juce::var& obj, CustomEngineVisual& visual);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomEngineManager)
};

} // namespace choroboros
