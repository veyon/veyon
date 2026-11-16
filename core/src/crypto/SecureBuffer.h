// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

#include <optional>

namespace VeyonCrypto {

// Owning byte buffer for secrets (keys, tokens, passphrases, plaintexts).
//
// - Buffers of up to MaximumSecureHeapAllocationSize bytes (keys, tokens, passphrases) come from the
//   OpenSSL secure heap when the process initialised it (see initializeSecureHeap(), mlock'ed and
//   excluded from core dumps) and the heap has room for them (see there), all other buffers from
//   the normal heap; memory is always zeroised (OPENSSL_cleanse) before it is released: on destruction, clear(),
//   resize() and move assignment.
// - It is move-only. Copies must be made explicitly with clone(), and there is no implicit
//   conversion to QByteArray; toUnprotectedByteArray() is the single, greppable escape hatch for
//   APIs that require a QByteArray (e.g. QSslKey) and documents that the copy is not zeroised.
// - There is no operator==; compare secrets with VeyonCrypto::constantTimeEquals().
// - Sizes outside 0..MaximumSize are never clamped: such a buffer stays empty (size() 0,
//   data() nullptr), resize() fails. Allocation failures abort like QByteArray does (Qt is built
//   without exceptions).
class VEYON_CORE_EXPORT SecureBuffer
{
public:
	// nothing in Veyon keeps larger secrets (IPC records, exports, keys)
	static constexpr qsizetype MaximumSize = 16 * 1024 * 1024;

	// larger buffers (bulk plaintexts, encoded exports) never use the secure heap
	static constexpr qsizetype MaximumSecureHeapAllocationSize = 4 * 1024;

	static constexpr qsizetype MinimumSecureHeapSize = 16 * 1024;
	static constexpr qsizetype MaximumSecureHeapSize = 64 * 1024 * 1024;

	enum class SecureHeapState {
		Unavailable, // no secure heap: invalid size, no platform/OpenSSL support or setup failed
		NotLocked,	 // secure heap set up, but mlock() failed (still excluded from core dumps)
		Locked,
	};

	SecureBuffer() noexcept = default;
	// zero-initialised buffer of the given size; empty if size is outside 0..MaximumSize
	explicit SecureBuffer(qsizetype size);
	// copy of size bytes at data; empty if data is nullptr or size is outside 0..MaximumSize
	SecureBuffer(const char* data, qsizetype size);
	~SecureBuffer();

	SecureBuffer(SecureBuffer&& other) noexcept;
	SecureBuffer& operator=(SecureBuffer&& other) noexcept;

	SecureBuffer(const SecureBuffer&) = delete;
	SecureBuffer& operator=(const SecureBuffer&) = delete;

	// copies the bytes of a QByteArray; the caller remains responsible for the source
	[[nodiscard]] static SecureBuffer fromByteArray(const QByteArray& data);

	[[nodiscard]] SecureBuffer clone() const;

	// lower-case hex encoding/strict decoding (even length, [0-9a-fA-F] only) without leaving
	// secure memory, e.g. for the token line of 02-ipc.md §2
	[[nodiscard]] std::optional<SecureBuffer> toHex() const;
	[[nodiscard]] static std::optional<SecureBuffer> fromHex(const char* hex, qsizetype size);

	[[nodiscard]] QByteArray toUnprotectedByteArray() const;

	char* data()
	{
		return m_data;
	}

	const char* data() const
	{
		return m_data;
	}

	const char* constData() const
	{
		return m_data;
	}

	unsigned char* bytes()
	{
		return reinterpret_cast<unsigned char*>(m_data);
	}

	const unsigned char* bytes() const
	{
		return reinterpret_cast<const unsigned char*>(m_data);
	}

	qsizetype size() const
	{
		return m_size;
	}

	bool isEmpty() const
	{
		return m_size == 0;
	}

	// keeps the first min(size(), newSize) bytes, new bytes are zero; the old memory is zeroised;
	// false (buffer unchanged) if newSize is outside 0..MaximumSize
	[[nodiscard]] bool resize(qsizetype newSize);

	// zeroises and releases the memory
	void clear();

	// overwrites the contents with zeros, keeping the size
	void wipe();

	bool isInSecureHeap() const;

	// Initialises the OpenSSL secure heap for this process. Call it once, early in main() of
	// privileged processes before any SecureBuffer is created. size must be a power of two within
	// MinimumSecureHeapSize..MaximumSecureHeapSize, otherwise nothing happens (Unavailable).
	// Repeated calls return the state of the first one; if other code initialised the heap,
	// SecureBuffers do not use it.
	// Failure mode: OpenSSL allocates the internals of private keys and big numbers (key
	// generation, parsing, signing, key agreement, HKDF state) from the secure heap and fails
	// these operations, instead of falling back to the normal heap, when it is exhausted.
	// SecureBuffers therefore use it only while the heap stays at most half full, i.e. at least
	// half of it is always left to OpenSSL; beyond that they use the normal heap (still zeroised)
	// and a warning is logged once.
	// Sizing: at least twice the small SecureBuffers alive at the same time (each rounded up to a
	// power of two), and enough for OpenSSL's private keys and concurrent private key operations
	// in the other half; 64 KiB suffice for Veyon's services.
	[[nodiscard]] static SecureHeapState initializeSecureHeap(qsizetype size);
	[[nodiscard]] static bool isSecureHeapInitialized();

private:
	char* m_data = nullptr;
	qsizetype m_size = 0;
};



// Non-owning view of contiguous bytes, accepted by all VeyonCrypto functions so that they can
// operate on SecureBuffer and QByteArray without copies. The viewed data must outlive the view;
// use it only as a function parameter type.
class ByteView
{
public:
	constexpr ByteView() noexcept = default;

	constexpr ByteView(const char* data, qsizetype size) noexcept :
		m_data(size > 0 ? data : nullptr),
		m_size(size > 0 && data != nullptr ? size : 0)
	{
	}

	ByteView(const QByteArray& data) noexcept :
		ByteView(data.constData(), data.size())
	{
	}

	ByteView(const SecureBuffer& buffer) noexcept :
		ByteView(buffer.constData(), buffer.size())
	{
	}

	// never nullptr, also for empty views (some OpenSSL functions reject nullptr)
	const char* data() const
	{
		return m_data ? m_data : "";
	}

	const unsigned char* bytes() const
	{
		return reinterpret_cast<const unsigned char*>(data());
	}

	qsizetype size() const
	{
		return m_size;
	}

	bool isEmpty() const
	{
		return m_size == 0;
	}

private:
	const char* m_data = nullptr;
	qsizetype m_size = 0;
};

} // namespace VeyonCrypto
