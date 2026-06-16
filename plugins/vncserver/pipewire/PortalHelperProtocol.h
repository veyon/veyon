/*
 * PortalHelperProtocol.h - IPC protocol between veyon-server and veyon-portal-helper
 *
 * Copyright (c) 2024-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

/**
 * @defgroup PortalHelperProtocol Binary IPC protocol for the portal helper
 *
 * The veyon-server (main process) and veyon-portal-helper communicate over a
 * Unix socketpair(AF_UNIX, SOCK_SEQPACKET) created before the helper is exec'd.
 *
 * SOCK_SEQPACKET preserves record boundaries: each sendmsg(2) produces exactly
 * one recvmsg(2) call, with no partial reads or coalescing.  This makes the
 * framing logic trivial.
 *
 * All messages are padded to a common EnvelopeSize (16 bytes) so that a single
 * fixed-size buffer is sufficient on both ends.  The first byte is always the
 * message-type discriminator.  The PipeWire remote file descriptor is
 * transferred as SCM_RIGHTS ancillary data alongside the H2MStarted body.
 *
 * Message directions:
 *   Main  → Helper : M2H* messages  (start / close / input events)
 *   Helper → Main  : H2M* messages  (started with FD via SCM_RIGHTS / failed)
 *
 * The helper child receives the socket FD number via the command-line flag
 * "--socket-fd=<N>".
 */

#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace PortalHelperProtocol {

/// All protocol messages are padded to this fixed envelope size.
/// Using a uniform size means both sides always call send/recvmsg with the
/// same byte count, which simplifies framing on the SOCK_SEQPACKET socket.
static constexpr size_t EnvelopeSize = 16;

// ---------------------------------------------------------------------------
// Helper → Main (H2M) message types
// ---------------------------------------------------------------------------

enum class H2MType : uint8_t {
    Started = 0, ///< Portal session ready; PipeWire FD sent as SCM_RIGHTS ancillary data
    Failed  = 1, ///< Portal session failed; no additional payload
};

/// Sent by the helper when the XDG portal session starts successfully.
/// The PipeWire remote FD is transferred as SCM_RIGHTS ancillary data in the
/// same sendmsg(2) call.
#pragma pack(push, 1)
struct H2MStarted {
    H2MType  type{H2MType::Started};
    uint32_t nodeId{0}; ///< PipeWire node ID for the screen capture stream
    uint8_t  _pad[11]{};
};
static_assert(sizeof(H2MStarted) == EnvelopeSize, "H2MStarted must be EnvelopeSize bytes");

/// Sent by the helper when any portal step fails.
struct H2MFailed {
    H2MType type{H2MType::Failed};
    uint8_t _pad[15]{};
};
static_assert(sizeof(H2MFailed) == EnvelopeSize, "H2MFailed must be EnvelopeSize bytes");
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Main → Helper (M2H) message types
// ---------------------------------------------------------------------------

enum class M2HType : uint8_t {
    Start         = 0, ///< Begin the portal session flow
    Close         = 1, ///< Close the portal session and exit
    NotifyKey     = 2, ///< Forward a keyboard keysym event
    NotifyPointer = 3, ///< Forward a pointer (mouse) event
};

#pragma pack(push, 1)
/// Begin the portal session flow.
struct M2HStart {
    M2HType type{M2HType::Start};
    uint8_t _pad[15]{};
};
static_assert(sizeof(M2HStart) == EnvelopeSize, "M2HStart must be EnvelopeSize bytes");

/// Close the portal session and ask the helper to exit cleanly.
struct M2HClose {
    M2HType type{M2HType::Close};
    uint8_t _pad[15]{};
};
static_assert(sizeof(M2HClose) == EnvelopeSize, "M2HClose must be EnvelopeSize bytes");

/// Forward a keyboard keysym event from the VNC client to the portal.
struct M2HNotifyKey {
    M2HType  type{M2HType::NotifyKey};
    uint32_t keySym{0};
    uint8_t  down{0}; ///< 1 = key pressed, 0 = key released
    uint8_t  _pad[10]{};
};
static_assert(sizeof(M2HNotifyKey) == EnvelopeSize, "M2HNotifyKey must be EnvelopeSize bytes");

/// Forward a pointer (mouse) event from the VNC client to the portal.
struct M2HNotifyPointer {
    M2HType type{M2HType::NotifyPointer};
    int32_t buttonMask{0}; ///< LibVNCServer button bitmask
    int32_t x{0};
    int32_t y{0};
    uint8_t _pad[3]{};
};
static_assert(sizeof(M2HNotifyPointer) == EnvelopeSize, "M2HNotifyPointer must be EnvelopeSize bytes");
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Helper: send a fixed-size envelope over the SOCK_SEQPACKET socket
// ---------------------------------------------------------------------------

/**
 * @brief Send exactly EnvelopeSize bytes from @p data over @p socketFd.
 *
 * The caller must ensure that @p data points to a buffer of at least
 * EnvelopeSize bytes (all message structs are padded to exactly that size).
 *
 * If @p fd is >= 0 it is attached as SCM_RIGHTS ancillary data so that the
 * peer can retrieve it with recvMessage().
 *
 * @return true on success, false on error.
 */
inline bool sendMessage(int socketFd, const void* data, int fd = -1)
{
    struct msghdr msg{};
    struct iovec  iov{};

    iov.iov_base   = const_cast<void*>(data);
    iov.iov_len    = EnvelopeSize;
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    // Allocate ancillary-data buffer on the stack; only populated when fd >= 0
    char cmsgBuf[CMSG_SPACE(sizeof(int))]{};
    if (fd >= 0)
    {
        msg.msg_control    = cmsgBuf;
        msg.msg_controllen = sizeof(cmsgBuf);
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type  = SCM_RIGHTS;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
        std::memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
    }

    return ::sendmsg(socketFd, &msg, MSG_NOSIGNAL) >= 0;
}

// ---------------------------------------------------------------------------
// Helper: receive a fixed-size envelope from the SOCK_SEQPACKET socket
// ---------------------------------------------------------------------------

/**
 * @brief Receive one EnvelopeSize-byte message into @p data from @p socketFd.
 *
 * Because the socket is SOCK_SEQPACKET each recv() returns exactly one
 * record, so no MSG_WAITALL or partial-read handling is required.
 *
 * If @p outFd is non-null it is set to any SCM_RIGHTS FD received in the
 * ancillary data, or -1 if no FD was attached.
 *
 * @return EnvelopeSize on success, 0 if the peer closed the socket, or -1
 *         on error.
 */
inline ssize_t recvMessage(int socketFd, void* data, int* outFd = nullptr)
{
    struct msghdr msg{};
    struct iovec  iov{};

    iov.iov_base   = data;
    iov.iov_len    = EnvelopeSize;
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    char cmsgBuf[CMSG_SPACE(sizeof(int))]{};
    msg.msg_control    = cmsgBuf;
    msg.msg_controllen = sizeof(cmsgBuf);

    if (outFd != nullptr)
    {
        *outFd = -1;
    }

    const ssize_t n = ::recvmsg(socketFd, &msg, 0);
    if (n > 0 && outFd != nullptr)
    {
        for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr;
             cmsg = CMSG_NXTHDR(&msg, cmsg))
        {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
            {
                std::memcpy(outFd, CMSG_DATA(cmsg), sizeof(int));
                break;
            }
        }
    }
    return n;
}

} // namespace PortalHelperProtocol
