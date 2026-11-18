// SPDX-License-Identifier: GPL-2.0-or-later
#include <QBitArray>
#include <QtTest>
#include <QApplication>
#include <QBuffer>
#include <QDesktopServices>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QUrl>
#include <memory>

#include "DesktopServicesFeaturePlugin.h"
#include "FeatureMessage.h"
#include "VeyonConfiguration.h"
#include "VeyonWorkerInterface.h"

class WebsiteUrlRecorder : public QObject
{
	Q_OBJECT
public:
	QList<QUrl> urls;

public Q_SLOTS:
	void record(const QUrl& url) { urls.append(url); }
};

class WebsiteTestWorker : public VeyonWorkerInterface
{
public:
	bool sendFeatureMessageReply(const FeatureMessage&) override { return true; }
};

class DesktopServicesFeatureTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_home.isValid());
		m_core = std::make_unique<VeyonCore>(qApp, VeyonCore::Component::Master,
			QStringLiteral("DesktopServicesFeatureTest"));
		VeyonCore::config().setUserConfigurationDirectory(m_home.path());
	}

	void serializedWebsiteMessageDispatchesUrls_data()
	{
		QTest::addColumn<QVariant>("payload");
		QTest::addColumn<QList<QUrl>>("expectedUrls");
		const QString first = QStringLiteral("http://127.0.0.1:18766/?unit-first");
		const QString second = QStringLiteral("http://127.0.0.1:18766/?unit-second");
		QTest::newRow("singleton-list") << QVariant(QStringList{first})
			<< QList<QUrl>{QUrl(first)};
		QTest::newRow("legacy-string") << QVariant(first)
			<< QList<QUrl>{QUrl(first)};
		QTest::newRow("multiple-urls") << QVariant(QStringList{first, second})
			<< QList<QUrl>{QUrl(first), QUrl(second)};
	}

	void serializedWebsiteMessageDispatchesUrls()
	{
		QFETCH(QVariant, payload);
		QFETCH(QList<QUrl>, expectedUrls);
		DesktopServicesFeaturePlugin plugin;
		Feature::Uid websiteUid;
		for (const auto& feature : plugin.featureList())
			if (feature.name() == QLatin1String("OpenWebsite")) websiteUid = feature.uid();
		QVERIFY(!websiteUid.isNull());

		// Exercise the real feature wire format and worker handler. Only the
		// desktop URL launcher is intercepted, so this never opens a browser.
		FeatureMessage outgoing(websiteUid, FeatureMessage::Command::Default);
		outgoing.addArgument(DesktopServicesFeaturePlugin::Argument::WebsiteUrls, payload);
		QBuffer transport;
		QVERIFY(transport.open(QIODevice::ReadWrite));
		QVERIFY(outgoing.sendPlain(&transport));
		QVERIFY(transport.seek(0));
		FeatureMessage incoming;
		QVERIFY(incoming.isReadyForReceive(&transport));
		QVERIFY(incoming.receive(&transport));
		QCOMPARE(incoming.argument(DesktopServicesFeaturePlugin::Argument::WebsiteUrls), payload);

		WebsiteUrlRecorder recorder;
		QDesktopServices::setUrlHandler(QStringLiteral("http"), &recorder, "record");
		const auto resetHandler = qScopeGuard([]() {
			QDesktopServices::unsetUrlHandler(QStringLiteral("http"));
		});
		WebsiteTestWorker worker;
		QVERIFY(plugin.handleFeatureMessage(worker, incoming));
		QCOMPARE(recorder.urls, expectedUrls);
	}

	void cleanupTestCase() { m_core.reset(); }

private:
	QTemporaryDir m_home;
	std::unique_ptr<VeyonCore> m_core;
};

int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	DesktopServicesFeatureTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "DesktopServicesFeatureTest.moc"
