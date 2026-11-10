/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "PipeWireFrame.h"
#include <QtTest>

class PipeWireFrameTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void acceptsPaddedRowsAndNonzeroOffset()
	{
		QVERIFY(PipeWireFrame::fits(2, 2, 12, 4, 24, 28));
		QVERIFY(PipeWireFrame::fits(2, 2, 0, 0, 16, 16));
	}
	void rejectsTruncatedOrOverlappingRows()
	{
		QVERIFY(!PipeWireFrame::fits(2, 2, 12, 4, 23, 28));
		QVERIFY(!PipeWireFrame::fits(2, 2, 4, 0, 16, 16));
		QVERIFY(!PipeWireFrame::fits(2, 2, -8, 0, 16, 16));
	}
	void rejectsChunksOutsideMappedMemory()
	{
		QVERIFY(!PipeWireFrame::fits(2, 2, 8, 4, 16, 16));
		QVERIFY(!PipeWireFrame::fits(2, 2, 8, uint32_t(-1), 16, 16));
		QVERIFY(!PipeWireFrame::fits(2, 2, 8, 0, uint32_t(-1), 16));
	}
	void rejectsOversizedImagesBeforeAllocation()
	{
		QVERIFY(!PipeWireFrame::fits(0, 1, 4, 0, 4, 4));
		QVERIFY(!PipeWireFrame::fits(20000, 1, 80000, 0, 80000, 80000));
		QVERIFY(!PipeWireFrame::fits(16384, 16384, 65536, 0, 1073741824, 1073741824));
	}
};

QTEST_GUILESS_MAIN(PipeWireFrameTest)
#include "PipeWireFrameTest.moc"
