// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

// mapPointerToPortalStream(): VNC framebuffer positions to the coordinate space of a portal stream

#include <QtTest>

#include "PortalPointerMapping.h"


Q_DECLARE_METATYPE(PortalStreamGeometry)

class PortalPointerMappingTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void mapsPosition_data()
	{
		QTest::addColumn<QPoint>("position");
		QTest::addColumn<QSize>("framebufferSize");
		QTest::addColumn<PortalStreamGeometry>("stream");
		QTest::addColumn<QPointF>("expected");

		const QSize uhd(3840, 2160);
		// KDE Plasma, 4K output at 150 %: logical stream size, buffers in physical pixels
		const PortalStreamGeometry scaled{{}, {2560, 1440}};
		// GNOME/no scaling: stream size equals buffer size
		const PortalStreamGeometry unscaled{{}, uhd};

		QTest::newRow("unscaled origin") << QPoint(0, 0) << uhd << unscaled << QPointF(0, 0);
		QTest::newRow("unscaled center") << QPoint(1920, 1080) << uhd << unscaled << QPointF(1920, 1080);
		QTest::newRow("unscaled last pixel") << QPoint(3839, 2159) << uhd << unscaled << QPointF(3839, 2159);

		QTest::newRow("scaled origin") << QPoint(0, 0) << uhd << scaled << QPointF(0, 0);
		QTest::newRow("scaled center") << QPoint(1920, 1080) << uhd << scaled << QPointF(1280, 720);
		// rejected with "Invalid position" before
		QTest::newRow("scaled beyond logical width") << QPoint(2580, 1500) << uhd << scaled << QPointF(1720, 1000);
		QTest::newRow("scaled last pixel") << QPoint(3839, 2159) << uhd << scaled
										   << QPointF(3839.0 * 2560 / 3840, 2159.0 * 1440 / 2160);

		QTest::newRow("clamped negative") << QPoint(-10, -1) << uhd << scaled << QPointF(0, 0);
		QTest::newRow("clamped beyond framebuffer") << QPoint(5000, 2160) << uhd << scaled
													<< QPointF(3839.0 * 2560 / 3840, 2159.0 * 1440 / 2160);
		QTest::newRow("unscaled clamped") << QPoint(3840, 9999) << uhd << unscaled << QPointF(3839, 2159);

		// old portals don't report a size
		QTest::newRow("missing size") << QPoint(3000, 2000) << uhd << PortalStreamGeometry{} << QPointF(3000, 2000);
		QTest::newRow("zero size") << QPoint(3000, 2000) << uhd << PortalStreamGeometry{{}, {0, 0}}
								   << QPointF(3000, 2000);
		QTest::newRow("missing size clamped") << QPoint(4000, -3) << uhd << PortalStreamGeometry{} << QPointF(3839, 0);

		// positions are relative to the stream, the compositor adds the stream's offset
		QTest::newRow("stream offset not added") << QPoint(1920, 1080) << uhd
												 << PortalStreamGeometry{{2560, 0}, {2560, 1440}} << QPointF(1280, 720);
		QTest::newRow("negative stream offset") << QPoint(0, 0) << uhd
												<< PortalStreamGeometry{{-1920, -1080}, {2560, 1440}} << QPointF(0, 0);

		// e.g. buffers downscaled by the compositor
		QTest::newRow("stream larger than framebuffer") << QPoint(1279, 719) << QSize(1280, 720) << unscaled
														<< QPointF(3837, 2157);
	}

	void mapsPosition()
	{
		QFETCH(QPoint, position);
		QFETCH(QSize, framebufferSize);
		QFETCH(PortalStreamGeometry, stream);
		QFETCH(QPointF, expected);

		const auto mapped = mapPointerToPortalStream(position, framebufferSize, stream);
		QVERIFY(mapped.has_value());
		QCOMPARE(*mapped, expected);
	}

	void framebufferResizeChangesScale()
	{
		const PortalStreamGeometry stream{{}, {2560, 1440}};

		QCOMPARE(mapPointerToPortalStream({1920, 1080}, {3840, 2160}, stream), QPointF(1280, 720));
		// same stream, framebuffer resized at runtime (e.g. new buffer size of the stream)
		QCOMPARE(mapPointerToPortalStream({960, 540}, {1920, 1080}, stream), QPointF(1280, 720));
		QCOMPARE(mapPointerToPortalStream({1920, 1080}, {1920, 1080}, stream),
				 QPointF(1919.0 * 2560 / 1920, 1079.0 * 1440 / 1080));
	}

	void emptyFramebufferIsNotMapped()
	{
		QVERIFY(mapPointerToPortalStream({10, 10}, {}, {{}, {2560, 1440}}).has_value() == false);
		QVERIFY(mapPointerToPortalStream({10, 10}, {0, 1080}, {{}, {2560, 1440}}).has_value() == false);
	}

	// xdg-desktop-portal accepts 0 <= x < width and 0 <= y < height of the stream only
	void resultIsAlwaysInsideStream_data()
	{
		QTest::addColumn<QSize>("framebufferSize");
		QTest::addColumn<QSize>("streamSize");

		QTest::newRow("150 %") << QSize(3840, 2160) << QSize(2560, 1440);
		QTest::newRow("125 %") << QSize(3840, 2160) << QSize(3072, 1728);
		QTest::newRow("175 %") << QSize(3840, 2160) << QSize(2194, 1234);
		QTest::newRow("133 % odd") << QSize(2879, 1799) << QSize(2159, 1349);
		QTest::newRow("200 %") << QSize(2560, 1600) << QSize(1280, 800);
		QTest::newRow("unscaled") << QSize(1920, 1080) << QSize(1920, 1080);
		QTest::newRow("upscaled") << QSize(1366, 768) << QSize(1920, 1080);
		QTest::newRow("tiny") << QSize(3, 2) << QSize(1, 1);
	}

	void resultIsAlwaysInsideStream()
	{
		QFETCH(QSize, framebufferSize);
		QFETCH(QSize, streamSize);
		const PortalStreamGeometry stream{{}, streamSize};

		const auto checkAxis = [&](int extent, auto coordinate) {
			for (int i = -2; i <= extent + 2; ++i)
			{
				const auto mapped = coordinate(mapPointerToPortalStream({i, i}, framebufferSize, stream).value());
				if (mapped < 0 || mapped >= (coordinate(QPointF(streamSize.width(), streamSize.height()))))
				{
					return false;
				}
			}
			return true;
		};

		QVERIFY(checkAxis(framebufferSize.width(), [](QPointF p) { return p.x(); }));
		QVERIFY(checkAxis(framebufferSize.height(), [](QPointF p) { return p.y(); }));

		// monotonic: the pointer never jumps back while moving right
		double previous = -1;
		for (int x = 0; x < framebufferSize.width(); ++x)
		{
			const auto mapped = mapPointerToPortalStream({x, 0}, framebufferSize, stream)->x();
			QVERIFY(mapped >= previous);
			previous = mapped;
		}
	}
};


QTEST_APPLESS_MAIN(PortalPointerMappingTest)
#include "PortalPointerMappingTest.moc"
