// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

// DesktopServicesFeaturePlugin: the worker opens every website the master sends

#include <QBuffer>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

#include "DesktopServicesFeaturePlugin.h"
#include "FeatureMessage.h"
#include "VeyonCore.h"
#include "VeyonWorkerInterface.h"

#include <memory>


// receives the URLs passed to QDesktopServices::openUrl() so no browser is started
class UrlRecorder : public QObject
{
	Q_OBJECT
public:
	QList<QUrl> urls;

public Q_SLOTS:
	void record(const QUrl& url)
	{
		urls.append(url);
	}
};



class TestWorker : public VeyonWorkerInterface
{
public:
	bool sendFeatureMessageReply(const FeatureMessage& reply) override
	{
		Q_UNUSED(reply)
		return true;
	}
};



class DesktopServicesFeatureTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_home.isValid());
		qputenv("HOME", m_home.path().toUtf8());
		qputenv("XDG_CONFIG_HOME", m_home.filePath(QStringLiteral(".config")).toUtf8());

		m_core = std::make_unique<VeyonCore>(QCoreApplication::instance(), VeyonCore::Component::Worker,
											 QStringLiteral("DesktopServicesFeatureTest"));
	}

	void cleanupTestCase()
	{
		m_core.reset();
	}

	void workerOpensAllWebsites_data()
	{
		QTest::addColumn<QVariant>("websiteUrls");
		QTest::addColumn<QList<QUrl>>("expectedUrls");

		const auto first = QStringLiteral("http://127.0.0.1:18766/?first");
		const auto second = QStringLiteral("http://127.0.0.1:18766/?second");

		// sent by the master's dialog and controlFeature()
		QTest::newRow("single-url-list") << QVariant(QStringList{first}) << QList<QUrl>{QUrl(first)};
		// sent for predefined websites
		QTest::newRow("single-url-string") << QVariant(first) << QList<QUrl>{QUrl(first)};
		// e.g. veyon-cli feature start … OpenWebsite '{"websiteUrls":[…]}'
		QTest::newRow("multiple-urls") << QVariant(QStringList{first, second})
									   << QList<QUrl>{QUrl(first), QUrl(second)};
	}

	void workerOpensAllWebsites()
	{
		QFETCH(QVariant, websiteUrls);
		QFETCH(QList<QUrl>, expectedUrls);

		DesktopServicesFeaturePlugin plugin;
		Feature::Uid openWebsiteFeatureUid;
		for (const auto& feature : plugin.featureList())
		{
			if (feature.name() == QLatin1String("OpenWebsite"))
			{
				openWebsiteFeatureUid = feature.uid();
			}
		}
		QVERIFY(openWebsiteFeatureUid.isNull() == false);

		// the message as the worker receives it, i.e. after serialization
		FeatureMessage outgoingMessage(openWebsiteFeatureUid, FeatureMessage::Command::Default);
		outgoingMessage.addArgument(DesktopServicesFeaturePlugin::Argument::WebsiteUrls, websiteUrls);
		QBuffer transport;
		QVERIFY(transport.open(QBuffer::ReadWrite));
		QVERIFY(outgoingMessage.sendPlain(&transport));
		QVERIFY(transport.seek(0));
		FeatureMessage incomingMessage;
		QVERIFY(incomingMessage.isReadyForReceive(&transport));
		QVERIFY(incomingMessage.receive(&transport));
		QCOMPARE(incomingMessage.argument(DesktopServicesFeaturePlugin::Argument::WebsiteUrls), websiteUrls);

		UrlRecorder recorder;
		QDesktopServices::setUrlHandler(QStringLiteral("http"), &recorder, "record");
		const auto unsetUrlHandler = qScopeGuard([]() { QDesktopServices::unsetUrlHandler(QStringLiteral("http")); });

		TestWorker worker;
		QVERIFY(plugin.handleFeatureMessage(worker, incomingMessage));
		QCOMPARE(recorder.urls, expectedUrls);
	}

private:
	QTemporaryDir m_home;
	std::unique_ptr<VeyonCore> m_core;
};



int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();
	QGuiApplication application(argc, argv);
	DesktopServicesFeatureTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "DesktopServicesFeatureTest.moc"
