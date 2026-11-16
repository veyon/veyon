// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SecureBuffer.h"

#include <atomic>
#include <cstring>
#include <openssl/crypto.h>
#include <utility>

namespace VeyonCrypto {

static std::atomic<SecureBuffer::SecureHeapState> secureHeapState{SecureBuffer::SecureHeapState::Unavailable};
static std::atomic<qsizetype> secureHeapSize{0};
static std::atomic<bool> secureHeapFallbackReported{false};



static bool isValidSize(qsizetype size)
{
	return size >= 0 && size <= SecureBuffer::MaximumSize;
}



// Once the secure heap is initialised, OpenSSL allocates its own secrets (private key and BIGNUM
// internals) from it and fails instead of falling back to the normal heap. SecureBuffers therefore
// only take small allocations from it and leave at least half of it to OpenSSL.
static bool mayUseSecureHeap(qsizetype size)
{
	static constexpr qsizetype SecureHeapMinimumBlockSize = 32;

	const auto heapSize = secureHeapSize.load();
	if (heapSize <= 0 || size > SecureBuffer::MaximumSecureHeapAllocationSize)
	{
		return false;
	}

	// the secure heap is a buddy allocator, i.e. it hands out blocks of power-of-two sizes
	qsizetype blockSize = SecureHeapMinimumBlockSize;
	while (blockSize < size)
	{
		blockSize *= 2;
	}

	if (qsizetype(CRYPTO_secure_used()) + blockSize > heapSize / 2)
	{
		if (secureHeapFallbackReported.exchange(true) == false)
		{
			vWarning() << "secure heap share for SecureBuffers exhausted, using the normal heap for secrets "
						  "(increase its size)";
		}
		return false;
	}

	return true;
}



static char* allocateZeroed(qsizetype size)
{
	if (size <= 0 || isValidSize(size) == false)
	{
		return nullptr;
	}

	void* memory = nullptr;
	if (mayUseSecureHeap(size))
	{
		// may still fail when other threads allocated concurrently
		memory = OPENSSL_secure_zalloc(size_t(size));
	}
	if (memory == nullptr)
	{
		memory = OPENSSL_zalloc(size_t(size));
	}
	Q_CHECK_PTR(memory);

	return static_cast<char*>(memory);
}



static void releaseZeroed(char* data, qsizetype size)
{
	if (data)
	{
		// zeroises and frees memory from the secure as well as from the normal heap
		OPENSSL_secure_clear_free(data, size_t(size));
	}
}



SecureBuffer::SecureBuffer(qsizetype size) :
	m_data(allocateZeroed(size)),
	m_size(m_data ? size : 0)
{
}



SecureBuffer::SecureBuffer(const char* data, qsizetype size) :
	SecureBuffer(data ? size : 0)
{
	if (m_size > 0)
	{
		std::memcpy(m_data, data, size_t(m_size));
	}
}



SecureBuffer::~SecureBuffer()
{
	clear();
}



SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept :
	m_data(std::exchange(other.m_data, nullptr)),
	m_size(std::exchange(other.m_size, 0))
{
}



SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept
{
	if (this != &other)
	{
		clear();
		m_data = std::exchange(other.m_data, nullptr);
		m_size = std::exchange(other.m_size, 0);
	}
	return *this;
}



SecureBuffer SecureBuffer::fromByteArray(const QByteArray& data)
{
	return SecureBuffer(data.constData(), data.size());
}



SecureBuffer SecureBuffer::clone() const
{
	return SecureBuffer(m_data, m_size);
}



std::optional<SecureBuffer> SecureBuffer::toHex() const
{
	static constexpr char Digits[] = "0123456789abcdef";

	if (m_size > MaximumSize / 2)
	{
		return std::nullopt;
	}

	SecureBuffer hex(m_size * 2);
	for (qsizetype i = 0; i < m_size; ++i)
	{
		const auto byte = static_cast<unsigned char>(m_data[i]);
		hex.m_data[2 * i] = Digits[byte >> 4];
		hex.m_data[2 * i + 1] = Digits[byte & 0x0f];
	}
	return hex;
}



std::optional<SecureBuffer> SecureBuffer::fromHex(const char* hex, qsizetype size)
{
	const auto nibble = [](char digit) -> int {
		if (digit >= '0' && digit <= '9')
		{
			return digit - '0';
		}
		if (digit >= 'a' && digit <= 'f')
		{
			return digit - 'a' + 10;
		}
		if (digit >= 'A' && digit <= 'F')
		{
			return digit - 'A' + 10;
		}
		return -1;
	};

	if (size < 0 || size % 2 != 0 || size / 2 > MaximumSize || (size > 0 && hex == nullptr))
	{
		return std::nullopt;
	}

	SecureBuffer bytes(size / 2);
	for (qsizetype i = 0; i < bytes.size(); ++i)
	{
		const auto high = nibble(hex[2 * i]);
		const auto low = nibble(hex[2 * i + 1]);
		if (high < 0 || low < 0)
		{
			return std::nullopt;
		}
		bytes.m_data[i] = char((high << 4) | low);
	}
	return bytes;
}



QByteArray SecureBuffer::toUnprotectedByteArray() const
{
	return QByteArray(m_data, int(m_size));
}



bool SecureBuffer::resize(qsizetype newSize)
{
	if (isValidSize(newSize) == false)
	{
		return false;
	}
	if (newSize == m_size)
	{
		return true;
	}

	auto newData = allocateZeroed(newSize);
	if (m_data && newData)
	{
		std::memcpy(newData, m_data, size_t(qMin(m_size, newSize)));
	}
	releaseZeroed(m_data, m_size);

	m_data = newData;
	m_size = newData ? newSize : 0;
	return true;
}



void SecureBuffer::clear()
{
	releaseZeroed(m_data, m_size);
	m_data = nullptr;
	m_size = 0;
}



void SecureBuffer::wipe()
{
	if (m_data)
	{
		OPENSSL_cleanse(m_data, size_t(m_size));
	}
}



bool SecureBuffer::isInSecureHeap() const
{
	return m_data && CRYPTO_secure_allocated(m_data);
}



SecureBuffer::SecureHeapState SecureBuffer::initializeSecureHeap(qsizetype size)
{
	static constexpr int MinimumAllocationSize = 32;

	if (CRYPTO_secure_malloc_initialized())
	{
		// initialised by an earlier call (or by other code: then the lock state is unknown)
		const auto state = secureHeapState.load();
		return state == SecureHeapState::Unavailable ? SecureHeapState::NotLocked : state;
	}

	// OpenSSL aborts the process for sizes that are not a power of two
	const auto isPowerOfTwo = size > 0 && (size & (size - 1)) == 0;
	if (isPowerOfTwo == false || size < MinimumSecureHeapSize || size > MaximumSecureHeapSize)
	{
		return SecureHeapState::Unavailable;
	}

	// 1: success, 2: memory could not be locked, 0: failure or no secure heap support
	switch (CRYPTO_secure_malloc_init(size_t(size), MinimumAllocationSize))
	{
	case 1: secureHeapState = SecureHeapState::Locked; break;
	case 2: secureHeapState = SecureHeapState::NotLocked; break;
	default: secureHeapState = SecureHeapState::Unavailable; return SecureHeapState::Unavailable;
	}
	secureHeapSize = size;
	return secureHeapState;
}



bool SecureBuffer::isSecureHeapInitialized()
{
	return CRYPTO_secure_malloc_initialized() != 0;
}

} // namespace VeyonCrypto
