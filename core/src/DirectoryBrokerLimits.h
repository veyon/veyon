// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IpcProtocol.h"

// Limits of the directory and user-groups broker (docs/specs/04-directory-broker.md §6)
namespace DirectoryBrokerLimits {

inline constexpr int MaximumRequestSize = 16 * 1024;
inline constexpr int MaximumReplySize = 1024 * 1024;

// replies are carried over the local IPC channel (02-ipc.md §3), whose records are limited to
// 64 KiB for session servers, so a page also has to fit into one IPC record including the envelope
inline constexpr int MaximumIpcEnvelopeSize = 4 * 1024;
inline constexpr int MaximumReplyPageSize = int(IpcProtocol::MaximumRecordPlaintextSize) - MaximumIpcEnvelopeSize;
static_assert(MaximumReplyPageSize <= MaximumReplySize);

inline constexpr int MaximumObjectsPerPage = 500;
inline constexpr int MaximumGroupsPerPage = 2000;

inline constexpr int CursorLifetime = 60 * 1000;
inline constexpr int CursorSize = 16; // random bytes, sent as 32 hex digits

// per session server: its own queries (access control) and the forwarded queries of masters are
// limited separately, so that masters cannot exhaust the budget of the access control queries
inline constexpr int MaximumRequestsPerSecond = 20;
inline constexpr int MaximumOutstandingRequests = 4;
inline constexpr int MaximumForwardedOutstandingRequests = MaximumOutstandingRequests - 1;
inline constexpr int MaximumForwardedRequestsPerSecond = MaximumRequestsPerSecond - 5;

// cursors: 64 and 4 MiB per session server, split into the pools of its own and of forwarded
// queries; the oldest cursor of a pool is evicted when it is full
inline constexpr int MaximumCursorsPerConnection = 64;
inline constexpr int MaximumMemoryPerConnection = 4 * 1024 * 1024;
inline constexpr int MaximumOwnCursors = 16;
inline constexpr int MaximumForwardedCursors = MaximumCursorsPerConnection - MaximumOwnCursors;
inline constexpr int MaximumOwnCursorMemory = 1024 * 1024;
inline constexpr int MaximumForwardedCursorMemory = MaximumMemoryPerConnection - MaximumOwnCursorMemory;

// per master connection (session server side), fractions of the forwarded limits above
inline constexpr int MaximumRequestsPerSecondPerMaster = 10;
inline constexpr int MaximumOutstandingRequestsPerMaster = 2;
inline constexpr int MaximumCursorsPerMaster = 4;

inline constexpr int MaximumStringSize = 256; // UTF-8 bytes per attribute value, user or group name
inline constexpr int MaximumArgumentCount = 8;

inline constexpr int RequestTimeout = 10 * 1000;
// execution time of a forwarded request, which may delay queued own requests (access control)
inline constexpr int ForwardedExecutionBudget = 4 * 1000;

// largest result of one query (e.g. all computers or all groups), also the LDAP size limit; larger
// directories have to be split into locations
inline constexpr int MaximumResultItems = 20000;
inline constexpr int MaximumReplyStringSize = 4096;

// objects returned to a client and therefore addressable by UID in its later requests (LRU per
// session server, separately for its own and for forwarded queries)
inline constexpr int MaximumKnownOwnObjects = 1000;
inline constexpr int MaximumKnownForwardedObjects = MaximumResultItems;

// a backend that could not be created (e.g. missing configuration) is tried again after this time
inline constexpr int BackendRetryInterval = 30 * 1000;

// UI listings (userGroups()) only, never group memberships used by access control
inline constexpr int UserGroupsCacheLifetime = 60 * 1000;

} // namespace DirectoryBrokerLimits
