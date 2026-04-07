================================================================================
.kzn FILE FORMAT SPECIFICATION
Kaizen DSP — v1.0 Draft (Revised)
================================================================================

OVERVIEW
--------
.kzn is the proprietary signed preset and engine format for all Kaizen DSP
plugins. A .kzn file encodes either a single preset configuration OR a custom
engine definition for a specific Kaizen DSP plugin.

It is a signed, tamper-evident binary+JSON hybrid. Files are created by a
licensed user via explicit export inside the plugin. Internal working presets
use a separate format. .kzn files are for finished, shareable, attributable
artifacts only.

Two content types:
  - Preset .kzn  — parameter snapshot for an existing engine
  - Engine .kzn  — full custom engine (cores + tuning + skin + default preset)

There is no executable code in a .kzn file. Ever.

================================================================================
FILE EXTENSION
================================================================================
.kzn
Examples: warmpad.kzn, warm-tape-engine.kzn

================================================================================
BINARY HEADER (128 bytes, v1)
================================================================================
All multi-byte integers are little-endian.

Offset  Size   Type      Field              Value (v1)
------  ----   ----      -----              ----------
0x00    4      char[4]   magic              "KZN1" (0x4B 0x5A 0x4E 0x31)
0x04    2      u16       header_size        128
0x06    2      u16       format_version     1
0x08    4      u32       flags              Bitfield (see below)
0x0C    1      u8        sig_scheme         0=unsigned, 1=server, 2=session-cert
0x0D    1      u8        c14n_scheme        0=raw-bytes, 1=JCS (RFC 8785)
0x0E    1      u8        payload_type       1=UTF-8 JSON
0x0F    1      u8        reserved0          0
0x10    4      u32       key_id             Identifies verifying public key
0x14    4      u32       payload_offset     128 (= header_size)
0x18    4      u32       payload_length     Byte length of JSON payload
0x1C    4      u32       signature_offset   64 (offset within header)
0x20    4      u32       signature_length   64 (Ed25519)
0x24    4      u32       footer_length      0 (no footer in v1)
0x28    24     u8[24]    reserved1          All zeros (future use)
0x40    64     u8[64]    signature          Ed25519 signature or 64 zeros

Total header: 128 bytes. Payload begins at byte 128.

================================================================================
FLAGS BITFIELD
================================================================================
bit 0: HAS_SIGNATURE       signature field is not all zeros
bit 1: EXPECT_SESSION_CERT  JSON contains sessionCert object
bit 2: PAYLOAD_IS_CANONICAL payload meets c14n_scheme constraints
bit 3: STRICT_IMPORT        reject unknown JSON fields
bits 4-31: reserved, must be 0

================================================================================
MAGIC BYTES
================================================================================
0x4B 0x5A 0x4E 0x31  =  ASCII "KZN1"

The "1" encodes the major format version in the magic itself, providing a
quick visual identifier when hex-dumping files.

================================================================================
SIGNING INPUT DEFINITION
================================================================================
CRITICAL: This is the frozen signing contract.

  signing_message = header_bytes_with_signature_zeroed || payload_bytes

The signature covers BOTH the header and payload. This binds:
  - format_version
  - key_id
  - sig_scheme
  - flags
  - payload_length
  - the JSON payload itself

To construct the signing message:
  1. Take the 128-byte header
  2. Zero bytes 64-127 (the signature field)
  3. Append the payload bytes
  4. The result is the signing message

The Worker signs these exact bytes. The verifier reconstructs these exact
bytes. No re-serialization of JSON occurs during verification.

================================================================================
CANONICAL JSON POLICY (v1)
================================================================================
c14n_scheme = 0 (raw bytes)

The plugin serializes JSON to deterministic bytes. Those exact bytes are
signed. Those exact bytes are verified. No canonicalization on the server side.

Constraints enforced at serialization time:
  - UTF-8 encoding, no BOM
  - No whitespace between tokens
  - All object keys are ASCII-only
  - Keys sorted lexicographically within each object (ASCII byte order)
  - Numbers: finite only, no NaN/Infinity, no unnecessary trailing zeros
  - Omit optional fields when absent (no explicit null unless schema requires)
  - No duplicate keys
  - Arrays preserve insertion order

These constraints are a subset of RFC 8785 (JCS). Upgrading to full JCS
(c14n_scheme=1) in a future version is non-breaking.

NOTE: juce::JSON::toString() does NOT guarantee this output. A small
schema-constrained canonical emitter must be implemented in libkzn.

================================================================================
ALGORITHM
================================================================================
Ed25519 (RFC 8032)
  - 32-byte public key embedded in plugin binary
  - 64-byte signature stored in header at offset 0x40
  - Private key never leaves the signing server
  - Monocypher crypto_ed25519_check() for verification
  - @noble/ed25519 with zip215:false for signing (strict RFC 8032)

================================================================================
JSON PAYLOAD SCHEMA — PRESET TYPE
================================================================================
{
  "schema_version": "1.0",
  "type": "preset",
  "key_id": 1,
  "file_id": "uuid-v4",
  "creator_id": "opaque-uuid-v4",
  "created_at": "ISO-8601 UTC",
  "plugin": {
    "id": "choroboros",
    "version": "1.0"
  },
  "generator": "Choroboros 1.0.0",
  "preset": {
    "name": "Warm Pad",
    "tags": ["pad","ambient"],
    "description": ""
  },
  "engine": {
    "id": "green",
    "rate": 1.2,
    "depth": 21.0,
    "offset": 33.0,
    "width": 150.0,
    "color": 16.0,
    "mix": 50.0,
    "hq": false
  },
  "assets": {}
}

================================================================================
JSON PAYLOAD SCHEMA — ENGINE TYPE
================================================================================
{
  "schema_version": "1.0",
  "type": "engine",
  "key_id": 1,
  "file_id": "uuid-v4",
  "creator_id": "opaque-uuid-v4",
  "created_at": "ISO-8601 UTC",
  "plugin": {
    "id": "choroboros",
    "version": "1.0"
  },
  "generator": "Choroboros 1.0.0",
  "engine_def": {
    "name": "Warm Tape",
    "nq_core": "bbd",
    "hq_core": "tape",
    "knob_theme": "red",
    "accent_theme": "purple",
    "nq_tuning": {},
    "hq_tuning": {}
  },
  "default_preset": {
    "name": "Warm Tape Init",
    "rate": 1.2,
    "depth": 21.0,
    "offset": 33.0,
    "width": 150.0,
    "color": 16.0,
    "mix": 50.0,
    "hq": false
  },
  "assets": {}
}

================================================================================
SCHEMA VERSIONING RULES
================================================================================
  - schema_version MAJOR bump = breaking change, plugin refuses if unsupported
  - schema_version MINOR bump = additive change, load and ignore unknown keys
  - Unknown keys at any level must be silently ignored (forward compatibility)
  - Missing optional keys use plugin defaults
  - Missing required keys = reject with error

================================================================================
PARAMETER VALIDATION
================================================================================
All numeric engine parameters must be bounds-checked after parsing.
Clamp to range and log warning. Do not crash. Do not reject the file
for an out-of-range value alone — the signature guarantees integrity.

Valid ranges for Choroboros v1.0 (display space):
  rate:    0.01 – 10.0   (Hz)
  depth:   0.0  – 100.0  (%)
  offset:  0.0  – 180.0  (degrees)
  width:   0.0  – 200.0  (%)
  color:   0.0  – 100.0  (%)
  mix:     0.0  – 100.0  (%)
  hq:      true | false

================================================================================
VERIFICATION ORDER
================================================================================
  1. Read 128-byte header
  2. Validate magic: "KZN1"
  3. Validate format_version (reject if major > supported)
  4. Validate bounds: payload_offset + payload_length <= file_size
  5. Validate payload_length <= 64KB max
  6. Validate signature_length == 64 (if sig_scheme != 0)
  7. Read payload bytes
  8. If sig_scheme == 0 or signature all zeros: status = Unsigned
  9. If sig_scheme == 1 (server):
       a. Look up public key by key_id
       b. Build signing_message = header(sig zeroed) || payload
       c. crypto_ed25519_check(signature, pubkey, signing_message, len)
       d. If returns -1: REJECT — tamper error
  10. If sig_scheme == 2 (session cert):
       a. Parse sessionCert from JSON
       b. Verify server signature over ephemeral pubkey
       c. Verify payload signature with ephemeral pubkey
  11. Parse JSON payload
  12. Check plugin.id matches — reject if mismatch
  13. Check type field — route to preset or engine import
  14. Validate and clamp parameters
  15. Load

NEVER parse JSON before verifying the signature.

================================================================================
TAMPER ERROR DIALOG
================================================================================
"This preset file has been modified and cannot be opened.

Kaizen DSP files are cryptographically signed. Any modification —
including whitespace, encoding, or line ending changes — will
invalidate the signature and the file will be refused by the plugin.

To recover: request a fresh copy from the original creator,
or re-export from within the plugin."

Do NOT use the word "piracy." Most failures are accidental.
Do NOT phone home on tamper detection. Log locally only.

================================================================================
CREATOR ATTRIBUTION AND PRIVACY
================================================================================
creator_id is a server-assigned opaque UUID v4.
It is NOT a hash of the user's email.
It is NOT any reversible identifier.
It maps to a user only via the server-side database.

GDPR classification: pseudonymous personal data (Art. 4(5))
  - Pseudonymised data remains personal data under GDPR
  - Data minimisation, purpose limitation, retention controls required
  - Server logs: hash license ID, don't log raw
  - Don't log preset descriptions/tags unless needed
  - Define 30-90 day retention windows
  - Right to erasure: delete server-side mapping, UUID becomes anonymous

================================================================================
OFFLINE SIGNING (SESSION CERTIFICATES) — DEFERRED TO MILESTONE B
================================================================================
Not implemented in v1 initial release. Server signing ships first.
Session certificates are the highest-complexity feature and should be
proven after server signing is stable.

================================================================================
FUTURE EXTENSIONS (reserved, not in v1)
================================================================================
  - assets block: manifest of sidecar asset files with SHA-256 hashes
  - engine builder output with full RuntimeTuning in engine_def
  - cross-plugin references
  - compression (c14n_scheme=1 + zlib)
  - JCS canonicalization (c14n_scheme=1)
  - revocation list

================================================================================
SPEC DOCUMENT URL
================================================================================
https://kaizenstrategic.ai/kzn-format
Must be live before commercial release.

================================================================================
END OF SPEC
================================================================================
