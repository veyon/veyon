// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "SecureBuffer.h"

#include <cstdlib>
#include <mutex>
#include <openssl/crypto.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Zeroisation checks for SecureBuffer. Released memory must not be read (undefined behaviour), so
// the checks run inside OpenSSL's memory functions: this test installs its own allocator via
// CRYPTO_set_mem_functions() (only possible before OpenSSL allocates anything, hence the custom
// main()) and verifies that watched SecureBuffer memory is all-zero at the moment it is freed.
// The test links neither veyon-crypto nor Qt Network, whose dependencies may already allocate
// through OpenSSL while being loaded.



namespace {

std::mutex allocationMutex;
std::unordered_map<void*, size_t> allocationSizes;
std::unordered_set<const void*> watchedAllocations;
int releasedWatchedAllocations = 0;
int releasedNonZeroAllocations = 0;

void* trackingMalloc(size_t size, const char* file, int line)
{
	Q_UNUSED(file)
	Q_UNUSED(line)
	auto memory = std::malloc(size);
	if (memory)
	{
		std::lock_guard<std::mutex> lock(allocationMutex);
		allocationSizes[memory] = size;
	}
	return memory;
}



void* trackingRealloc(void* memory, size_t size, const char* file, int line)
{
	Q_UNUSED(file)
	Q_UNUSED(line)
	std::lock_guard<std::mutex> lock(allocationMutex);
	allocationSizes.erase(memory);
	auto newMemory = std::realloc(memory, size);
	if (newMemory)
	{
		allocationSizes[newMemory] = size;
	}
	return newMemory;
}



void trackingFree(void* memory, const char* file, int line)
{
	Q_UNUSED(file)
	Q_UNUSED(line)
	if (memory)
	{
		std::lock_guard<std::mutex> lock(allocationMutex);
		const auto size = allocationSizes.find(memory);
		if (watchedAllocations.erase(memory) > 0 && size != allocationSizes.end())
		{
			++releasedWatchedAllocations;
			const auto bytes = static_cast<const unsigned char*>(memory);
			for (size_t i = 0; i < size->second; ++i)
			{
				if (bytes[i] != 0)
				{
					++releasedNonZeroAllocations;
					break;
				}
			}
		}
		if (size != allocationSizes.end())
		{
			allocationSizes.erase(size);
		}
	}
	std::free(memory);
}



void watch(const Veyon::Crypto::SecureBuffer& buffer)
{
	std::lock_guard<std::mutex> lock(allocationMutex);
	watchedAllocations.insert(buffer.constData());
}

} // namespace



class VeyonCryptoSecureBufferTest : public QObject
{
	Q_OBJECT
public:
	explicit VeyonCryptoSecureBufferTest(bool memoryFunctionsInstalled) :
		m_memoryFunctionsInstalled(memoryFunctionsInstalled)
	{
	}

private Q_SLOTS:
	void init();
	void zeroisedBeforeRelease();
	// must be last: initialises the process-wide secure heap
	void secureHeapUsageAndFallback();

private:
	const bool m_memoryFunctionsInstalled;
};



void VeyonCryptoSecureBufferTest::init()
{
	std::lock_guard<std::mutex> lock(allocationMutex);
	watchedAllocations.clear();
	releasedWatchedAllocations = 0;
	releasedNonZeroAllocations = 0;
}



void VeyonCryptoSecureBufferTest::zeroisedBeforeRelease()
{
	if (m_memoryFunctionsInstalled == false)
	{
		QSKIP("OpenSSL memory functions could not be replaced");
	}
	QVERIFY(Veyon::Crypto::SecureBuffer::isSecureHeapInitialized() == false);

	const QByteArray secret(100, char(0xa5));

	// destructor
	{
		auto buffer = Veyon::Crypto::SecureBuffer::fromByteArray(secret);
		QVERIFY(buffer.isInSecureHeap() == false);
		watch(buffer);
	}

	// clear()
	auto buffer = Veyon::Crypto::SecureBuffer::fromByteArray(secret);
	watch(buffer);
	buffer.clear();

	// resize() releases the old memory
	buffer = Veyon::Crypto::SecureBuffer::fromByteArray(secret);
	watch(buffer);
	QVERIFY(buffer.resize(200));
	QCOMPARE(buffer.toUnprotectedByteArray(), QByteArray(secret + QByteArray(100, 0)));

	// move assignment releases the previous contents of the target
	watch(buffer);
	buffer = Veyon::Crypto::SecureBuffer::fromByteArray(QByteArray("other"));
	buffer.clear();

	std::lock_guard<std::mutex> lock(allocationMutex);
	QCOMPARE(releasedWatchedAllocations, 4);
	QCOMPARE(releasedNonZeroAllocations, 0);
}



static int secureHeapWarnings = 0;

static void countSecureHeapWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
	Q_UNUSED(context)
	if (type == QtWarningMsg && message.contains(QStringLiteral("secure heap share for SecureBuffers exhausted")))
	{
		++secureHeapWarnings;
	}
}



void VeyonCryptoSecureBufferTest::secureHeapUsageAndFallback()
{
	using Veyon::Crypto::SecureBuffer;

	const auto state = SecureBuffer::initializeSecureHeap(64 * 1024);
	if (state == SecureBuffer::SecureHeapState::Unavailable)
	{
		QSKIP("OpenSSL secure heap not available on this platform");
	}
	qInfo() << "secure heap locked:" << (state == SecureBuffer::SecureHeapState::Locked);
	QVERIFY(SecureBuffer::isSecureHeapInitialized());
	QCOMPARE(SecureBuffer::initializeSecureHeap(64 * 1024), state);
	QCOMPARE(SecureBuffer::initializeSecureHeap(1000), state);

	// small buffers come from the secure heap ...
	static constexpr qsizetype Size = SecureBuffer::MaximumSecureHeapAllocationSize;
	std::vector<SecureBuffer> buffers;
	buffers.emplace_back(Size);
	QVERIFY(buffers.front().isInSecureHeap());

	// ... large ones never
	QVERIFY(SecureBuffer(Size + 1).isInSecureHeap() == false);

	// ... until half of the heap is used: then from the normal heap (still usable and
	// zero-initialised), reported once
	const auto previousHandler = qInstallMessageHandler(countSecureHeapWarnings);
	int fallbacks = 0;
	for (int i = 0; i < 16; ++i)
	{
		buffers.emplace_back(Size);
		if (buffers.back().isInSecureHeap() == false)
		{
			++fallbacks;
			QCOMPARE(buffers.back().size(), Size);
			QCOMPARE(buffers.back().toUnprotectedByteArray(), QByteArray(int(Size), 0));
		}
	}
	qInstallMessageHandler(previousHandler);
	QVERIFY(fallbacks >= 8);
	QCOMPARE(secureHeapWarnings, 1);
	QVERIFY(CRYPTO_secure_used() <= 32 * 1024);
	buffers.clear();

	// Released secure heap blocks are zero behind the 16 bytes of free-list links the arena
	// allocator writes there. Note: OpenSSL itself clears secure heap memory on release, so this
	// documents the property rather than SecureBuffer's own wiping, which zeroisedBeforeRelease()
	// verifies on the normal heap.
	static constexpr qsizetype FreeListHeaderSize = 16;
	const char* released = nullptr;
	{
		auto buffer = SecureBuffer::fromByteArray(QByteArray(256, char(0xa5)));
		QVERIFY(buffer.isInSecureHeap());
		released = buffer.constData();
	}
	for (qsizetype i = FreeListHeaderSize; i < 256; ++i)
	{
		QCOMPARE(int(released[i]), 0);
	}
}



int main(int argc, char** argv)
{
	// before anything else can make OpenSSL allocate memory
	const auto memoryFunctionsInstalled = CRYPTO_set_mem_functions(trackingMalloc, trackingRealloc, trackingFree) == 1;

	QCoreApplication app(argc, argv);
	VeyonCryptoSecureBufferTest test(memoryFunctionsInstalled);
	return QTest::qExec(&test, argc, argv);
}

#include "VeyonCryptoSecureBufferTest.moc"
