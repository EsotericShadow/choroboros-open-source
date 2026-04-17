#pragma once

#ifndef CHOROBOROS_ASSET_PACK_VERSION
#define CHOROBOROS_ASSET_PACK_VERSION "2.0.50"
#endif

#ifndef CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK
#define CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK 1
#endif

#ifndef CHOROBOROS_USE_EXTERNAL_ASSET_PACK
#define CHOROBOROS_USE_EXTERNAL_ASSET_PACK 1
#endif

namespace choroboros::assets
{
inline constexpr int kManifestSchemaVersion = 1;
inline constexpr const char* kPackName = "Choroboros Assets";
inline constexpr const char* kExpectedPackVersion = CHOROBOROS_ASSET_PACK_VERSION;
inline constexpr bool kAllowEmbeddedAssetFallback = CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK != 0;
inline constexpr bool kUseExternalAssetPack = CHOROBOROS_USE_EXTERNAL_ASSET_PACK != 0;
inline constexpr const char* kAssetPackOverrideEnvVar = "CHOROBOROS_ASSET_PACK_DIR";
}
