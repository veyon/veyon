/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "QuickUi.h"
#include "DesktopIcons.h"
#include "PlatformUserFunctions.h"
#include "QuickFramebufferItem.h"
#include "VeyonConfiguration.h"
#include <QDesktopServices>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPainter>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickStyle>
#include <QTimer>
#include <QTextDocument>
#include <QUrl>
#include <QUrlQuery>
#include <QWindow>
#include <memory>

namespace
{
class IconProvider final : public QQuickImageProvider
{
public:
	IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
	QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requested) override
	{
		const auto url = QUrl(QStringLiteral("image://icons/") + id);
		const auto extent = requested.isValid() ? requested : QSize(16, 16);
		auto result = DesktopIcons::icon(url.path().mid(1)).pixmap(extent);
		const QColor color(QUrlQuery(url).queryItemValue(QStringLiteral("color")));
		if (color.isValid())
		{
			QPainter painter(&result);
			painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
			painter.fillRect(result.rect(), color);
		}
		if (size) *size = result.size();
		return result;
	}
};
}

QuickUi::QuickUi(QObject* parent) : QObject(parent)
{
	connect(&VeyonCore::config(), &VeyonConfiguration::configurationChanged, this, &QuickUi::themeChanged);
}

bool QuickUi::dark() const { return VeyonCore::useDarkMode(); }

QQmlApplicationEngine* QuickUi::createEngine(QObject* parent)
{
	static const bool registered = []() {
		QQuickStyle::setStyle(QStringLiteral("Basic"));
		qmlRegisterType<QuickFramebufferItem>("Veyon", 1, 0, "Framebuffer");
		return true;
	}();
	Q_UNUSED(registered)
	auto engine = new QQmlApplicationEngine(parent);
	engine->addImageProvider(QStringLiteral("icons"), new IconProvider);
	engine->rootContext()->setContextProperty(QStringLiteral("veyonUi"), new QuickUi(engine));
	return engine;
}

QVariantMap QuickUi::execDialog(const QString& kind, const QVariantMap& data, QWindow* parent)
{
	const bool quitOnLastWindow = QGuiApplication::quitOnLastWindowClosed();
	QGuiApplication::setQuitOnLastWindowClosed(false);
	std::unique_ptr<QQmlApplicationEngine> engine(createEngine());
	auto request = new QuickDialogRequest(kind, data, engine.get());
	engine->rootContext()->setContextProperty(QStringLiteral("dialogRequest"), request);
	QEventLoop loop;
	QObject::connect(request, &QuickDialogRequest::completed, &loop, &QEventLoop::quit);
	engine->load(QUrl(QStringLiteral("qrc:/quick/GenericDialog.qml")));
	if (engine->rootObjects().isEmpty())
	{
		engine.reset();
		QGuiApplication::setQuitOnLastWindowClosed(quitOnLastWindow);
		return {{QStringLiteral("accepted"), false}, {QStringLiteral("error"), tr("Could not load the dialog.")}};
	}
	if (auto window = qobject_cast<QWindow*>(engine->rootObjects().first()))
	{
		const bool visible = window->isVisible();
		window->hide();
		window->setTransientParent(parent);
		window->setModality(Qt::ApplicationModal);
		if (visible) { window->show(); window->raise(); window->requestActivate(); }
	}
	if (!request->finished()) loop.exec();
	const auto result = request->result();
	engine.reset();
	QGuiApplication::setQuitOnLastWindowClosed(quitOnLastWindow);
	return result;
}

void QuickUi::showError(const QString& title, const QString& text, QWindow* parent)
{
	execDialog(QStringLiteral("error"), {{QStringLiteral("title"), title}, {QStringLiteral("text"), text}}, parent);
}

void QuickUi::showNotification(const QString& title, const QString& text)
{
	auto engine = createEngine(qApp);
	auto request = new QuickDialogRequest(QStringLiteral("notification"),
		{{QStringLiteral("title"), title}, {QStringLiteral("text"), text}}, engine);
	engine->rootContext()->setContextProperty(QStringLiteral("dialogRequest"), request);
	connect(request, &QuickDialogRequest::completed, engine, &QObject::deleteLater);
	if (VeyonCore::component() == VeyonCore::Component::Worker)
		connect(request, &QuickDialogRequest::completed, qApp, &QCoreApplication::quit);
	engine->load(QUrl(QStringLiteral("qrc:/quick/GenericDialog.qml")));
	if (engine->rootObjects().isEmpty()) engine->deleteLater();
}

void QuickUi::openUrl(const QUrl& url) { QDesktopServices::openUrl(url); }
QString QuickUi::localPath(const QUrl& url) const { return url.isLocalFile() ? url.toLocalFile() : url.toString(); }
QUrl QuickUi::fileUrl(const QString& path) const { const QUrl url(path); return url.isLocalFile() ? url : QUrl::fromLocalFile(path); }

QuickDialogRequest::QuickDialogRequest(QString kind, QVariantMap data, QObject* parent)
	: QObject(parent), m_kind(std::move(kind)), m_data(std::move(data)) {}

void QuickDialogRequest::accept(const QVariantMap& result)
{
	if (m_finished) return;
	auto submitted = result;
	if (m_kind == QStringLiteral("website"))
	{
		const auto address = result.value(QStringLiteral("value")).toString().trimmed();
		QUrl url(address, QUrl::TolerantMode);
		if (url.scheme().isEmpty()) url = QUrl(QStringLiteral("http://") + address, QUrl::TolerantMode);
		const bool webAddress = url.scheme().compare(QStringLiteral("http"), Qt::CaseInsensitive) == 0 ||
			url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
		if (address.isEmpty() || !url.isValid() || (webAddress && url.host().isEmpty()))
		{
			m_error = tr("Enter a valid website address.");
			Q_EMIT errorChanged();
			return;
		}
		submitted.insert(QStringLiteral("value"), address);
	}
	if ((m_kind == QStringLiteral("website") || m_kind == QStringLiteral("application")) &&
		result.value(QStringLiteral("remember")).toBool() && result.value(QStringLiteral("presetName")).toString().trimmed().isEmpty())
	{
		m_error = tr("Enter a name for the saved entry.");
		Q_EMIT errorChanged();
		return;
	}
	if (m_kind == QStringLiteral("message"))
	{
		QTextDocument document;
		const auto text = result.value(QStringLiteral("value")).toString();
		if (result.value(QStringLiteral("richText")).toBool()) document.setHtml(text);
		else document.setPlainText(text);
		if (document.toPlainText().trimmed().isEmpty())
		{
			m_error = tr("Enter a message to send.");
			Q_EMIT errorChanged();
			return;
		}
		submitted.insert(QStringLiteral("value"), document.toHtml());
		submitted.insert(QStringLiteral("richText"), true);
	}
	if (m_kind == QStringLiteral("authenticate") &&
		!VeyonCore::platform().userFunctions().authenticate(result.value(QStringLiteral("username")).toString(),
			result.value(QStringLiteral("password")).toString().toUtf8()))
	{
		m_error = tr("Logon failed with given username and password. Please try again!");
		Q_EMIT errorChanged();
		return;
	}
	if (!m_error.isEmpty())
	{
		m_error.clear();
		Q_EMIT errorChanged();
	}
	m_result = submitted;
	m_result.insert(QStringLiteral("accepted"), true);
	m_finished = true;
	Q_EMIT completed();
}

void QuickDialogRequest::reject()
{
	if (m_finished) return;
	m_result = {{QStringLiteral("accepted"), false}};
	m_finished = true;
	Q_EMIT completed();
}
