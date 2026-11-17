// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include "LegacyRsaPrivateKey.h"

// Legacy RSA of Veyon 4.x (06-pki.md §2, WP11g): byte-compatible replacements for the QCA-based
// RSA code of Veyon <= 4.11 (QCA 2.x with the OpenSSL plugin), verified with known-answer tests
// from QCA output (tests/unit/core/data/legacyrsa). New code must not use these schemes.
//
// - Keys (LegacyRsaPublicKey, LegacyRsaPrivateKey): 4.x generates RSA-4096 keys with exponent
//   65537 (key files and the per-connection logon authentication key) and writes them as
//   unencrypted PKCS#8 "PRIVATE KEY" and SubjectPublicKeyInfo "PUBLIC KEY" PEM (64-character
//   lines, LF). 4.x also reads PKCS#1 "RSA PRIVATE KEY" and "RSA PUBLIC KEY" PEM; both are read
//   here as well, but never written.
// - Key file authentication: RSASSA-PKCS1-v1_5 with SHA-512 (QCA EMSA3_SHA512) over the
//   server's 128-byte challenge.
// - Logon authentication: the client encrypts the password with RSAES-OAEP (SHA-1, MGF1-SHA1,
//   empty label; QCA EME_PKCS1_OAEP) to the server's per-connection public key.
// - Password obfuscation in the configuration: the same RSAES-OAEP with the public key of the
//   built-in key core/resources/default-pkey.pem, encoded as lower-case hex (encryptPasswordHex(),
//   decryptPasswordHex()). The key is public, so this is obfuscation, not protection.
//
// Deliberate differences to 4.x (fail closed instead of silent failures): keys outside
// LegacyRsaPublicKey::MinimumBits..MaximumBits, even moduli, even or out-of-range public exponents,
// multi-prime or inconsistent private keys, non-DER key encodings, encrypted key files, files with
// more than one key block, plaintexts longer than the OAEP limit (4.x silently truncated them to
// the limit), signatures and ciphertexts not of exactly the modulus size and non-hex password
// strings (4.x skipped invalid characters) are rejected. Successful operations clear lastError().

namespace Veyon::Crypto::LegacyRsa {

// lower-case hex of encryptOaepSha1(password), as the password values of the 4.x configuration
[[nodiscard]] VEYON_CRYPTO_EXPORT std::optional<QString> encryptPasswordHex(const LegacyRsaPublicKey& key, ByteView password);

// inverse of encryptPasswordHex(): exactly 2 * modulus size hex digits (either case, nothing
// else); an empty string is an error (no password set must be handled by the caller)
[[nodiscard]] VEYON_CRYPTO_EXPORT std::optional<SecureBuffer> decryptPasswordHex(const LegacyRsaPrivateKey& key,
																				 const QString& encryptedPassword);

} // namespace Veyon::Crypto::LegacyRsa
