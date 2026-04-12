/*
 * Choroboros - Custom engine data model and persistence
 */

#include "CustomEngineManager.h"

namespace choroboros
{

#define KZN_TUNING_FIELD(obj, tuning, field) \
    (obj).setProperty(#field, static_cast<double>((tuning).field.load()))

#define KZN_TUNING_READ(src, tuning, field) \
    if ((src).hasProperty(#field)) (tuning).field.store(static_cast<float>(static_cast<double>((src)[#field])))

namespace
{

juce::Uuid factoryUuid(int index)
{
    static const char* uuids[5] = {
        "00000000-0000-0000-0000-000000000001",
        "00000000-0000-0000-0000-000000000002",
        "00000000-0000-0000-0000-000000000003",
        "00000000-0000-0000-0000-000000000004",
        "00000000-0000-0000-0000-000000000005",
    };
    return juce::Uuid(uuids[juce::jlimit(0, 4, index)]);
}

CustomEngine makeFactoryEngine(int index)
{
    static const char* names[5] = { "Green", "Blue", "Red", "Purple", "Black" };

    CustomEngine engine;
    engine.id = factoryUuid(index);
    engine.name = names[juce::jlimit(0, 4, index)];
    engine.visual = getFactoryVisual(index);
    engine.isFactory = true;
    engine.factoryIndex = index;
    return engine;
}

} // namespace

CustomEngineManager::CustomEngineManager() = default;

juce::File CustomEngineManager::getEnginesDirectory() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
        .getChildFile("Application Support")
#endif
        .getChildFile("Kaizen DSP")
        .getChildFile("Choroboros")
        .getChildFile("Engines");

    dir.createDirectory();
    return dir;
}

juce::File CustomEngineManager::getRegistryFile() const
{
    return getEnginesDirectory().getChildFile("engines.json");
}

juce::File CustomEngineManager::getEngineFile(const juce::Uuid& id) const
{
    return getEnginesDirectory().getChildFile(id.toString() + ".json");
}

CustomEngine* CustomEngineManager::getEngineById(const juce::Uuid& id)
{
    for (auto& engine : engines_)
        if (engine.id == id)
            return &engine;
    return nullptr;
}

const CustomEngine* CustomEngineManager::getEngineById(const juce::Uuid& id) const
{
    for (const auto& engine : engines_)
        if (engine.id == id)
            return &engine;
    return nullptr;
}

int CustomEngineManager::getEngineIndex(const juce::Uuid& id) const
{
    for (int i = 0; i < static_cast<int>(engines_.size()); ++i)
        if (engines_[static_cast<size_t>(i)].id == id)
            return i;
    return -1;
}

const CustomEngine* CustomEngineManager::getFactoryEngine(int colorIndex) const
{
    if (colorIndex < 0 || colorIndex >= kNumFactoryEngines || colorIndex >= static_cast<int>(engines_.size()))
        return nullptr;
    return &engines_[static_cast<size_t>(colorIndex)];
}

CustomEngine* CustomEngineManager::getFactoryEngine(int colorIndex)
{
    if (colorIndex < 0 || colorIndex >= kNumFactoryEngines || colorIndex >= static_cast<int>(engines_.size()))
        return nullptr;
    return &engines_[static_cast<size_t>(colorIndex)];
}

CustomEngine* CustomEngineManager::getSingleCustomEngine()
{
    for (auto& engine : engines_)
        if (!engine.isFactory)
            return &engine;
    return nullptr;
}

const CustomEngine* CustomEngineManager::getSingleCustomEngine() const
{
    for (const auto& engine : engines_)
        if (!engine.isFactory)
            return &engine;
    return nullptr;
}

juce::Uuid CustomEngineManager::createEngine(const juce::String& name)
{
    CustomEngine engine;
    engine.id = juce::Uuid();
    engine.name = name.isEmpty() ? "New Engine" : name;
    engine.visual = getWhiteVisual();
    engines_.push_back(std::move(engine));

    saveEngineToDisk(engines_.back());
    saveRegistry();
    listeners_.call(&Listener::engineListChanged);
    return engines_.back().id;
}

bool CustomEngineManager::deleteEngine(const juce::Uuid& id)
{
    for (auto it = engines_.begin(); it != engines_.end(); ++it)
    {
        if (it->id == id)
        {
            if (it->isFactory)
                return false;

            getEngineFile(id).deleteFile();
            engines_.erase(it);
            saveRegistry();
            listeners_.call(&Listener::engineListChanged);
            return true;
        }
    }

    return false;
}

bool CustomEngineManager::renameEngine(const juce::Uuid& id, const juce::String& newName)
{
    auto* engine = getEngineById(id);
    if (engine == nullptr || newName.isEmpty() || engine->isFactory)
        return false;

    engine->name = newName;
    saveEngineToDisk(*engine);
    saveRegistry();
    listeners_.call(&Listener::engineListChanged);
    return true;
}

void CustomEngineManager::saveEngine(const juce::Uuid& id)
{
    if (auto* engine = getEngineById(id))
    {
        saveEngineToDisk(*engine);
        saveRegistry();
    }
}

void CustomEngineManager::saveRegistry()
{
    auto* root = new juce::DynamicObject();
    root->setProperty("version", 1);

    juce::Array<juce::var> engineList;
    for (const auto& engine : engines_)
    {
        if (engine.isFactory)
            continue;

        auto* entry = new juce::DynamicObject();
        entry->setProperty("id", engine.id.toString());
        entry->setProperty("name", engine.name);
        engineList.add(juce::var(entry));
    }

    root->setProperty("engines", engineList);
    getRegistryFile().replaceWithText(juce::JSON::toString(juce::var(root)));
}

void CustomEngineManager::loadAll()
{
    engines_.clear();

    for (int i = 0; i < kNumFactoryEngines; ++i)
        engines_.push_back(makeFactoryEngine(i));

    const auto registryFile = getRegistryFile();
    if (!registryFile.existsAsFile())
        return;

    const auto json = juce::JSON::parse(registryFile);
    if (!json.isObject())
        return;

    const auto* engineArray = json["engines"].getArray();
    if (engineArray == nullptr)
        return;

    for (const auto& entry : *engineArray)
    {
        if (!entry.isObject())
            continue;

        const juce::Uuid id(entry["id"].toString());
        if (id.isNull())
            continue;

        CustomEngine engine;
        if (loadEngineFromDisk(id, engine))
            engines_.push_back(std::move(engine));
    }
}

void CustomEngineManager::tuningToJson(juce::DynamicObject& obj, const ChorusDSP::RuntimeTuning& t)
{
    KZN_TUNING_FIELD(obj, t, rateSmoothingMs);
    KZN_TUNING_FIELD(obj, t, depthSmoothingMs);
    KZN_TUNING_FIELD(obj, t, depthRateLimit);
    KZN_TUNING_FIELD(obj, t, centreDelaySmoothingMs);
    KZN_TUNING_FIELD(obj, t, colorSmoothingMs);
    KZN_TUNING_FIELD(obj, t, widthSmoothingMs);
    KZN_TUNING_FIELD(obj, t, centreDelayBaseMs);
    KZN_TUNING_FIELD(obj, t, centreDelayScale);
    KZN_TUNING_FIELD(obj, t, hpfCutoffHz);
    KZN_TUNING_FIELD(obj, t, hpfQ);
    KZN_TUNING_FIELD(obj, t, lpfCutoffHz);
    KZN_TUNING_FIELD(obj, t, lpfQ);
    KZN_TUNING_FIELD(obj, t, preEmphasisFreqHz);
    KZN_TUNING_FIELD(obj, t, preEmphasisQ);
    KZN_TUNING_FIELD(obj, t, preEmphasisGain);
    KZN_TUNING_FIELD(obj, t, preEmphasisLevelSmoothing);
    KZN_TUNING_FIELD(obj, t, preEmphasisQuietThreshold);
    KZN_TUNING_FIELD(obj, t, preEmphasisMaxAmount);
    KZN_TUNING_FIELD(obj, t, compressorAttackMs);
    KZN_TUNING_FIELD(obj, t, compressorReleaseMs);
    KZN_TUNING_FIELD(obj, t, compressorThresholdDb);
    KZN_TUNING_FIELD(obj, t, compressorRatio);
    KZN_TUNING_FIELD(obj, t, saturationDriveScale);
    KZN_TUNING_FIELD(obj, t, greenBloomExponent);
    KZN_TUNING_FIELD(obj, t, greenBloomDepthScale);
    KZN_TUNING_FIELD(obj, t, greenBloomCentreOffsetMs);
    KZN_TUNING_FIELD(obj, t, greenBloomCutoffMaxHz);
    KZN_TUNING_FIELD(obj, t, greenBloomCutoffMinHz);
    KZN_TUNING_FIELD(obj, t, greenBloomWetBlend);
    KZN_TUNING_FIELD(obj, t, greenBloomGain);
    KZN_TUNING_FIELD(obj, t, blueFocusExponent);
    KZN_TUNING_FIELD(obj, t, blueFocusHpMinHz);
    KZN_TUNING_FIELD(obj, t, blueFocusHpMaxHz);
    KZN_TUNING_FIELD(obj, t, blueFocusLpMaxHz);
    KZN_TUNING_FIELD(obj, t, blueFocusLpMinHz);
    KZN_TUNING_FIELD(obj, t, bluePresenceFreqMinHz);
    KZN_TUNING_FIELD(obj, t, bluePresenceFreqMaxHz);
    KZN_TUNING_FIELD(obj, t, bluePresenceQMin);
    KZN_TUNING_FIELD(obj, t, bluePresenceQMax);
    KZN_TUNING_FIELD(obj, t, bluePresenceGainMaxDb);
    KZN_TUNING_FIELD(obj, t, blueFocusWetBlend);
    KZN_TUNING_FIELD(obj, t, blueFocusOutputGain);
    KZN_TUNING_FIELD(obj, t, purpleWarpA);
    KZN_TUNING_FIELD(obj, t, purpleWarpB);
    KZN_TUNING_FIELD(obj, t, purpleWarpKBase);
    KZN_TUNING_FIELD(obj, t, purpleWarpKScale);
    KZN_TUNING_FIELD(obj, t, purpleWarpDelaySmoothingMs);
    KZN_TUNING_FIELD(obj, t, purpleOrbitEccentricity);
    KZN_TUNING_FIELD(obj, t, purpleOrbitThetaRateBaseHz);
    KZN_TUNING_FIELD(obj, t, purpleOrbitThetaRateScaleHz);
    KZN_TUNING_FIELD(obj, t, purpleOrbitThetaRate2Ratio);
    KZN_TUNING_FIELD(obj, t, purpleOrbitEccentricity2Ratio);
    KZN_TUNING_FIELD(obj, t, purpleOrbitMix1);
    KZN_TUNING_FIELD(obj, t, purpleOrbitStereoThetaOffset);
    KZN_TUNING_FIELD(obj, t, purpleOrbitDelaySmoothingMs);
    KZN_TUNING_FIELD(obj, t, blackNqDepthBase);
    KZN_TUNING_FIELD(obj, t, blackNqDepthScale);
    KZN_TUNING_FIELD(obj, t, blackNqDelayGlideMs);
    KZN_TUNING_FIELD(obj, t, blackHqTap2MixBase);
    KZN_TUNING_FIELD(obj, t, blackHqTap2MixScale);
    KZN_TUNING_FIELD(obj, t, blackHqSecondTapDepthBase);
    KZN_TUNING_FIELD(obj, t, blackHqSecondTapDepthScale);
    KZN_TUNING_FIELD(obj, t, blackHqSecondTapDelayOffsetBase);
    KZN_TUNING_FIELD(obj, t, blackHqSecondTapDelayOffsetScale);
    KZN_TUNING_FIELD(obj, t, bbdDelaySmoothingMs);
    KZN_TUNING_FIELD(obj, t, bbdDelayMinMs);
    KZN_TUNING_FIELD(obj, t, bbdDelayMaxMs);
    KZN_TUNING_FIELD(obj, t, bbdCentreBaseMs);
    KZN_TUNING_FIELD(obj, t, bbdCentreScale);
    KZN_TUNING_FIELD(obj, t, bbdDepthMs);
    KZN_TUNING_FIELD(obj, t, bbdClockSmoothingMs);
    KZN_TUNING_FIELD(obj, t, bbdFilterSmoothingMs);
    KZN_TUNING_FIELD(obj, t, bbdFilterCutoffMinHz);
    KZN_TUNING_FIELD(obj, t, bbdFilterCutoffMaxHz);
    KZN_TUNING_FIELD(obj, t, bbdFilterCutoffScale);
    KZN_TUNING_FIELD(obj, t, bbdClockMinHz);
    KZN_TUNING_FIELD(obj, t, bbdClockMaxRatio);
    KZN_TUNING_FIELD(obj, t, bbdStages);
    KZN_TUNING_FIELD(obj, t, bbdFilterMaxRatio);
    KZN_TUNING_FIELD(obj, t, tapeDelaySmoothingMs);
    KZN_TUNING_FIELD(obj, t, tapeCentreBaseMs);
    KZN_TUNING_FIELD(obj, t, tapeCentreScale);
    KZN_TUNING_FIELD(obj, t, tapeToneMaxHz);
    KZN_TUNING_FIELD(obj, t, tapeToneMinHz);
    KZN_TUNING_FIELD(obj, t, tapeToneSmoothingCoeff);
    KZN_TUNING_FIELD(obj, t, tapeDriveScale);
    KZN_TUNING_FIELD(obj, t, tapeLfoRatioScale);
    KZN_TUNING_FIELD(obj, t, tapeLfoModSmoothingCoeff);
    KZN_TUNING_FIELD(obj, t, tapeRatioSmoothingCoeff);
    KZN_TUNING_FIELD(obj, t, tapePhaseDampingPerSec);
    KZN_TUNING_FIELD(obj, t, tapeWowFreqBase);
    KZN_TUNING_FIELD(obj, t, tapeWowFreqSpread);
    KZN_TUNING_FIELD(obj, t, tapeFlutterFreqBase);
    KZN_TUNING_FIELD(obj, t, tapeFlutterFreqSpread);
    KZN_TUNING_FIELD(obj, t, tapeWowDepthBase);
    KZN_TUNING_FIELD(obj, t, tapeWowDepthSpread);
    KZN_TUNING_FIELD(obj, t, tapeFlutterDepthBase);
    KZN_TUNING_FIELD(obj, t, tapeFlutterDepthSpread);
    KZN_TUNING_FIELD(obj, t, tapeRatioMin);
    KZN_TUNING_FIELD(obj, t, tapeRatioMax);
    KZN_TUNING_FIELD(obj, t, tapeWetGain);
    KZN_TUNING_FIELD(obj, t, tapeHermiteTension);
}

void CustomEngineManager::tuningFromJson(const juce::var& src, ChorusDSP::RuntimeTuning& t)
{
    if (!src.isObject())
        return;

    KZN_TUNING_READ(src, t, rateSmoothingMs);
    KZN_TUNING_READ(src, t, depthSmoothingMs);
    KZN_TUNING_READ(src, t, depthRateLimit);
    KZN_TUNING_READ(src, t, centreDelaySmoothingMs);
    KZN_TUNING_READ(src, t, colorSmoothingMs);
    KZN_TUNING_READ(src, t, widthSmoothingMs);
    KZN_TUNING_READ(src, t, centreDelayBaseMs);
    KZN_TUNING_READ(src, t, centreDelayScale);
    KZN_TUNING_READ(src, t, hpfCutoffHz);
    KZN_TUNING_READ(src, t, hpfQ);
    KZN_TUNING_READ(src, t, lpfCutoffHz);
    KZN_TUNING_READ(src, t, lpfQ);
    KZN_TUNING_READ(src, t, preEmphasisFreqHz);
    KZN_TUNING_READ(src, t, preEmphasisQ);
    KZN_TUNING_READ(src, t, preEmphasisGain);
    KZN_TUNING_READ(src, t, preEmphasisLevelSmoothing);
    KZN_TUNING_READ(src, t, preEmphasisQuietThreshold);
    KZN_TUNING_READ(src, t, preEmphasisMaxAmount);
    KZN_TUNING_READ(src, t, compressorAttackMs);
    KZN_TUNING_READ(src, t, compressorReleaseMs);
    KZN_TUNING_READ(src, t, compressorThresholdDb);
    KZN_TUNING_READ(src, t, compressorRatio);
    KZN_TUNING_READ(src, t, saturationDriveScale);
    KZN_TUNING_READ(src, t, greenBloomExponent);
    KZN_TUNING_READ(src, t, greenBloomDepthScale);
    KZN_TUNING_READ(src, t, greenBloomCentreOffsetMs);
    KZN_TUNING_READ(src, t, greenBloomCutoffMaxHz);
    KZN_TUNING_READ(src, t, greenBloomCutoffMinHz);
    KZN_TUNING_READ(src, t, greenBloomWetBlend);
    KZN_TUNING_READ(src, t, greenBloomGain);
    KZN_TUNING_READ(src, t, blueFocusExponent);
    KZN_TUNING_READ(src, t, blueFocusHpMinHz);
    KZN_TUNING_READ(src, t, blueFocusHpMaxHz);
    KZN_TUNING_READ(src, t, blueFocusLpMaxHz);
    KZN_TUNING_READ(src, t, blueFocusLpMinHz);
    KZN_TUNING_READ(src, t, bluePresenceFreqMinHz);
    KZN_TUNING_READ(src, t, bluePresenceFreqMaxHz);
    KZN_TUNING_READ(src, t, bluePresenceQMin);
    KZN_TUNING_READ(src, t, bluePresenceQMax);
    KZN_TUNING_READ(src, t, bluePresenceGainMaxDb);
    KZN_TUNING_READ(src, t, blueFocusWetBlend);
    KZN_TUNING_READ(src, t, blueFocusOutputGain);
    KZN_TUNING_READ(src, t, purpleWarpA);
    KZN_TUNING_READ(src, t, purpleWarpB);
    KZN_TUNING_READ(src, t, purpleWarpKBase);
    KZN_TUNING_READ(src, t, purpleWarpKScale);
    KZN_TUNING_READ(src, t, purpleWarpDelaySmoothingMs);
    KZN_TUNING_READ(src, t, purpleOrbitEccentricity);
    KZN_TUNING_READ(src, t, purpleOrbitThetaRateBaseHz);
    KZN_TUNING_READ(src, t, purpleOrbitThetaRateScaleHz);
    KZN_TUNING_READ(src, t, purpleOrbitThetaRate2Ratio);
    KZN_TUNING_READ(src, t, purpleOrbitEccentricity2Ratio);
    KZN_TUNING_READ(src, t, purpleOrbitMix1);
    KZN_TUNING_READ(src, t, purpleOrbitStereoThetaOffset);
    KZN_TUNING_READ(src, t, purpleOrbitDelaySmoothingMs);
    KZN_TUNING_READ(src, t, blackNqDepthBase);
    KZN_TUNING_READ(src, t, blackNqDepthScale);
    KZN_TUNING_READ(src, t, blackNqDelayGlideMs);
    KZN_TUNING_READ(src, t, blackHqTap2MixBase);
    KZN_TUNING_READ(src, t, blackHqTap2MixScale);
    KZN_TUNING_READ(src, t, blackHqSecondTapDepthBase);
    KZN_TUNING_READ(src, t, blackHqSecondTapDepthScale);
    KZN_TUNING_READ(src, t, blackHqSecondTapDelayOffsetBase);
    KZN_TUNING_READ(src, t, blackHqSecondTapDelayOffsetScale);
    KZN_TUNING_READ(src, t, bbdDelaySmoothingMs);
    KZN_TUNING_READ(src, t, bbdDelayMinMs);
    KZN_TUNING_READ(src, t, bbdDelayMaxMs);
    KZN_TUNING_READ(src, t, bbdCentreBaseMs);
    KZN_TUNING_READ(src, t, bbdCentreScale);
    KZN_TUNING_READ(src, t, bbdDepthMs);
    KZN_TUNING_READ(src, t, bbdClockSmoothingMs);
    KZN_TUNING_READ(src, t, bbdFilterSmoothingMs);
    KZN_TUNING_READ(src, t, bbdFilterCutoffMinHz);
    KZN_TUNING_READ(src, t, bbdFilterCutoffMaxHz);
    KZN_TUNING_READ(src, t, bbdFilterCutoffScale);
    KZN_TUNING_READ(src, t, bbdClockMinHz);
    KZN_TUNING_READ(src, t, bbdClockMaxRatio);
    KZN_TUNING_READ(src, t, bbdStages);
    KZN_TUNING_READ(src, t, bbdFilterMaxRatio);
    KZN_TUNING_READ(src, t, tapeDelaySmoothingMs);
    KZN_TUNING_READ(src, t, tapeCentreBaseMs);
    KZN_TUNING_READ(src, t, tapeCentreScale);
    KZN_TUNING_READ(src, t, tapeToneMaxHz);
    KZN_TUNING_READ(src, t, tapeToneMinHz);
    KZN_TUNING_READ(src, t, tapeToneSmoothingCoeff);
    KZN_TUNING_READ(src, t, tapeDriveScale);
    KZN_TUNING_READ(src, t, tapeLfoRatioScale);
    KZN_TUNING_READ(src, t, tapeLfoModSmoothingCoeff);
    KZN_TUNING_READ(src, t, tapeRatioSmoothingCoeff);
    KZN_TUNING_READ(src, t, tapePhaseDampingPerSec);
    KZN_TUNING_READ(src, t, tapeWowFreqBase);
    KZN_TUNING_READ(src, t, tapeWowFreqSpread);
    KZN_TUNING_READ(src, t, tapeFlutterFreqBase);
    KZN_TUNING_READ(src, t, tapeFlutterFreqSpread);
    KZN_TUNING_READ(src, t, tapeWowDepthBase);
    KZN_TUNING_READ(src, t, tapeWowDepthSpread);
    KZN_TUNING_READ(src, t, tapeFlutterDepthBase);
    KZN_TUNING_READ(src, t, tapeFlutterDepthSpread);
    KZN_TUNING_READ(src, t, tapeRatioMin);
    KZN_TUNING_READ(src, t, tapeRatioMax);
    KZN_TUNING_READ(src, t, tapeWetGain);
    KZN_TUNING_READ(src, t, tapeHermiteTension);
}

void applyBaseThemeDefaults(CustomEngineVisual& visual, int factoryColorIndex)
{
    visual = getFactoryVisual(factoryColorIndex);
    visual.hasCustomRateValueColour = false;
    visual.hasCustomDepthValueColour = false;
    visual.hasCustomOffsetValueColour = false;
    visual.hasCustomWidthValueColour = false;
    visual.hasCustomColorValueColour = false;
    visual.hasCustomMixValueColour = false;
}

void CustomEngineManager::visualToJson(juce::DynamicObject& obj, const CustomEngineVisual& v)
{
    obj.setProperty("rate_knob_sprite", v.rateKnobSprite);
    obj.setProperty("depth_knob_sprite", v.depthKnobSprite);
    obj.setProperty("offset_knob_sprite", v.offsetKnobSprite);
    obj.setProperty("width_knob_sprite", v.widthKnobSprite);
    obj.setProperty("mix_knob_sprite", v.mixKnobSprite);
    obj.setProperty("slider_thumb_sprite", v.sliderThumbSprite);
    obj.setProperty("background_set", v.backgroundSet);
    obj.setProperty("accent_colour", v.accentColour.toString());
    obj.setProperty("value_text_colour", v.valueTextColour.toString());
    obj.setProperty("rate_value_colour", v.rateValueColour.toString());
    obj.setProperty("has_custom_rate_value_colour", v.hasCustomRateValueColour);
    obj.setProperty("depth_value_colour", v.depthValueColour.toString());
    obj.setProperty("has_custom_depth_value_colour", v.hasCustomDepthValueColour);
    obj.setProperty("offset_value_colour", v.offsetValueColour.toString());
    obj.setProperty("has_custom_offset_value_colour", v.hasCustomOffsetValueColour);
    obj.setProperty("width_value_colour", v.widthValueColour.toString());
    obj.setProperty("has_custom_width_value_colour", v.hasCustomWidthValueColour);
    obj.setProperty("color_value_colour", v.colorValueColour.toString());
    obj.setProperty("has_custom_color_value_colour", v.hasCustomColorValueColour);
    obj.setProperty("mix_value_colour", v.mixValueColour.toString());
    obj.setProperty("has_custom_mix_value_colour", v.hasCustomMixValueColour);
}

void CustomEngineManager::visualFromJson(const juce::var& src, CustomEngineVisual& v)
{
    if (!src.isObject())
        return;

    v.rateKnobSprite = juce::jlimit(0, kMainKnobSpriteCount - 1, static_cast<int>(src["rate_knob_sprite"]));
    v.depthKnobSprite = juce::jlimit(0, kMainKnobSpriteCount - 1, static_cast<int>(src["depth_knob_sprite"]));
    v.offsetKnobSprite = juce::jlimit(0, kMainKnobSpriteCount - 1, static_cast<int>(src["offset_knob_sprite"]));
    v.widthKnobSprite = juce::jlimit(0, kMainKnobSpriteCount - 1, static_cast<int>(src["width_knob_sprite"]));
    v.mixKnobSprite = juce::jlimit(0, kMixKnobSpriteCount - 1, static_cast<int>(src["mix_knob_sprite"]));
    v.sliderThumbSprite = juce::jlimit(0, kSliderThumbCount - 1, static_cast<int>(src["slider_thumb_sprite"]));
    v.backgroundSet = juce::jlimit(0, kBackgroundSetCount - 1, static_cast<int>(src["background_set"]));

    if (src.hasProperty("accent_colour"))
        v.accentColour = juce::Colour::fromString(src["accent_colour"].toString());
    if (src.hasProperty("value_text_colour"))
        v.valueTextColour = juce::Colour::fromString(src["value_text_colour"].toString());

    const auto readOptColour = [&](const char* colourKey, const char* hasKey, juce::Colour& colour, bool& hasCustom)
    {
        if (src.hasProperty(hasKey))
            hasCustom = static_cast<bool>(src[hasKey]);
        if (src.hasProperty(colourKey))
            colour = juce::Colour::fromString(src[colourKey].toString());
    };

    readOptColour("rate_value_colour", "has_custom_rate_value_colour", v.rateValueColour, v.hasCustomRateValueColour);
    readOptColour("depth_value_colour", "has_custom_depth_value_colour", v.depthValueColour, v.hasCustomDepthValueColour);
    readOptColour("offset_value_colour", "has_custom_offset_value_colour", v.offsetValueColour, v.hasCustomOffsetValueColour);
    readOptColour("width_value_colour", "has_custom_width_value_colour", v.widthValueColour, v.hasCustomWidthValueColour);
    readOptColour("color_value_colour", "has_custom_color_value_colour", v.colorValueColour, v.hasCustomColorValueColour);
    readOptColour("mix_value_colour", "has_custom_mix_value_colour", v.mixValueColour, v.hasCustomMixValueColour);
}

void CustomEngineManager::saveEngineToDisk(const CustomEngine& engine)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("version", 2);
    root->setProperty("id", engine.id.toString());
    root->setProperty("name", engine.name);
    root->setProperty("nq_core", juce::String(coreIdToToken(engine.nqCore)));
    root->setProperty("hq_core", juce::String(coreIdToToken(engine.hqCore)));
    root->setProperty("knob_theme", engine.knobTheme);
    root->setProperty("accent_theme", engine.accentTheme);

    auto* visualObj = new juce::DynamicObject();
    visualToJson(*visualObj, engine.visual);
    root->setProperty("visual", juce::var(visualObj));

    auto* nqObj = new juce::DynamicObject();
    tuningToJson(*nqObj, *engine.nqTuning);
    root->setProperty("nq_tuning", juce::var(nqObj));

    auto* hqObj = new juce::DynamicObject();
    tuningToJson(*hqObj, *engine.hqTuning);
    root->setProperty("hq_tuning", juce::var(hqObj));

    getEngineFile(engine.id).replaceWithText(juce::JSON::toString(juce::var(root)));
}

bool CustomEngineManager::loadEngineFromDisk(const juce::Uuid& id, CustomEngine& out)
{
    const auto file = getEngineFile(id);
    if (!file.existsAsFile())
        return false;

    const auto json = juce::JSON::parse(file);
    if (!json.isObject())
        return false;

    out.id = id;
    out.name = json["name"].toString();

    CoreId nqCore = CoreId::lagrange3;
    CoreId hqCore = CoreId::lagrange5;
    if (parseCoreIdToken(json["nq_core"].toString().toStdString(), nqCore))
        out.nqCore = nqCore;
    if (parseCoreIdToken(json["hq_core"].toString().toStdString(), hqCore))
        out.hqCore = hqCore;

    out.knobTheme = static_cast<int>(json["knob_theme"]);
    out.accentTheme = static_cast<int>(json["accent_theme"]);

    const int version = static_cast<int>(json["version"]);
    if (version >= 2 && json.hasProperty("visual"))
        visualFromJson(json["visual"], out.visual);
    else
        applyBaseThemeDefaults(out.visual, juce::jlimit(0, 4, out.knobTheme));

    tuningFromJson(json["nq_tuning"], *out.nqTuning);
    tuningFromJson(json["hq_tuning"], *out.hqTuning);
    return true;
}

#undef KZN_TUNING_FIELD
#undef KZN_TUNING_READ

} // namespace choroboros
