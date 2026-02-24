# License and Contact Information Update Summary

## Changes Made

### 1. Contact Information Updated
Changed all references from "support" to "info":

- **AboutDialog.cpp**: Changed "Support:" label to "Info:" and email from `support@kaizenstrategicai.com` to `info@kaizenstrategic.ai`
- **FeedbackDialog.cpp**: Updated email link to use `info@kaizenstrategic.ai`
- **PluginEditor.cpp**: Updated Help button email link to use `info@kaizenstrategic.ai`

### 2. End User License Agreement (EULA) Created
Created comprehensive EULA document (`EULA.md`) that includes:

#### Key Sections:
- **Grant of License**: Defines what users can and cannot do
- **Proprietary Rights**: 
  - Ownership by Kaizen Strategic AI Inc.
  - **Special Protection for Purple Engine Algorithms**: 
    - Phase-Warped Chorus
    - Orbit Chorus
    - Explicitly prohibited from reverse engineering, extraction, copying, or unauthorized use
    - Protected as trade secrets
- **Third-Party Components**: 
  - JUCE framework license acknowledgment
  - Link to JUCE license terms
- **Code and Assets Credit**: 
  - Gabriel Lacroix Marko credited as founder and creator
  - All code, GUI, assets, and sound design credited
- **Standard EULA Terms**:
  - Restrictions
  - Termination
  - No warranties
  - Limitation of liability
  - Export restrictions
  - U.S. Government rights
  - Governing law (British Columbia, Canada)
  - Contact information

#### Proprietary Algorithm Protection
The EULA specifically protects the two Purple engine algorithms:
- **Phase-Warped Chorus** (non-uniform phase modulation)
- **Orbit Chorus** (2D rotating modulation)

These are explicitly marked as:
- Proprietary intellectual property
- Protected by trade secret law
- Subject to potential patent applications
- Cannot be reverse engineered, extracted, copied, or used without explicit written license

### 3. About Dialog Enhanced
Added "View License" button to About dialog that:
- Attempts to open EULA.md file from plugin bundle/resources
- Falls back to showing key license terms in a dialog if file not found
- Provides contact information for license questions

## Files Modified

1. `Source/Plugin/AboutDialog.h` - Added license button and method
2. `Source/Plugin/AboutDialog.cpp` - Updated contact info, added license button functionality
3. `Source/Plugin/FeedbackDialog.cpp` - Updated email address
4. `Source/Plugin/PluginEditor.cpp` - Updated help button email address
5. `EULA.md` - **NEW FILE** - Complete End User License Agreement

## Company Information in EULA

- **Company Name**: Kaizen Strategic AI Inc.
- **DBA**: Green DSP
- **Location**: British Columbia, Canada
- **Contact**: info@kaizenstrategic.ai
- **Copyright**: © 2026 Kaizen Strategic AI Inc.
- **Creator**: Gabriel Lacroix Marko (founder)

## Next Steps

1. **Bundle EULA with Plugin**: 
   - Copy `EULA.md` to plugin bundle Resources folder during build
   - Ensure it's accessible when "View License" button is clicked

2. **Legal Review**: 
   - Have the EULA reviewed by a qualified attorney familiar with:
     - Canadian software licensing law
     - Intellectual property protection
     - Trade secret protection
     - International software distribution

3. **Distribution**: 
   - Include EULA.md in plugin installer
   - Make EULA accessible from About dialog
   - Consider requiring EULA acceptance during installation (if applicable)

4. **Documentation**: 
   - Update README.md to reference EULA
   - Include EULA link in distribution materials
   - Consider adding EULA acceptance checkbox in installer (if applicable)

## Important Notes

- The EULA protects proprietary algorithms through trade secret law
- Consider filing patent applications for the Purple engine algorithms if not already done
- The EULA is governed by British Columbia, Canada law
- JUCE framework has its own license terms that users must comply with
- All code, GUI, assets, and sound design are credited to Gabriel Lacroix Marko

---

**Status**: ✅ Complete
**Date**: January 2026
