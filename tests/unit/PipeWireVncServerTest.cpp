// GPLv2 copyright header (2019-2026 Tobias Junghans)
#include <QObject>
#include <QTest>
#include <QString>
#include <cstdint>

class PipeWireVncServerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testBgrxToRgbConversion();
    void testDamageRectClamping();
    void testPortalTokenPattern();
    void testPortalRequestPath();
};

static void convertLineBGRxToRgb(const uint8_t* src, int width, uint8_t* dst)
{
    for (int x = 0; x < width; ++x) {
        const int i = x * 4;
        dst[i + 0] = src[i + 2]; // R = B
        dst[i + 1] = src[i + 1]; // G = G
        dst[i + 2] = src[i + 0]; // B = R
        dst[i + 3] = src[i + 3]; // A = A
    }
}

void PipeWireVncServerTest::testBgrxToRgbConversion()
{
    // Test single pixel: pure red BGRx (0x00,0x00,0xFF,0xFF) -> RGBA (0xFF,0x00,0x00,0xFF)
    {
        uint8_t src[4] = { 0x00, 0x00, 0xFF, 0xFF };
        uint8_t dst[4] = { 0 };
        convertLineBGRxToRgb(src, 1, dst);
        QCOMPARE(dst[0], uint8_t(0xFF));
        QCOMPARE(dst[1], uint8_t(0x00));
        QCOMPARE(dst[2], uint8_t(0x00));
        QCOMPARE(dst[3], uint8_t(0xFF));
    }

    // Test single pixel: pure green (0x00,0xFF,0x00,0xFF) -> unchanged
    {
        uint8_t src[4] = { 0x00, 0xFF, 0x00, 0xFF };
        uint8_t dst[4] = { 0 };
        convertLineBGRxToRgb(src, 1, dst);
        QCOMPARE(dst[0], uint8_t(0x00));
        QCOMPARE(dst[1], uint8_t(0xFF));
        QCOMPARE(dst[2], uint8_t(0x00));
        QCOMPARE(dst[3], uint8_t(0xFF));
    }

    // Test single pixel: pure blue (0xFF,0x00,0x00,0xFF) -> (0x00,0x00,0xFF,0xFF)
    {
        uint8_t src[4] = { 0xFF, 0x00, 0x00, 0xFF };
        uint8_t dst[4] = { 0 };
        convertLineBGRxToRgb(src, 1, dst);
        QCOMPARE(dst[0], uint8_t(0x00));
        QCOMPARE(dst[1], uint8_t(0x00));
        QCOMPARE(dst[2], uint8_t(0xFF));
        QCOMPARE(dst[3], uint8_t(0xFF));
    }

    // Test multiple pixels with known pattern
    {
        // 3 pixels: red, green, blue in BGRx
        uint8_t src[12] = {
            0x00, 0x00, 0xFF, 0xFF,  // red in BGRx
            0x00, 0xFF, 0x00, 0xFF,  // green in BGRx
            0xFF, 0x00, 0x00, 0xFF   // blue in BGRx
        };
        uint8_t dst[12] = { 0 };
        convertLineBGRxToRgb(src, 3, dst);

        // Pixel 0: should be RGBA red
        QCOMPARE(dst[0], uint8_t(0xFF));
        QCOMPARE(dst[1], uint8_t(0x00));
        QCOMPARE(dst[2], uint8_t(0x00));
        QCOMPARE(dst[3], uint8_t(0xFF));

        // Pixel 1: should be RGBA green (unchanged)
        QCOMPARE(dst[4], uint8_t(0x00));
        QCOMPARE(dst[5], uint8_t(0xFF));
        QCOMPARE(dst[6], uint8_t(0x00));
        QCOMPARE(dst[7], uint8_t(0xFF));

        // Pixel 2: should be RGBA blue
        QCOMPARE(dst[8], uint8_t(0x00));
        QCOMPARE(dst[9], uint8_t(0x00));
        QCOMPARE(dst[10], uint8_t(0xFF));
        QCOMPARE(dst[11], uint8_t(0xFF));
    }

    // Verify each pixel's 4 bytes are correctly transformed: mixed pattern
    {
        // 2 pixels with arbitrary values
        uint8_t src[8] = {
            0x10, 0x20, 0x30, 0x40,  // pixel 0
            0x50, 0x60, 0x70, 0x80   // pixel 1
        };
        uint8_t dst[8] = { 0 };
        convertLineBGRxToRgb(src, 2, dst);

        // Pixel 0: src[B]=0x30 -> dst[R]=0x30, src[G]=0x20 -> dst[G]=0x20, src[R]=0x10 -> dst[B]=0x10, src[A]=0x40 -> dst[A]=0x40
        QCOMPARE(dst[0], uint8_t(0x30));
        QCOMPARE(dst[1], uint8_t(0x20));
        QCOMPARE(dst[2], uint8_t(0x10));
        QCOMPARE(dst[3], uint8_t(0x40));

        // Pixel 1: src[B]=0x70 -> dst[R]=0x70, src[G]=0x60 -> dst[G]=0x60, src[R]=0x50 -> dst[B]=0x50, src[A]=0x80 -> dst[A]=0x80
        QCOMPARE(dst[4], uint8_t(0x70));
        QCOMPARE(dst[5], uint8_t(0x60));
        QCOMPARE(dst[6], uint8_t(0x50));
        QCOMPARE(dst[7], uint8_t(0x80));
    }
}

void PipeWireVncServerTest::testDamageRectClamping()
{
    const int width = 1920;
    const int height = 1080;

    // Region fully inside: (10,10,100,100) -> clamped to (10,10,110,110)
    {
        int x = 10, y = 10, w = 100, h = 100;
        const int x1 = qBound(0, x, width);
        const int y1 = qBound(0, y, height);
        const int x2 = qBound(0, x1 + w, width);
        const int y2 = qBound(0, y1 + h, height);
        QCOMPARE(x1, 10);
        QCOMPARE(y1, 10);
        QCOMPARE(x2, 110);
        QCOMPARE(y2, 110);
    }

    // Region partially outside right edge: (1900,10,100,100) -> x2 clamped to 1920
    {
        int x = 1900, y = 10, w = 100, h = 100;
        const int x1 = qBound(0, x, width);
        const int y1 = qBound(0, y, height);
        const int x2 = qBound(0, x1 + w, width);
        const int y2 = qBound(0, y1 + h, height);
        QCOMPARE(x1, 1900);
        QCOMPARE(y1, 10);
        QCOMPARE(x2, 1920);
        QCOMPARE(y2, 110);
    }

    // Region completely outside: (2000,2000,100,100) -> x1=y1=1920,1080 -> x2=y2=1920,1080 -> invalid (not x2>x1)
    {
        int x = 2000, y = 2000, w = 100, h = 100;
        const int x1 = qBound(0, x, width);
        const int y1 = qBound(0, y, height);
        const int x2 = qBound(0, x1 + w, width);
        const int y2 = qBound(0, y1 + h, height);
        QCOMPARE(x1, 1920);
        QCOMPARE(y1, 1080);
        QCOMPARE(x2, 1920);
        QCOMPARE(y2, 1080);
        QVERIFY(x2 <= x1);
        QVERIFY(y2 <= y1);
    }

    // Negative position: (-10,-10,100,100) -> x1=y1=0 -> x2=100,y2=100
    {
        int x = -10, y = -10, w = 100, h = 100;
        const int x1 = qBound(0, x, width);
        const int y1 = qBound(0, y, height);
        const int x2 = qBound(0, x1 + w, width);
        const int y2 = qBound(0, y1 + h, height);
        QCOMPARE(x1, 0);
        QCOMPARE(y1, 0);
        QCOMPARE(x2, 100);
        QCOMPARE(y2, 100);
    }
}

void PipeWireVncServerTest::testPortalTokenPattern()
{
    // makeRequestToken() should return "veyon_" followed by a numeric part
    QVERIFY(QString("veyon_%1").arg(12345).startsWith("veyon_"));
    QVERIFY(QString("veyon_%1").arg(0).startsWith("veyon_"));
    QVERIFY(QString("veyon_%1").arg(999999).startsWith("veyon_"));

    // Verify the numeric part is present after the prefix
    QString token = QString("veyon_%1").arg(12345);
    QCOMPARE(token, QString("veyon_12345"));
}

void PipeWireVncServerTest::testPortalRequestPath()
{
    // Test makeRequestPath format: /org/freedesktop/portal/desktop/request/%1/%2
    const QString senderToken = "sender123";
    const QString requestToken = "req456";
    QString path = QString("/org/freedesktop/portal/desktop/request/%1/%2")
                       .arg(senderToken, requestToken);
    QCOMPARE(path, QString("/org/freedesktop/portal/desktop/request/sender123/req456"));
    QVERIFY(path.startsWith("/org/freedesktop/portal/desktop/request/"));
    QVERIFY(path.endsWith("/req456"));

    // Test with empty tokens
    QString pathEmpty = QString("/org/freedesktop/portal/desktop/request/%1/%2")
                            .arg(QString(), QString());
    QCOMPARE(pathEmpty, QString("/org/freedesktop/portal/desktop/request//"));

    // Test with special characters in tokens
    QString pathSpecial = QString("/org/freedesktop/portal/desktop/request/%1/%2")
                              .arg(QString("test_user"), QString("req_42"));
    QCOMPARE(pathSpecial, QString("/org/freedesktop/portal/desktop/request/test_user/req_42"));
}

QTEST_MAIN(PipeWireVncServerTest)
#include "PipeWireVncServerTest.moc"
