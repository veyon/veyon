// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QStringList>
#include <QTest>

#include "PipeWireFrame.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

extern "C" {
#include <spa/buffer/buffer.h>
#include <spa/buffer/meta.h>
#include <spa/param/video/raw.h>
}

// Frame validation and copy of veyon-capture-helper (02-ipc.md §1a.5): every rejection branch of
// PipeWireFrame, damage clipping and bounding, and the BGRx to R,G,B,x copy.
class PipeWireFrameTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void validSize_data();
	void validSize();
	void supportedFormat();

	void acceptsTightAndPaddedFrames();
	void acceptsMemFdAndChunkOffset();
	void rejectsInvalidSizeAndFormat();
	void rejectsUnsupportedDataTypes_data();
	void rejectsUnsupportedDataTypes();
	void rejectsUnmappedOrMissingChunk();
	void reportsChunksWithoutContent_data();
	void reportsChunksWithoutContent();
	void ignoresUnknownChunkFlags();
	void rejectsInvalidStride_data();
	void rejectsInvalidStride();
	void rejectsChunkOutsideData_data();
	void rejectsChunkOutsideData();
	void rejectsTruncatedFrames_data();
	void rejectsTruncatedFrames();

	void damageWithoutMetadataIsFullFrame();
	void damageWithoutValidRegionIsFullFrame();
	void damageRegionsAreClipped();
	void damageOutsideFrameIsSkipped();
	void damageStopsAtTerminator();
	void damageIsBoundedByMetadataSize();
	void damageIsBoundedByRegionLimit();
	void damageFromUnalignedMetadata();

	void copyConvertsFullFrame();
	void copyOnlyDamagedRects();
	void copyFallsBackToFullFrameForLargeDamage();
	void copyWithClippedAwayDamageCopiesNothing();
	void copyRejectsInvalidSource();
	void copyRejectsInvalidTarget();
	void copyRejectsInvalidDamage_data();
	void copyRejectsInvalidDamage();

	void statusNames();

private:
	struct Frame
	{
		std::vector<uchar> data;
		PipeWireFrame::BufferData bufferData;
	};

	// BGRx frame with pixel (x, y) = B: x, G: y, R: 0x80 | index, x: 0xff and 0xee padding bytes
	static Frame makeFrame(int width, int height, int stride, uint32_t offset = 0, uint32_t extra = 0)
	{
		Frame frame;
		const auto size = uint32_t(stride) * uint32_t(height);
		frame.data.assign(offset + size + extra, 0xee);
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				auto* pixel = frame.data.data() + offset + y * stride + x * 4;
				pixel[0] = uchar(x);
				pixel[1] = uchar(y);
				pixel[2] = uchar(0x80 | ((y * width + x) & 0x7f));
				pixel[3] = 0xff;
			}
		}
		frame.bufferData = {SPA_DATA_MemPtr, frame.data.data(), uint32_t(frame.data.size()),
							PipeWireFrame::Chunk{offset, size, stride, 0}};
		return frame;
	}

	static std::vector<uchar> makeDamage(const std::vector<spa_region>& regions)
	{
		std::vector<uchar> meta(regions.size() * sizeof(spa_meta_region));
		for (size_t i = 0; i < regions.size(); ++i)
		{
			const spa_meta_region region{regions[i]};
			std::memcpy(meta.data() + i * sizeof(region), &region, sizeof(region));
		}
		return meta;
	}

	static PipeWireFrame::Damage parse(const std::vector<uchar>& meta, int width, int height)
	{
		return PipeWireFrame::parseDamage(meta.data(), uint32_t(meta.size()), width, height);
	}

	static bool isConverted(const Frame& frame, const std::vector<uchar>& target, int targetStride,
							int x, int y, int sourceStride, uint32_t offset = 0)
	{
		const auto* source = frame.data.data() + offset + y * sourceStride + x * 4;
		const auto* pixel = target.data() + y * targetStride + x * 4;
		return pixel[0] == source[2] && pixel[1] == source[1] && pixel[2] == source[0] && pixel[3] == source[3];
	}

	static constexpr uchar Untouched = 0x5a;
	static constexpr auto BGRx = uint32_t(SPA_VIDEO_FORMAT_BGRx);
};



void PipeWireFrameTest::validSize_data()
{
	QTest::addColumn<qint64>("width");
	QTest::addColumn<qint64>("height");
	QTest::addColumn<bool>("valid");

	QTest::newRow("1x1") << qint64(1) << qint64(1) << true;
	QTest::newRow("1080p") << qint64(1920) << qint64(1080) << true;
	QTest::newRow("8K") << qint64(7680) << qint64(4320) << true;
	QTest::newRow("two 8K side by side") << qint64(15360) << qint64(4320) << true;
	QTest::newRow("maximum width at pixel limit") << qint64(16384) << qint64(4096) << true;
	QTest::newRow("maximum height at pixel limit") << qint64(4096) << qint64(16384) << true;
	QTest::newRow("zero width") << qint64(0) << qint64(1080) << false;
	QTest::newRow("zero height") << qint64(1920) << qint64(0) << false;
	QTest::newRow("negative width") << qint64(-1) << qint64(1080) << false;
	QTest::newRow("negative height") << qint64(1920) << qint64(-1) << false;
	QTest::newRow("width above maximum") << qint64(16385) << qint64(1) << false;
	QTest::newRow("height above maximum") << qint64(1) << qint64(16385) << false;
	QTest::newRow("above pixel limit") << qint64(16384) << qint64(4097) << false;
	QTest::newRow("maximum dimensions") << qint64(16384) << qint64(16384) << false;
	QTest::newRow("uint32 width") << qint64(std::numeric_limits<uint32_t>::max()) << qint64(1) << false;
	QTest::newRow("int overflow of width*height*4") << qint64(32768) << qint64(16384) << false;
}



void PipeWireFrameTest::validSize()
{
	QFETCH(qint64, width);
	QFETCH(qint64, height);
	QFETCH(bool, valid);

	QCOMPARE(PipeWireFrame::isValidSize(width, height), valid);
}



void PipeWireFrameTest::supportedFormat()
{
	QVERIFY(PipeWireFrame::isSupportedFormat(SPA_VIDEO_FORMAT_BGRx));

	// not negotiated (PipeWireFramebuffer::open() only offers BGRx)
	QVERIFY(PipeWireFrame::isSupportedFormat(SPA_VIDEO_FORMAT_UNKNOWN) == false);
	QVERIFY(PipeWireFrame::isSupportedFormat(SPA_VIDEO_FORMAT_BGRA) == false);
	QVERIFY(PipeWireFrame::isSupportedFormat(SPA_VIDEO_FORMAT_RGBx) == false);
	QVERIFY(PipeWireFrame::isSupportedFormat(SPA_VIDEO_FORMAT_RGB) == false);
	QVERIFY(PipeWireFrame::isSupportedFormat(SPA_VIDEO_FORMAT_I420) == false);
	QVERIFY(PipeWireFrame::isSupportedFormat(std::numeric_limits<uint32_t>::max()) == false);
}



void PipeWireFrameTest::acceptsTightAndPaddedFrames()
{
	auto tight = makeFrame(3, 2, 12);
	auto source = PipeWireFrame::validate(3, 2, BGRx, tight.bufferData);
	QCOMPARE(source.status, PipeWireFrame::Status::Valid);
	QCOMPARE(source.pixels, tight.data.data());
	QCOMPARE(source.stride, 12);

	// stride 0: tightly packed
	tight.bufferData.chunk->stride = 0;
	source = PipeWireFrame::validate(3, 2, BGRx, tight.bufferData);
	QCOMPARE(source.status, PipeWireFrame::Status::Valid);
	QCOMPARE(source.stride, 12);

	const auto padded = makeFrame(3, 2, 20);
	source = PipeWireFrame::validate(3, 2, BGRx, padded.bufferData);
	QCOMPARE(source.status, PipeWireFrame::Status::Valid);
	QCOMPARE(source.stride, 20);

	// an unaligned stride is fine, the copy is byte-wise
	const auto unaligned = makeFrame(3, 2, 13);
	QCOMPARE(PipeWireFrame::validate(3, 2, BGRx, unaligned.bufferData).status, PipeWireFrame::Status::Valid);
}



void PipeWireFrameTest::acceptsMemFdAndChunkOffset()
{
	auto frame = makeFrame(2, 2, 8, 5, 3);
	frame.bufferData.type = SPA_DATA_MemFd;

	const auto source = PipeWireFrame::validate(2, 2, BGRx, frame.bufferData);
	QCOMPARE(source.status, PipeWireFrame::Status::Valid);
	QCOMPARE(source.pixels, frame.data.data() + 5);
	QCOMPARE(source.stride, 8);

	// chunk ends exactly at maxSize
	frame.bufferData.maxSize = 5 + 16;
	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, frame.bufferData).status, PipeWireFrame::Status::Valid);
}



void PipeWireFrameTest::rejectsInvalidSizeAndFormat()
{
	const auto frame = makeFrame(2, 2, 8);

	QCOMPARE(PipeWireFrame::validate(0, 2, BGRx, frame.bufferData).status, PipeWireFrame::Status::InvalidSize);
	QCOMPARE(PipeWireFrame::validate(2, -1, BGRx, frame.bufferData).status, PipeWireFrame::Status::InvalidSize);
	QCOMPARE(PipeWireFrame::validate(PipeWireFrame::MaximumDimension + 1, 1, BGRx, frame.bufferData).status,
			 PipeWireFrame::Status::InvalidSize);
	QCOMPARE(PipeWireFrame::validate(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
									 BGRx, frame.bufferData)
				 .status,
			 PipeWireFrame::Status::InvalidSize);

	QCOMPARE(PipeWireFrame::validate(2, 2, SPA_VIDEO_FORMAT_RGB, frame.bufferData).status,
			 PipeWireFrame::Status::UnsupportedFormat);
	QCOMPARE(PipeWireFrame::validate(2, 2, SPA_VIDEO_FORMAT_UNKNOWN, frame.bufferData).status,
			 PipeWireFrame::Status::UnsupportedFormat);
}



void PipeWireFrameTest::rejectsUnsupportedDataTypes_data()
{
	QTest::addColumn<uint32_t>("type");

	QTest::newRow("Invalid") << uint32_t(SPA_DATA_Invalid);
	QTest::newRow("DmaBuf") << uint32_t(SPA_DATA_DmaBuf);
	QTest::newRow("MemId") << uint32_t(SPA_DATA_MemId);
	QTest::newRow("unknown") << uint32_t(0x1000);
}



void PipeWireFrameTest::rejectsUnsupportedDataTypes()
{
	QFETCH(uint32_t, type);

	auto frame = makeFrame(2, 2, 8);
	frame.bufferData.type = type;

	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, frame.bufferData).status, PipeWireFrame::Status::UnsupportedDataType);

	// no datas[0] at all
	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, {}).status, PipeWireFrame::Status::UnsupportedDataType);
}



void PipeWireFrameTest::rejectsUnmappedOrMissingChunk()
{
	auto unmapped = makeFrame(2, 2, 8);
	unmapped.bufferData.data = nullptr;
	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, unmapped.bufferData).status, PipeWireFrame::Status::NotMapped);

	auto withoutChunk = makeFrame(2, 2, 8);
	withoutChunk.bufferData.chunk.reset();
	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, withoutChunk.bufferData).status, PipeWireFrame::Status::MissingChunk);
}



void PipeWireFrameTest::reportsChunksWithoutContent_data()
{
	QTest::addColumn<uint32_t>("flags");
	QTest::addColumn<uint32_t>("size");

	// what producers send for cursor-only updates
	QTest::newRow("corrupted, size 0 (mutter 46, KWin 5.27)") << uint32_t(SPA_CHUNK_FLAG_CORRUPTED) << uint32_t(0);
	QTest::newRow("corrupted, full size (KWin 6)") << uint32_t(SPA_CHUNK_FLAG_CORRUPTED) << uint32_t(16);
	QTest::newRow("corrupted and empty") << uint32_t(SPA_CHUNK_FLAG_CORRUPTED | (1u << 1)) << uint32_t(16);
	QTest::newRow("empty flag (SPA_CHUNK_FLAG_EMPTY)") << uint32_t(1u << 1) << uint32_t(16);
	QTest::newRow("size 0") << uint32_t(0) << uint32_t(0);
}



void PipeWireFrameTest::reportsChunksWithoutContent()
{
	QFETCH(uint32_t, flags);
	QFETCH(uint32_t, size);

	auto frame = makeFrame(2, 2, 8);
	frame.bufferData.chunk->flags = int32_t(flags);
	frame.bufferData.chunk->size = size;

	const auto source = PipeWireFrame::validate(2, 2, BGRx, frame.bufferData);
	QCOMPARE(source.status, PipeWireFrame::Status::NoContent);
	QVERIFY(source.pixels == nullptr);
}



void PipeWireFrameTest::ignoresUnknownChunkFlags()
{
	auto frame = makeFrame(2, 2, 8);

	// unknown flags are ignored
	frame.bufferData.chunk->flags = int32_t(0x80000000u);
	frame.bufferData.chunk->size = 16;
	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, frame.bufferData).status, PipeWireFrame::Status::Valid);
}



void PipeWireFrameTest::rejectsInvalidStride_data()
{
	QTest::addColumn<int32_t>("stride");

	QTest::newRow("negative") << int32_t(-8);
	QTest::newRow("int32 minimum") << std::numeric_limits<int32_t>::min();
	QTest::newRow("one byte") << int32_t(1);
	QTest::newRow("one byte short of a row") << int32_t(7);
	QTest::newRow("three bytes per pixel") << int32_t(6);
}



void PipeWireFrameTest::rejectsInvalidStride()
{
	QFETCH(int32_t, stride);

	// enough data for any of these strides, so only the stride itself can be the reason
	auto frame = makeFrame(2, 2, 8, 0, 64);
	frame.bufferData.chunk->stride = stride;

	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, frame.bufferData).status, PipeWireFrame::Status::InvalidStride);
}



void PipeWireFrameTest::rejectsChunkOutsideData_data()
{
	QTest::addColumn<uint32_t>("offset");
	QTest::addColumn<uint32_t>("size");
	QTest::addColumn<uint32_t>("maxSize");

	QTest::newRow("offset beyond maxSize") << uint32_t(17) << uint32_t(16) << uint32_t(16);
	QTest::newRow("offset + size beyond maxSize") << uint32_t(4) << uint32_t(16) << uint32_t(16);
	QTest::newRow("size beyond maxSize") << uint32_t(0) << uint32_t(17) << uint32_t(16);
	QTest::newRow("offset + size wraps around") << uint32_t(0xfffffff8) << uint32_t(16) << uint32_t(16);
	QTest::newRow("size wraps around") << uint32_t(8) << uint32_t(0xfffffffc) << uint32_t(16);
	QTest::newRow("maximum offset") << std::numeric_limits<uint32_t>::max() << uint32_t(16) << uint32_t(16);
	QTest::newRow("empty mapping") << uint32_t(0) << uint32_t(16) << uint32_t(0);
}



void PipeWireFrameTest::rejectsChunkOutsideData()
{
	QFETCH(uint32_t, offset);
	QFETCH(uint32_t, size);
	QFETCH(uint32_t, maxSize);

	auto frame = makeFrame(2, 2, 8);
	frame.bufferData.maxSize = maxSize;
	frame.bufferData.chunk->offset = offset;
	frame.bufferData.chunk->size = size;

	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, frame.bufferData).status, PipeWireFrame::Status::ChunkOutsideData);
}



void PipeWireFrameTest::rejectsTruncatedFrames_data()
{
	QTest::addColumn<int32_t>("stride");
	QTest::addColumn<uint32_t>("size");

	QTest::newRow("one byte short") << int32_t(8) << uint32_t(15);
	QTest::newRow("padding of the last row missing") << int32_t(12) << uint32_t(20);
	QTest::newRow("tight stride, half a frame") << int32_t(0) << uint32_t(8);
	QTest::newRow("int32 maximum stride") << std::numeric_limits<int32_t>::max() << uint32_t(64);
}



void PipeWireFrameTest::rejectsTruncatedFrames()
{
	QFETCH(int32_t, stride);
	QFETCH(uint32_t, size);

	auto frame = makeFrame(2, 2, 8, 0, 64);
	frame.bufferData.chunk->stride = stride;
	frame.bufferData.chunk->size = size;

	QCOMPARE(PipeWireFrame::validate(2, 2, BGRx, frame.bufferData).status, PipeWireFrame::Status::Truncated);

	// largest stride with the largest frame: no overflow in stride * height
	frame.bufferData.chunk->stride = std::numeric_limits<int32_t>::max();
	frame.bufferData.chunk->size = uint32_t(frame.data.size());
	QCOMPARE(PipeWireFrame::validate(PipeWireFrame::MaximumDimension, 4096, BGRx, frame.bufferData).status,
			 PipeWireFrame::Status::Truncated);
}



void PipeWireFrameTest::damageWithoutMetadataIsFullFrame()
{
	QVERIFY(PipeWireFrame::parseDamage(nullptr, 0, 100, 100).isFullFrame);
	QVERIFY(PipeWireFrame::parseDamage(nullptr, 256, 100, 100).isFullFrame);

	// shorter than one region
	const auto meta = makeDamage({{{1, 1}, {10, 10}}});
	QVERIFY(PipeWireFrame::parseDamage(meta.data(), sizeof(spa_meta_region) - 1, 100, 100).isFullFrame);
	QVERIFY(PipeWireFrame::parseDamage(meta.data(), 0, 100, 100).isFullFrame);

	// invalid frame size
	QVERIFY(parse(meta, 0, 100).isFullFrame);
	QVERIFY(parse(meta, 100, PipeWireFrame::MaximumDimension + 1).isFullFrame);
}



void PipeWireFrameTest::damageWithoutValidRegionIsFullFrame()
{
	// the first entry terminates the list: no damage information
	QVERIFY(parse(makeDamage({{{0, 0}, {0, 0}}, {{1, 1}, {10, 10}}}), 100, 100).isFullFrame);
	QVERIFY(parse(makeDamage({{{0, 0}, {0, 10}}}), 100, 100).isFullFrame);
	QVERIFY(parse(makeDamage({{{0, 0}, {10, 0}}}), 100, 100).isFullFrame);
}



void PipeWireFrameTest::damageRegionsAreClipped()
{
	const auto damage = parse(makeDamage({
								  {{10, 20}, {30, 40}},
								  {{-10, -20}, {30, 40}},
								  {{90, 95}, {30, 40}},
								  {{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()},
								   {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()}},
								  {{std::numeric_limits<int32_t>::max(), 0}, {std::numeric_limits<uint32_t>::max(), 1}},
							  }),
							  100, 100);

	QCOMPARE(damage.isFullFrame, false);
	QCOMPARE(damage.count, 4);

	QCOMPARE(damage.rects[0].x1, 10);
	QCOMPARE(damage.rects[0].y1, 20);
	QCOMPARE(damage.rects[0].x2, 40);
	QCOMPARE(damage.rects[0].y2, 60);

	QCOMPARE(damage.rects[1].x1, 0);
	QCOMPARE(damage.rects[1].y1, 0);
	QCOMPARE(damage.rects[1].x2, 20);
	QCOMPARE(damage.rects[1].y2, 20);

	QCOMPARE(damage.rects[2].x1, 90);
	QCOMPARE(damage.rects[2].y1, 95);
	QCOMPARE(damage.rects[2].x2, 100);
	QCOMPARE(damage.rects[2].y2, 100);

	// int32 minimum + uint32 maximum does not wrap
	QCOMPARE(damage.rects[3].x1, 0);
	QCOMPARE(damage.rects[3].y1, 0);
	QCOMPARE(damage.rects[3].x2, 100);
	QCOMPARE(damage.rects[3].y2, 100);

	// the fifth region starts beyond the right edge
}



void PipeWireFrameTest::damageOutsideFrameIsSkipped()
{
	const auto damage = parse(makeDamage({
								  {{100, 0}, {10, 10}},
								  {{0, 100}, {10, 10}},
								  {{-20, 0}, {20, 10}},
								  {{0, -20}, {10, 20}},
								  {{50, 50}, {1, 1}},
							  }),
							  100, 100);

	QCOMPARE(damage.isFullFrame, false);
	QCOMPARE(damage.count, 1);
	QCOMPARE(damage.rects[0].x1, 50);
	QCOMPARE(damage.rects[0].y1, 50);
	QCOMPARE(damage.rects[0].x2, 51);
	QCOMPARE(damage.rects[0].y2, 51);

	// valid regions, but none of them inside the frame: nothing changed
	const auto outside = parse(makeDamage({{{200, 200}, {10, 10}}}), 100, 100);
	QCOMPARE(outside.isFullFrame, false);
	QCOMPARE(outside.count, 0);
}



void PipeWireFrameTest::damageStopsAtTerminator()
{
	const auto damage = parse(makeDamage({
								  {{1, 1}, {1, 1}},
								  {{2, 2}, {1, 1}},
								  {{0, 0}, {0, 0}},
								  {{3, 3}, {1, 1}}, // stale entry of an earlier frame
							  }),
							  100, 100);

	QCOMPARE(damage.isFullFrame, false);
	QCOMPARE(damage.count, 2);
	QCOMPARE(damage.rects[1].x1, 2);
}



void PipeWireFrameTest::damageIsBoundedByMetadataSize()
{
	// all entries valid, no terminator: the old loop read beyond the metadata
	auto meta = makeDamage({{{1, 1}, {1, 1}}, {{2, 2}, {1, 1}}, {{3, 3}, {1, 1}}});

	const auto twoEntries = PipeWireFrame::parseDamage(meta.data(), 2 * sizeof(spa_meta_region), 100, 100);
	QCOMPARE(twoEntries.isFullFrame, false);
	QCOMPARE(twoEntries.count, 2);

	// a trailing partial entry is ignored
	const auto partial = PipeWireFrame::parseDamage(meta.data(), 3 * sizeof(spa_meta_region) - 1, 100, 100);
	QCOMPARE(partial.isFullFrame, false);
	QCOMPARE(partial.count, 2);

	// exactly the metadata size, read under ASan with a heap block of that size
	meta.resize(sizeof(spa_meta_region));
	meta.shrink_to_fit();
	const auto single = parse(meta, 100, 100);
	QCOMPARE(single.isFullFrame, false);
	QCOMPARE(single.count, 1);
}



void PipeWireFrameTest::damageIsBoundedByRegionLimit()
{
	std::vector<spa_region> regions;
	for (int i = 0; i < PipeWireFrame::MaximumDamageRegions; ++i)
	{
		regions.push_back({{i, i}, {1, 1}});
	}

	// exactly the limit
	auto damage = parse(makeDamage(regions), 100, 100);
	QCOMPARE(damage.isFullFrame, false);
	QCOMPARE(damage.count, PipeWireFrame::MaximumDamageRegions);

	// the limit followed by a terminator
	auto terminated = regions;
	terminated.push_back({{0, 0}, {0, 0}});
	terminated.push_back({{50, 50}, {1, 1}});
	damage = parse(makeDamage(terminated), 100, 100);
	QCOMPARE(damage.isFullFrame, false);
	QCOMPARE(damage.count, PipeWireFrame::MaximumDamageRegions);

	// more regions than requested: unknown damage
	auto tooMany = regions;
	tooMany.push_back({{50, 50}, {1, 1}});
	QVERIFY(parse(makeDamage(tooMany), 100, 100).isFullFrame);

	// more regions than requested, even if all of the first ones are outside the frame
	for (auto& region : tooMany)
	{
		region.position = {500, 500};
	}
	QVERIFY(parse(makeDamage(tooMany), 100, 100).isFullFrame);

	// a large metadata block is not read beyond the limit plus one
	std::vector<spa_region> many(1000, spa_region{{1, 1}, {1, 1}});
	QVERIFY(parse(makeDamage(many), 100, 100).isFullFrame);
}



void PipeWireFrameTest::damageFromUnalignedMetadata()
{
	const auto meta = makeDamage({{{1, 2}, {3, 4}}});
	std::vector<uchar> unaligned(meta.size() + 1);
	std::memcpy(unaligned.data() + 1, meta.data(), meta.size());

	const auto damage = PipeWireFrame::parseDamage(unaligned.data() + 1, uint32_t(meta.size()), 100, 100);
	QCOMPARE(damage.isFullFrame, false);
	QCOMPARE(damage.count, 1);
	QCOMPARE(damage.rects[0].x1, 1);
	QCOMPARE(damage.rects[0].y1, 2);
	QCOMPARE(damage.rects[0].x2, 4);
	QCOMPARE(damage.rects[0].y2, 6);
}



void PipeWireFrameTest::copyConvertsFullFrame()
{
	static constexpr int Width = 5;
	static constexpr int Height = 3;
	static constexpr int SourceStride = 24;
	static constexpr int TargetStride = 28;
	static constexpr uint32_t Offset = 7;

	const auto frame = makeFrame(Width, Height, SourceStride, Offset);
	const auto source = PipeWireFrame::validate(Width, Height, BGRx, frame.bufferData);
	QCOMPARE(source.status, PipeWireFrame::Status::Valid);

	// target without padding after the last row
	std::vector<uchar> target(TargetStride * (Height - 1) + Width * 4, Untouched);
	const PipeWireFrame::Target targetInfo{target.data(), TargetStride, qint64(target.size())};

	QVERIFY(PipeWireFrame::copy(source, Width, Height, targetInfo, {}, false));

	for (int y = 0; y < Height; ++y)
	{
		for (int x = 0; x < Width; ++x)
		{
			QVERIFY(isConverted(frame, target, TargetStride, x, y, SourceStride, Offset));
		}
		// row padding of the target is not written
		if (y < Height - 1)
		{
			QCOMPARE(target[size_t(y * TargetStride + Width * 4)], Untouched);
		}
	}

	// red and blue swapped, green and the x byte kept
	const auto* firstPixel = target.data() + TargetStride + 2 * 4;
	QCOMPARE(firstPixel[0], uchar(0x80 | (1 * Width + 2)));
	QCOMPARE(firstPixel[1], uchar(1));
	QCOMPARE(firstPixel[2], uchar(2));
	QCOMPARE(firstPixel[3], uchar(0xff));

	// forced full frame despite small damage
	std::fill(target.begin(), target.end(), Untouched);
	const auto damage = parse(makeDamage({{{0, 0}, {1, 1}}}), Width, Height);
	QCOMPARE(damage.isFullFrame, false);
	QVERIFY(PipeWireFrame::copy(source, Width, Height, targetInfo, damage, true));
	QVERIFY(isConverted(frame, target, TargetStride, Width - 1, Height - 1, SourceStride, Offset));
}



void PipeWireFrameTest::copyOnlyDamagedRects()
{
	static constexpr int Width = 10;
	static constexpr int Height = 10;
	static constexpr int Stride = Width * 4;

	const auto frame = makeFrame(Width, Height, Stride);
	const auto source = PipeWireFrame::validate(Width, Height, BGRx, frame.bufferData);
	std::vector<uchar> target(Stride * Height, Untouched);

	const auto damage = parse(makeDamage({{{1, 1}, {2, 2}}, {{8, 7}, {5, 5}}}), Width, Height);
	QCOMPARE(damage.count, 2);

	QVERIFY(PipeWireFrame::copy(source, Width, Height, {target.data(), Stride, qint64(target.size())}, damage, false));

	const auto isDamaged = [](int x, int y) {
		return (x >= 1 && x < 3 && y >= 1 && y < 3) || (x >= 8 && y >= 7);
	};

	for (int y = 0; y < Height; ++y)
	{
		for (int x = 0; x < Width; ++x)
		{
			if (isDamaged(x, y))
			{
				QVERIFY(isConverted(frame, target, Stride, x, y, Stride));
			}
			else
			{
				QCOMPARE(target[size_t(y * Stride + x * 4)], Untouched);
			}
		}
	}
}



void PipeWireFrameTest::copyFallsBackToFullFrameForLargeDamage()
{
	static constexpr int Width = 10;
	static constexpr int Height = 10;
	static constexpr int Stride = Width * 4;

	const auto frame = makeFrame(Width, Height, Stride);
	const auto source = PipeWireFrame::validate(Width, Height, BGRx, frame.bufferData);
	std::vector<uchar> target(Stride * Height, Untouched);
	const PipeWireFrame::Target targetInfo{target.data(), Stride, qint64(target.size())};

	// exactly 60%: still rect by rect
	QVERIFY(PipeWireFrame::copy(source, Width, Height, targetInfo,
								parse(makeDamage({{{0, 0}, {10, 6}}}), Width, Height), false));
	QCOMPARE(target[size_t(9 * Stride)], Untouched);

	// 61%, also with overlapping rects: whole frame
	QVERIFY(PipeWireFrame::copy(source, Width, Height, targetInfo,
								parse(makeDamage({{{0, 0}, {10, 6}}, {{0, 0}, {1, 1}}}), Width, Height), false));
	QVERIFY(isConverted(frame, target, Stride, 9, 9, Stride));
}



void PipeWireFrameTest::copyWithClippedAwayDamageCopiesNothing()
{
	const auto frame = makeFrame(4, 4, 16);
	const auto source = PipeWireFrame::validate(4, 4, BGRx, frame.bufferData);
	std::vector<uchar> target(16 * 4, Untouched);

	const auto damage = parse(makeDamage({{{10, 10}, {5, 5}}}), 4, 4);
	QVERIFY(PipeWireFrame::copy(source, 4, 4, {target.data(), 16, qint64(target.size())}, damage, false));
	QVERIFY(std::all_of(target.cbegin(), target.cend(), [](uchar value) { return value == Untouched; }));
}



void PipeWireFrameTest::copyRejectsInvalidSource()
{
	auto frame = makeFrame(4, 4, 16);
	const auto valid = PipeWireFrame::validate(4, 4, BGRx, frame.bufferData);
	std::vector<uchar> target(16 * 4, Untouched);
	const PipeWireFrame::Target targetInfo{target.data(), 16, qint64(target.size())};

	frame.bufferData.chunk->size = 15;
	const auto truncated = PipeWireFrame::validate(4, 4, BGRx, frame.bufferData);
	QCOMPARE(truncated.status, PipeWireFrame::Status::Truncated);
	QCOMPARE(PipeWireFrame::copy(truncated, 4, 4, targetInfo, {}, true), false);

	// Valid status, but values the validation never produces
	auto withoutPixels = valid;
	withoutPixels.pixels = nullptr;
	QCOMPARE(PipeWireFrame::copy(withoutPixels, 4, 4, targetInfo, {}, true), false);

	auto shortStride = valid;
	shortStride.stride = 15;
	QCOMPARE(PipeWireFrame::copy(shortStride, 4, 4, targetInfo, {}, true), false);

	QCOMPARE(PipeWireFrame::copy(valid, 0, 4, targetInfo, {}, true), false);
	QCOMPARE(PipeWireFrame::copy(valid, 4, PipeWireFrame::MaximumDimension + 1, targetInfo, {}, true), false);

	QVERIFY(std::all_of(target.cbegin(), target.cend(), [](uchar value) { return value == Untouched; }));
}



void PipeWireFrameTest::copyRejectsInvalidTarget()
{
	const auto frame = makeFrame(4, 4, 16);
	const auto source = PipeWireFrame::validate(4, 4, BGRx, frame.bufferData);
	std::vector<uchar> target(16 * 4, Untouched);

	QCOMPARE(PipeWireFrame::copy(source, 4, 4, {nullptr, 16, qint64(target.size())}, {}, true), false);
	QCOMPARE(PipeWireFrame::copy(source, 4, 4, {target.data(), 15, qint64(target.size())}, {}, true), false);
	QCOMPARE(PipeWireFrame::copy(source, 4, 4, {target.data(), -16, qint64(target.size())}, {}, true), false);
	QCOMPARE(PipeWireFrame::copy(source, 4, 4, {target.data(), 16, qint64(target.size()) - 1}, {}, true), false);

	// a framebuffer of a different (smaller) size than the stream
	QCOMPARE(PipeWireFrame::copy(source, 4, 4, {target.data(), 16, 16 * 3}, {}, true), false);

	QVERIFY(std::all_of(target.cbegin(), target.cend(), [](uchar value) { return value == Untouched; }));
}



void PipeWireFrameTest::copyRejectsInvalidDamage_data()
{
	QTest::addColumn<int>("count");
	QTest::addColumn<int>("x1");
	QTest::addColumn<int>("y1");
	QTest::addColumn<int>("x2");
	QTest::addColumn<int>("y2");

	QTest::newRow("negative count") << -1 << 0 << 0 << 1 << 1;
	QTest::newRow("count above limit") << PipeWireFrame::MaximumDamageRegions + 1 << 0 << 0 << 1 << 1;
	QTest::newRow("negative x1") << 1 << -1 << 0 << 1 << 1;
	QTest::newRow("negative y1") << 1 << 0 << -1 << 1 << 1;
	QTest::newRow("empty width") << 1 << 1 << 0 << 1 << 1;
	QTest::newRow("empty height") << 1 << 0 << 1 << 1 << 1;
	QTest::newRow("x2 beyond width") << 1 << 0 << 0 << 5 << 1;
	QTest::newRow("y2 beyond height") << 1 << 0 << 0 << 1 << 5;
}



void PipeWireFrameTest::copyRejectsInvalidDamage()
{
	QFETCH(int, count);
	QFETCH(int, x1);
	QFETCH(int, y1);
	QFETCH(int, x2);
	QFETCH(int, y2);

	const auto frame = makeFrame(4, 4, 16);
	const auto source = PipeWireFrame::validate(4, 4, BGRx, frame.bufferData);
	std::vector<uchar> target(16 * 4, Untouched);

	// not produced by parseDamage()
	PipeWireFrame::Damage damage;
	damage.isFullFrame = false;
	damage.count = count;
	damage.rects[0] = {x1, y1, x2, y2};
	for (int i = 1; i < PipeWireFrame::MaximumDamageRegions; ++i)
	{
		damage.rects[size_t(i)] = {0, 0, 1, 1};
	}

	QCOMPARE(PipeWireFrame::copy(source, 4, 4, {target.data(), 16, qint64(target.size())}, damage, false), false);
	QVERIFY(std::all_of(target.cbegin(), target.cend(), [](uchar value) { return value == Untouched; }));
}



void PipeWireFrameTest::statusNames()
{
	using Status = PipeWireFrame::Status;

	const auto statuses = {Status::Valid, Status::NoContent, Status::InvalidSize, Status::UnsupportedFormat,
						   Status::UnsupportedDataType, Status::NotMapped, Status::MissingChunk,
						   Status::InvalidStride, Status::ChunkOutsideData, Status::Truncated};

	QStringList names;
	for (const auto status : statuses)
	{
		const auto name = QString::fromLatin1(PipeWireFrame::statusName(status));
		QVERIFY(name.isEmpty() == false);
		QVERIFY(name != QLatin1String("unknown"));
		names.append(name);
	}
	names.removeDuplicates();
	QCOMPARE(names.size(), int(statuses.size()));

	QCOMPARE(PipeWireFrame::statusName(static_cast<Status>(1000)), "unknown");
}


QTEST_GUILESS_MAIN(PipeWireFrameTest)
#include "PipeWireFrameTest.moc"
