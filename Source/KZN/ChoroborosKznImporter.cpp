/*
 * Choroboros - .kzn preset importer
 */

#include "ChoroborosKznImporter.h"
#include "ChoroborosKznSchema.h"
#include "../DSP/CoreAssignments.h"
#include "../Engine/CustomEngineManager.h"
#include "../Plugin/PluginProcessor.h"

#include <kzn/KznFormat.h>
#include <kzn/KznVerifier.h>

namespace choroboros
{

namespace
{

bool getJsonDouble(const juce::var& obj, const char* key, double& out)
{
    if (!obj.hasProperty(key))
        return false;

    const auto& v = obj[key];
    if (v.isDouble() || v.isInt() || v.isInt64())
    {
        out = static_cast<double>(v);
        return true;
    }

    return false;
}

bool getJsonBool(const juce::var& obj, const char* key, bool& out)
{
    if (!obj.hasProperty(key))
        return false;

    const auto& v = obj[key];
    if (v.isBool())
    {
        out = static_cast<bool>(v);
        return true;
    }

    return false;
}

bool getJsonString(const juce::var& obj, const char* key, juce::String& out)
{
    if (!obj.hasProperty(key))
        return false;

    const auto& v = obj[key];
    if (v.isString())
    {
        out = v.toString();
        return true;
    }

    return false;
}

KznImportResult makeReadError(kzn::KznError err)
{
    return { false, "Failed to read .kzn file (error " + juce::String(static_cast<int>(err)) + ")" };
}

KznImportResult validateSignatureOrBuildWarning(const kzn::KznFileData& fileData, juce::String& warningOut)
{
    const auto sigReport = kzn::verifySignature(fileData);

    if (sigReport.status == kzn::KznSignatureStatus::InvalidSignature
        || sigReport.status == kzn::KznSignatureStatus::Malformed)
    {
        return { false, "This preset file has been modified and cannot be opened.\n\n"
                        "Kaizen DSP files are cryptographically signed. Any modification - "
                        "including whitespace, encoding, or line ending changes - will "
                        "invalidate the signature.\n\n"
                        "To recover: request a fresh copy from the original creator." };
    }

    if (sigReport.status == kzn::KznSignatureStatus::UnknownKeyId
        || sigReport.status == kzn::KznSignatureStatus::UnsupportedVersion
        || sigReport.status == kzn::KznSignatureStatus::FormatError)
    {
        juce::String reason = sigReport.reason.empty() ? "Unsupported signature metadata." : juce::String(sigReport.reason);
        return { false, "This .kzn file could not be verified.\n\n" + reason };
    }

    if (sigReport.status == kzn::KznSignatureStatus::Unsigned)
    {
        warningOut = "This .kzn file is unsigned. Import succeeded, but the file could not be authenticated.";
    }

    return { true };
}

void applyDisplayPresetValues(ChoroborosAudioProcessor& processor, const juce::var& engineVar)
{
    double rateDisplay = 0.65;
    double depthDisplay = 50.0;
    double offsetDisplay = 90.0;
    double widthDisplay = 100.0;
    double colorDisplay = 50.0;
    double mixDisplay = 50.0;
    bool hq = false;

    getJsonDouble(engineVar, "rate", rateDisplay);
    getJsonDouble(engineVar, "depth", depthDisplay);
    getJsonDouble(engineVar, "offset", offsetDisplay);
    getJsonDouble(engineVar, "width", widthDisplay);
    getJsonDouble(engineVar, "color", colorDisplay);
    getJsonDouble(engineVar, "mix", mixDisplay);
    getJsonBool(engineVar, "hq", hq);

    const float rateMapped = static_cast<float>(rateDisplay);
    const float depthMapped = static_cast<float>(depthDisplay / 100.0);
    const float offsetMapped = static_cast<float>(offsetDisplay);
    const float widthMapped = static_cast<float>(widthDisplay / 100.0);
    const float colorMapped = static_cast<float>(colorDisplay / 100.0);
    const float mixMapped = static_cast<float>(mixDisplay / 100.0);

    auto& params = processor.getValueTreeState();

    const auto setMappedParam = [&](const juce::String& paramId, float mappedValue)
    {
        if (auto* param = params.getParameter(paramId))
        {
            const float rawValue = processor.unmapParameterValue(paramId, mappedValue);
            float normalizedValue = param->convertTo0to1(rawValue);
            normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);
            param->setValueNotifyingHost(normalizedValue);
        }
    };

    if (auto* param = params.getParameter(ChoroborosAudioProcessor::HQ_ID))
        param->setValueNotifyingHost(hq ? 1.0f : 0.0f);

    setMappedParam(ChoroborosAudioProcessor::RATE_ID, rateMapped);
    setMappedParam(ChoroborosAudioProcessor::DEPTH_ID, depthMapped);
    setMappedParam(ChoroborosAudioProcessor::OFFSET_ID, offsetMapped);
    setMappedParam(ChoroborosAudioProcessor::WIDTH_ID, widthMapped);
    setMappedParam(ChoroborosAudioProcessor::MIX_ID, mixMapped);
    setMappedParam(ChoroborosAudioProcessor::COLOR_ID, colorMapped);
}

void readTuningFromVar(const juce::var& src, ChorusDSP::RuntimeTuning& t)
{
#define KZN_READ_TUNING(field) if ((src).hasProperty(#field)) (t).field.store(static_cast<float>(static_cast<double>((src)[#field])))
    if (!src.isObject())
        return;

    KZN_READ_TUNING(rateSmoothingMs); KZN_READ_TUNING(depthSmoothingMs);
    KZN_READ_TUNING(depthRateLimit); KZN_READ_TUNING(centreDelaySmoothingMs);
    KZN_READ_TUNING(colorSmoothingMs); KZN_READ_TUNING(widthSmoothingMs);
    KZN_READ_TUNING(centreDelayBaseMs); KZN_READ_TUNING(centreDelayScale);
    KZN_READ_TUNING(hpfCutoffHz); KZN_READ_TUNING(hpfQ);
    KZN_READ_TUNING(lpfCutoffHz); KZN_READ_TUNING(lpfQ);
    KZN_READ_TUNING(preEmphasisFreqHz); KZN_READ_TUNING(preEmphasisQ);
    KZN_READ_TUNING(preEmphasisGain); KZN_READ_TUNING(preEmphasisLevelSmoothing);
    KZN_READ_TUNING(preEmphasisQuietThreshold); KZN_READ_TUNING(preEmphasisMaxAmount);
    KZN_READ_TUNING(compressorAttackMs); KZN_READ_TUNING(compressorReleaseMs);
    KZN_READ_TUNING(compressorThresholdDb); KZN_READ_TUNING(compressorRatio);
    KZN_READ_TUNING(saturationDriveScale);
    KZN_READ_TUNING(greenBloomExponent); KZN_READ_TUNING(greenBloomDepthScale);
    KZN_READ_TUNING(greenBloomCentreOffsetMs); KZN_READ_TUNING(greenBloomCutoffMaxHz);
    KZN_READ_TUNING(greenBloomCutoffMinHz); KZN_READ_TUNING(greenBloomWetBlend);
    KZN_READ_TUNING(greenBloomGain);
    KZN_READ_TUNING(blueFocusExponent); KZN_READ_TUNING(blueFocusHpMinHz);
    KZN_READ_TUNING(blueFocusHpMaxHz); KZN_READ_TUNING(blueFocusLpMaxHz);
    KZN_READ_TUNING(blueFocusLpMinHz); KZN_READ_TUNING(bluePresenceFreqMinHz);
    KZN_READ_TUNING(bluePresenceFreqMaxHz); KZN_READ_TUNING(bluePresenceQMin);
    KZN_READ_TUNING(bluePresenceQMax); KZN_READ_TUNING(bluePresenceGainMaxDb);
    KZN_READ_TUNING(blueFocusWetBlend); KZN_READ_TUNING(blueFocusOutputGain);
    KZN_READ_TUNING(purpleWarpA); KZN_READ_TUNING(purpleWarpB);
    KZN_READ_TUNING(purpleWarpKBase); KZN_READ_TUNING(purpleWarpKScale);
    KZN_READ_TUNING(purpleWarpDelaySmoothingMs);
    KZN_READ_TUNING(purpleOrbitEccentricity); KZN_READ_TUNING(purpleOrbitThetaRateBaseHz);
    KZN_READ_TUNING(purpleOrbitThetaRateScaleHz); KZN_READ_TUNING(purpleOrbitThetaRate2Ratio);
    KZN_READ_TUNING(purpleOrbitEccentricity2Ratio); KZN_READ_TUNING(purpleOrbitMix1);
    KZN_READ_TUNING(purpleOrbitStereoThetaOffset); KZN_READ_TUNING(purpleOrbitDelaySmoothingMs);
    KZN_READ_TUNING(blackNqDepthBase); KZN_READ_TUNING(blackNqDepthScale);
    KZN_READ_TUNING(blackNqDelayGlideMs);
    KZN_READ_TUNING(blackHqTap2MixBase); KZN_READ_TUNING(blackHqTap2MixScale);
    KZN_READ_TUNING(blackHqSecondTapDepthBase); KZN_READ_TUNING(blackHqSecondTapDepthScale);
    KZN_READ_TUNING(blackHqSecondTapDelayOffsetBase); KZN_READ_TUNING(blackHqSecondTapDelayOffsetScale);
    KZN_READ_TUNING(bbdDelaySmoothingMs); KZN_READ_TUNING(bbdDelayMinMs);
    KZN_READ_TUNING(bbdDelayMaxMs); KZN_READ_TUNING(bbdCentreBaseMs);
    KZN_READ_TUNING(bbdCentreScale); KZN_READ_TUNING(bbdDepthMs);
    KZN_READ_TUNING(bbdClockSmoothingMs); KZN_READ_TUNING(bbdFilterSmoothingMs);
    KZN_READ_TUNING(bbdFilterCutoffMinHz); KZN_READ_TUNING(bbdFilterCutoffMaxHz);
    KZN_READ_TUNING(bbdFilterCutoffScale); KZN_READ_TUNING(bbdClockMinHz);
    KZN_READ_TUNING(bbdClockMaxRatio); KZN_READ_TUNING(bbdStages);
    KZN_READ_TUNING(bbdFilterMaxRatio);
    KZN_READ_TUNING(tapeDelaySmoothingMs); KZN_READ_TUNING(tapeCentreBaseMs);
    KZN_READ_TUNING(tapeCentreScale); KZN_READ_TUNING(tapeToneMaxHz);
    KZN_READ_TUNING(tapeToneMinHz); KZN_READ_TUNING(tapeToneSmoothingCoeff);
    KZN_READ_TUNING(tapeDriveScale); KZN_READ_TUNING(tapeLfoRatioScale);
    KZN_READ_TUNING(tapeLfoModSmoothingCoeff); KZN_READ_TUNING(tapeRatioSmoothingCoeff);
    KZN_READ_TUNING(tapePhaseDamping); KZN_READ_TUNING(tapeWowFreqBase);
    KZN_READ_TUNING(tapeWowFreqSpread); KZN_READ_TUNING(tapeFlutterFreqBase);
    KZN_READ_TUNING(tapeFlutterFreqSpread); KZN_READ_TUNING(tapeWowDepthBase);
    KZN_READ_TUNING(tapeWowDepthSpread); KZN_READ_TUNING(tapeFlutterDepthBase);
    KZN_READ_TUNING(tapeFlutterDepthSpread); KZN_READ_TUNING(tapeRatioMin);
    KZN_READ_TUNING(tapeRatioMax); KZN_READ_TUNING(tapeWetGain);
    KZN_READ_TUNING(tapeHermiteTension);
#undef KZN_READ_TUNING
}

} // namespace

KznImportResult importPresetKzn(ChoroborosAudioProcessor& processor, const juce::File& inputFile)
{
    kzn::KznFileData fileData;
    const auto err = kzn::readKznFile(inputFile.getFullPathName().toStdString(), fileData);
    if (err != kzn::KznError::OK)
        return makeReadError(err);

    juce::String warning;
    auto signatureResult = validateSignatureOrBuildWarning(fileData, warning);
    if (!signatureResult.success)
        return signatureResult;

    const auto jsonVar = juce::JSON::parse(juce::String::fromUTF8(fileData.jsonPayload.data(),
                                                                  static_cast<int>(fileData.jsonPayload.size())));
    if (!jsonVar.isObject())
        return { false, "Invalid JSON payload" };

    juce::String type;
    getJsonString(jsonVar, "type", type);
    if (type != kzn_schema::TYPE_PRESET)
        return { false, "Not a preset .kzn file (type: " + type + ")" };

    const auto engineVar = jsonVar["engine"];
    if (!engineVar.isObject())
        return { false, "Missing engine object in payload" };

    juce::String presetName = "Imported Preset";
    const auto presetVar = jsonVar["preset"];
    if (presetVar.isObject())
        getJsonString(presetVar, "name", presetName);

    juce::String engineId;
    if (!getJsonString(engineVar, "id", engineId))
        return { false, "Missing engine id" };

    const int engineIndex = parseEngineColorToken(engineId.toStdString());
    if (engineIndex < 0)
        return { false, "Unknown engine id: " + engineId };

    if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
    {
        param->setValueNotifyingHost(
            processor.getValueTreeState().getParameterRange(ChoroborosAudioProcessor::ENGINE_COLOR_ID)
                .convertTo0to1(static_cast<float>(engineIndex)));
    }

    applyDisplayPresetValues(processor, engineVar);
    return { true, {}, warning, presetName, kzn_schema::TYPE_PRESET };
}

KznImportResult importEngineKzn(ChoroborosAudioProcessor& processor, const juce::File& inputFile)
{
    kzn::KznFileData fileData;
    const auto err = kzn::readKznFile(inputFile.getFullPathName().toStdString(), fileData);
    if (err != kzn::KznError::OK)
        return makeReadError(err);

    juce::String warning;
    auto signatureResult = validateSignatureOrBuildWarning(fileData, warning);
    if (!signatureResult.success)
        return signatureResult;

    const auto jsonVar = juce::JSON::parse(juce::String::fromUTF8(fileData.jsonPayload.data(),
                                                                  static_cast<int>(fileData.jsonPayload.size())));
    if (!jsonVar.isObject())
        return { false, "Invalid JSON payload" };

    juce::String type;
    getJsonString(jsonVar, "type", type);
    if (type != kzn_schema::TYPE_ENGINE)
        return { false, "Not an engine .kzn file (type: " + type + ")" };

    const auto engineDefVar = jsonVar["engine_def"];
    if (!engineDefVar.isObject())
        return { false, "Missing engine_def in payload" };

    if (!processor.customEngineManager)
        return { false, "Custom engine manager not available" };

    juce::String engineName;
    getJsonString(engineDefVar, "name", engineName);
    if (engineName.isEmpty())
        engineName = "Imported Engine";

    juce::String nqCoreToken;
    juce::String hqCoreToken;
    getJsonString(engineDefVar, "nq_core", nqCoreToken);
    getJsonString(engineDefVar, "hq_core", hqCoreToken);

    CoreId nqCore = CoreId::lagrange3;
    CoreId hqCore = CoreId::lagrange5;
    const bool parsedNq = parseCoreIdToken(nqCoreToken.toStdString(), nqCore);
    const bool parsedHq = parseCoreIdToken(hqCoreToken.toStdString(), hqCore);

    if (!parsedNq || !parsedHq)
    {
        return { false,
            "This engine uses premium DSP cores that require Choroboros Commercial.\n\n"
            "Visit choroboros.kaizenstrategic.ai to upgrade." };
    }

    if (choroboros::isCommercialCore(nqCore) || choroboros::isCommercialCore(hqCore))
    {
        return { false,
            "This engine uses premium DSP cores that require Choroboros Commercial.\n\n"
            "Visit choroboros.kaizenstrategic.ai to upgrade." };
    }

    double knobTheme = 0.0;
    double accentTheme = 0.0;
    getJsonDouble(engineDefVar, "knob_theme", knobTheme);
    getJsonDouble(engineDefVar, "accent_theme", accentTheme);

    // Free tier: replace existing custom engine if one exists (single slot).
    juce::Uuid id;
    auto* existing = processor.customEngineManager->getSingleCustomEngine();
    if (existing != nullptr)
    {
        if (processor.hasActiveCustomEngine() && processor.getActiveCustomEngineId() == existing->id)
            processor.deactivateCustomEngine();

        id = existing->id;
        existing->name = engineName;
    }
    else
    {
        id = processor.customEngineManager->createEngine(engineName);
    }

    auto* engine = processor.customEngineManager->getEngineById(id);
    if (engine == nullptr)
        return { false, "Failed to create engine" };

    engine->nqCore = nqCore;
    engine->hqCore = hqCore;
    engine->knobTheme = juce::jlimit(0, 4, static_cast<int>(knobTheme));
    engine->accentTheme = juce::jlimit(0, 4, static_cast<int>(accentTheme));

    readTuningFromVar(engineDefVar["nq_tuning"], *engine->nqTuning);
    readTuningFromVar(engineDefVar["hq_tuning"], *engine->hqTuning);

    // Open-source: force white visual identity for all custom engines.
    engine->visual = getWhiteVisual();

    processor.customEngineManager->saveEngine(id);
    processor.activateCustomEngine(id);

    const auto defaultPresetVar = jsonVar["default_preset"];
    if (defaultPresetVar.isObject())
        applyDisplayPresetValues(processor, defaultPresetVar);

    return { true, {}, warning, engineName, kzn_schema::TYPE_ENGINE };
}

KznImportResult importKzn(ChoroborosAudioProcessor& processor, const juce::File& inputFile)
{
    kzn::KznFileData fileData;
    const auto err = kzn::readKznFile(inputFile.getFullPathName().toStdString(), fileData);
    if (err != kzn::KznError::OK)
        return makeReadError(err);

    const auto jsonVar = juce::JSON::parse(juce::String::fromUTF8(fileData.jsonPayload.data(),
                                                                  static_cast<int>(fileData.jsonPayload.size())));
    juce::String type;
    if (jsonVar.isObject())
        getJsonString(jsonVar, "type", type);

    if (type == kzn_schema::TYPE_ENGINE)
        return importEngineKzn(processor, inputFile);

    return importPresetKzn(processor, inputFile);
}

} // namespace choroboros
