// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// PkiPublicApi (06-pki.md §1): the capability layer of the Veyon PKI that every component may use.
// It parses certificates (Veyon::Crypto::Certificate, SPKI pins via spkiSha256()/spkiFingerprint()),
// verifies TLS peers (TlsPeerVerifier, 05-tls.md §5) and parses/creates trust bundles
// (TrustBundle, 06-pki.md §4). It never touches private keys and has no CA authority: it lives in
// veyon-core, which must not link veyon-crypto-authority (configure-time capability boundary check,
// top-level CMakeLists.txt).

#include "Certificate.h"
#include "TlsClientPolicy.h"
#include "TlsConfiguration.h"
#include "TlsError.h"
#include "TlsPeerVerifier.h"
#include "TrustBundle.h"
