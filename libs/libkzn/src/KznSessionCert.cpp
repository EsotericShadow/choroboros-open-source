#include "kzn/KznSessionCert.h"

namespace kzn {

bool signWithSessionCert(
    const SessionCertificate& /*cert*/,
    const uint8_t* /*signingMessage*/,
    size_t /*signingMessageLen*/,
    uint8_t /*outSignature*/[64])
{
    // Milestone B — not yet implemented
    return false;
}

} // namespace kzn
