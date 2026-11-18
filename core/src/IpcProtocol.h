// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QLatin1String>
#include <QtGlobal>

// Wire constants of the local IPC protocol (docs/specs/02-ipc.md §2–§6)
namespace IpcProtocol {

// §4 handshake
inline constexpr char Magic[] = "VIPC";
inline constexpr qsizetype MagicSize = 4;
inline constexpr quint8 Version = 0x01;

enum class Role : quint8 {
	SessionServer = 1,
	Worker = 2,
};

inline constexpr char ServerProofLabel[] = "veyon-ipc-v1 server";
inline constexpr char ClientProofLabel[] = "veyon-ipc-v1 client";
inline constexpr char KeyLabel[] = "veyon-ipc-v1 key";
inline constexpr char ClientToServerKeyInfo[] = "c2s";
inline constexpr char ServerToClientKeyInfo[] = "s2c";

inline constexpr qsizetype TokenSize = 32;
inline constexpr qsizetype NonceSize = 32;
inline constexpr qsizetype ProofSize = 32;
inline constexpr qsizetype KeySize = 32;

// "VIPC" version role sessionId(u32be) nonceC
inline constexpr qsizetype HelloSize = MagicSize + 1 + 1 + 4 + NonceSize;
// nonceS proofS
inline constexpr qsizetype ServerChallengeSize = NonceSize + ProofSize;
inline constexpr quint8 HandshakeConfirmation = 0x01;
inline constexpr qsizetype HandshakeConfirmationSize = 1;

// the server rejects a hello whose nonceC is among the last NonceHistorySize ones of the endpoint
inline constexpr int NonceHistorySize = 16;

// §3 framing
inline constexpr qsizetype FrameHeaderSize = 4;
inline constexpr qsizetype MaximumPreAuthFrameSize = 256;
inline constexpr qsizetype MaximumFrameSize = 65536;
// the worker channel carries feature messages, e.g. 256 KiB file transfer chunks
inline constexpr qsizetype MaximumWorkerFrameSize = 1048576;
inline constexpr int HandshakeTimeout = 3000;
inline constexpr int FrameTimeout = 10000;

// §5 records (ChaCha20-Poly1305)
inline constexpr qsizetype RecordNonceSize = 12;
inline constexpr qsizetype RecordTagSize = 16;
inline constexpr qsizetype MaximumRecordPlaintextSize = MaximumFrameSize - RecordTagSize;

// post-auth frame limit (incl. tag) per role
constexpr qsizetype maximumFrameSize(Role role)
{
	return role == Role::Worker ? MaximumWorkerFrameSize : MaximumFrameSize;
}

constexpr qsizetype maximumRecordPlaintextSize(Role role)
{
	return maximumFrameSize(role) - RecordTagSize;
}

// §1 endpoint limits
inline constexpr int MaximumFailedHandshakes = 3;

// §2 token transfer: "VEYONIPC1 <name> <hex64>\n"
inline constexpr char CredentialsLinePrefix[] = "VEYONIPC1";
inline constexpr qsizetype MaximumCredentialsLineSize = 128;
inline constexpr qsizetype EndpointRandomSize = 4; // 8 hex digits
inline constexpr qsizetype MaximumEndpointNameSize = 64;
inline constexpr auto ServerEndpointPrefix = QLatin1String("veyon-ipc-");
inline constexpr auto WorkerEndpointPrefix = QLatin1String("veyon-worker-");

// §6 application messages
inline constexpr int MaximumJsonDepth = 8;
inline constexpr int MaximumInFlightRequests = 16;
inline constexpr int DefaultCallTimeout = 10000;

namespace Command {
inline constexpr auto GetSessionData = QLatin1String("GetSessionData");
inline constexpr auto SetSessionData = QLatin1String("SetSessionData");
inline constexpr auto GetTlsHostKey = QLatin1String("GetTlsHostKey");
inline constexpr auto GetExternalVncServerPassword = QLatin1String("GetExternalVncServerPassword");
inline constexpr auto Authenticate = QLatin1String("Authenticate");
} // namespace Command

namespace Error {
inline constexpr auto UnknownCommandError = QLatin1String("unknown-command");
inline constexpr auto DeniedError = QLatin1String("denied");
inline constexpr auto BadRequestError = QLatin1String("bad-request");
inline constexpr auto RateLimitedError = QLatin1String("rate-limited");
// GetTlsHostKey: the server's memory may be readable by other processes (T-PROC-02)
inline constexpr auto HostKeyWithheldError = QLatin1String("host-key-withheld");
} // namespace Error

namespace Key {
inline constexpr auto Id = QLatin1String("id");
inline constexpr auto Command = QLatin1String("cmd");
inline constexpr auto Arguments = QLatin1String("args");
inline constexpr auto Ok = QLatin1String("ok");
inline constexpr auto Error = QLatin1String("error");
inline constexpr auto Result = QLatin1String("result");
inline constexpr auto Data = QLatin1String("data");
inline constexpr auto TlsKey = QLatin1String("key");
inline constexpr auto CertificateChain = QLatin1String("certificateChain");
inline constexpr auto Password = QLatin1String("password");
inline constexpr auto Username = QLatin1String("username");
inline constexpr auto Authenticated = QLatin1String("authenticated");
} // namespace Key

// §6 session data
inline constexpr int SessionDataLifetime = 120 * 1000;
inline constexpr int MaximumTlsHostKeyRequests = 2;

} // namespace IpcProtocol
