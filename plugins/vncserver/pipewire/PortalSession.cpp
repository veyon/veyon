// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

#include "PortalSession.h"
#include "VeyonCore.h"

#include <linux/input.h>
#include <limits>
#include <unistd.h>

// XDG Desktop Portal service and interface names
static const auto PortalService = QStringLiteral("org.freedesktop.portal.Desktop");
static const auto PortalObjectPath = QStringLiteral("/org/freedesktop/portal/desktop");
static const auto RemoteDesktopInterface = QStringLiteral("org.freedesktop.portal.RemoteDesktop");
static const auto ScreenCastInterface = QStringLiteral("org.freedesktop.portal.ScreenCast");
static const auto SessionInterface = QStringLiteral("org.freedesktop.portal.Session");
static const auto MutterRemoteDesktopService = QStringLiteral("org.gnome.Mutter.RemoteDesktop");

namespace {

// the portal fails the whole SelectDevices call for an invalid token
constexpr int MaximumRestoreTokenLength = 64;

bool isValidRestoreToken(const QString& token)
{
	// xdg-desktop-portal < 1.22.1 generates UUIDs (QUuid would also accept one in braces), newer
	// versions D-Bus object path elements ([A-Za-z0-9_]) and still accept UUIDs
	static const QRegularExpression pathElement(QRegularExpression::anchoredPattern(QStringLiteral("[A-Za-z0-9_]+")));
	return (token.size() == 36 && QUuid::fromString(token).isNull() == false) ||
		(token.size() <= MaximumRestoreTokenLength && pathElement.match(token).hasMatch());
}

} // namespace



QDBusConnection PortalSession::portalConnection()
{
	// a session bus connection of its own, so that registerHostAppId() surely comes first on it:
	// Qt (< 6.10) may call the portal on the shared connection already while constructing
	// QGuiApplication, e.g. the GNOME platform theme reading the Settings portal, after which the
	// portal has derived the app ID of that connection and refuses Register(); Qt >= 6.10 registers
	// the shared connection itself, but from a queued call which may come after CreateSession
	return QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("veyon-portal-session"));
}



bool PortalSession::registerHostAppId()
{
	const auto appId = QGuiApplication::desktopFileName();
	if (appId.isEmpty())
	{
		m_portalInstance.clear();
		return true;
	}

	// exported on the same object as all portal interfaces; the only call which may start the
	// portal, see portalCall()
	auto call = QDBusMessage::createMethodCall(PortalService, PortalObjectPath,
											   QStringLiteral("org.freedesktop.host.portal.Registry"),
											   QStringLiteral("Register"));
	call << appId << QVariantMap{};

	const auto reply = m_sessionBus.call(call, QDBus::Block, RegistryCallTimeout);
	const auto errorName = reply.errorName();

	// the portal is not (yet) reachable: no other call may come first, so the session must wait
	if (reply.type() == QDBusMessage::ErrorMessage &&
		(errorName == QLatin1String("org.freedesktop.DBus.Error.NoReply") ||
		 errorName == QLatin1String("org.freedesktop.DBus.Error.Timeout") ||
		 errorName == QLatin1String("org.freedesktop.DBus.Error.ServiceUnknown") ||
		 errorName == QLatin1String("org.freedesktop.DBus.Error.NameHasNoOwner")))
	{
		vWarning() << "xdg-desktop-portal not reachable for registering app ID" << appId << ":"
				   << reply.errorMessage();
		return false;
	}

	// the unique name of the portal instance which answered (an error of the bus itself comes
	// from org.freedesktop.DBus)
	m_portalInstance = reply.service().startsWith(QLatin1Char(':')) ? reply.service() : QString();

	if (reply.type() != QDBusMessage::ErrorMessage)
	{
		vDebug() << "registered app ID" << appId << "with xdg-desktop-portal";
	}
	else if (errorName == QLatin1String("org.freedesktop.DBus.Error.UnknownMethod") ||
			 errorName == QLatin1String("org.freedesktop.DBus.Error.UnknownObject") ||
			 errorName == QLatin1String("org.freedesktop.DBus.Error.UnknownInterface"))
	{
		vDebug() << "xdg-desktop-portal < 1.19.4 without host app registry, app ID derived from the systemd scope";
	}
	else if (errorName == QLatin1String("org.freedesktop.portal.Error.Failed") &&
			 reply.errorMessage().contains(QLatin1String("already associated")))
	{
		// by the registration of an earlier start: no other call reaches a portal instance before
		// it, see portalCall()
		vDebug() << "app ID" << appId << "already registered with xdg-desktop-portal";
	}
	else
	{
		// e.g. a sandboxed process: the portal derives the app ID itself
		vWarning() << "could not register app ID" << appId << "with xdg-desktop-portal:" << errorName
				   << reply.errorMessage();
	}

	return true;
}



QDBusMessage PortalSession::portalCall(const QString& path, const QString& interface, const QString& method,
									   const QVariantList& arguments) const
{
	// addressed to the portal instance the app ID has been registered with: a call reaching
	// another (restarted) one first would make it derive the app ID itself, so a call for a gone
	// instance fails instead, and never starts the portal
	auto message = QDBusMessage::createMethodCall(m_portalInstance.isEmpty() ? PortalService : m_portalInstance,
												  path, interface, method);
	message.setArguments(arguments);
	message.setAutoStartService(false);
	return message;
}



PortalSession::PortalSession(bool persistRestoreToken, QObject* parent) :
	QObject(parent), m_sessionBus(portalConnection()), m_persistRestoreToken(persistRestoreToken)
{
	// the portal is gone, does not respond or input can't keep up: release the session and let
	// the owner restart it like after any other failure
	m_inputDispatcher.setFailureHandler([this](const QDBusError&) {
		close();
		setState(State::Failed);
		Q_EMIT failed();
	});

	// xdg-desktop-portal derives the app ID from the process behind the unique name of the
	// calling connection: from its systemd unit/scope (sd_pid_get_user_unit(), "app-<id>-*.scope")
	// or, with portal >= 1.19.4, from org.freedesktop.host.portal.Registry.Register() on that
	// connection. Well-known bus names are not consulted. With portal >= 1.19.4 the server is
	// started without a scope, so start() registers the app ID before any other portal call on
	// this connection; otherwise the app ID is empty and e.g. KDE's kde-authorized
	// pre-authorization doesn't apply. Older portals don't provide the registry and keep using
	// the scope.

	const auto appId = QGuiApplication::desktopFileName();

	// Apply KDE pre-authorization so the consent dialog can be suppressed
	// in unattended/kiosk scenarios. These calls are best-effort and may
	// fail silently if flatpak is not installed.
	QProcess::execute(QStringLiteral("flatpak"),
					  {QStringLiteral("permission-set"), QStringLiteral("kde-authorized"),
					   QStringLiteral("screencast"), appId, QStringLiteral("yes")});
	QProcess::execute(QStringLiteral("flatpak"),
					  {QStringLiteral("permission-set"), QStringLiteral("kde-authorized"),
					   QStringLiteral("remote-desktop"), appId, QStringLiteral("yes")});

	// The portal doesn't resolve the app ID from the well-known name, but it helps consistency
	// for logging and debugging.
	if (!m_sessionBus.registerService(appId))
	{
		vDebug() << "Could not register D-Bus service name" << appId
				 << ":" << m_sessionBus.lastError().message();
	}

	// persist_mode 2 only pays off across server runs if the token outlives this process; a
	// token left from when persistence was enabled is removed so it no longer grants anything
	if (m_persistRestoreToken)
	{
		loadRestoreToken();
	}
	else
	{
		QFile::remove(restoreTokenFilePath());
	}
}



PortalSession::~PortalSession()
{
	close();
	m_sessionBus.unregisterService(QGuiApplication::desktopFileName());
}



void PortalSession::start()
{
	if (m_state != State::Idle && m_state != State::Failed)
	{
		return;
	}

	// the registration comes first, also with a restarted portal, and determines the portal
	// instance all other calls go to; the owner retries a failed start
	if (registerHostAppId() == false)
	{
		close();
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	// a failed session may still be open at the portal
	close();
	m_inputDispatcher.reset();
	m_pipewireFd = -1;
	m_pipeWireNodeId = 0;
	m_streamGeometry = {};
	m_pointerMappingMode = PortalPointerMappingMode::Scale;
	createSession();
}



void PortalSession::setInputReplyTimeout(int milliseconds)
{
	m_inputDispatcher.setReplyTimeout(milliseconds);
}



void PortalSession::close()
{
	// input state of this session must not leak into the next one, and nothing may stay pressed
	// in the compositor
	m_inputDispatcher.clear();
	releaseHeldInputs();
	m_lastButtonMask = 0;
	m_lastPointerX = 0;
	m_lastPointerY = 0;

	disconnectResponseSignal();

	if (!m_sessionHandle.isEmpty())
	{
		m_sessionBus.asyncCall(portalCall(m_sessionHandle, SessionInterface, QStringLiteral("Close")));
		m_sessionHandle.clear();
	}

	setState(State::Idle);
}



void PortalSession::createSession()
{
	setState(State::CreatingSession);

	const QString token = makeRequestToken();

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	options[QStringLiteral("session_handle_token")] = makeRequestToken();

	// Subscribe to the Response signal BEFORE making the async call to avoid a
	// race condition where the portal emits Response before we can connect.
	connectResponseSignal(makeRequestPath(token));

	auto call = m_sessionBus.asyncCall(
		portalCall(PortalObjectPath, RemoteDesktopInterface, QStringLiteral("CreateSession"), {options}));
	auto* watcher = new QDBusPendingCallWatcher(call, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "CreateSession call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}



void PortalSession::selectDevices()
{
	setState(State::SelectingDevices);

	const QString token = makeRequestToken();

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	// Request keyboard + pointer devices (1=keyboard, 2=pointer, 3=both)
	options[QStringLiteral("types")] = QVariant::fromValue<uint>(3u);
	// persist_mode 2 = persist until explicitly revoked
	options[QStringLiteral("persist_mode")] = QVariant::fromValue<uint>(2u);
	if (!m_restoreToken.isEmpty())
	{
		options[QStringLiteral("restore_token")] = m_restoreToken;
	}

	connectResponseSignal(makeRequestPath(token));

	auto call = m_sessionBus.asyncCall(portalCall(PortalObjectPath, RemoteDesktopInterface, QStringLiteral("SelectDevices"),
												  {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), options}));
	connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "SelectDevices call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}



void PortalSession::selectSources()
{
	setState(State::SelectingSources);

	const QString token = makeRequestToken();

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	options[QStringLiteral("types")] = QVariant::fromValue<uint>(1u); // 1=monitor
	options[QStringLiteral("multiple")] = false;
	// ScreenCast interface only supports persist_mode=0; persist_mode=2 is
	// only valid on the RemoteDesktop interface (used in selectDevices()).
	options[QStringLiteral("persist_mode")] = QVariant::fromValue<uint>(0u);

	connectResponseSignal(makeRequestPath(token));

	auto call = m_sessionBus.asyncCall(portalCall(PortalObjectPath, ScreenCastInterface, QStringLiteral("SelectSources"),
												  {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), options}));
	auto* watcher = new QDBusPendingCallWatcher(call, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "SelectSources call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}



void PortalSession::startSession()
{
	setState(State::Starting);

	const QString token = makeRequestToken();

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;

	connectResponseSignal(makeRequestPath(token));

	// parent_window handle empty for daemon/unattended use
	auto call = m_sessionBus.asyncCall(portalCall(PortalObjectPath, RemoteDesktopInterface, QStringLiteral("Start"),
												  {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), QString(), options}));
	auto* watcher = new QDBusPendingCallWatcher(call, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "Start call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}



void PortalSession::openPipeWireRemote()
{
	QDBusPendingReply<QDBusUnixFileDescriptor> pending = m_sessionBus.asyncCall(
		portalCall(PortalObjectPath, ScreenCastInterface, QStringLiteral("OpenPipeWireRemote"),
				   {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), QVariantMap{}}));
	pending.waitForFinished();

	if (pending.isError())
	{
		vCritical() << "OpenPipeWireRemote failed:" << pending.error().message();
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	// dup() the FD so we own it independently of the QDBus reply lifetime
	m_pipewireFd = ::dup(pending.value().fileDescriptor());
	if (m_pipewireFd < 0)
	{
		vCritical() << "dup() of PipeWire FD failed";
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	setState(State::Running);
	Q_EMIT started();
}



void PortalSession::connectResponseSignal(const QString& requestPath)
{
	disconnectResponseSignal();

	m_connectedRequestPath = requestPath;
	m_requestInterface = new OrgFreedesktopPortalRequestInterface(
		PortalService, requestPath, m_sessionBus, this);

	connect(m_requestInterface, &OrgFreedesktopPortalRequestInterface::Response,
			this, &PortalSession::onPortalResponse);
}



void PortalSession::disconnectResponseSignal()
{
	delete m_requestInterface;
	m_requestInterface = nullptr;
	m_connectedRequestPath.clear();
}



void PortalSession::onPortalResponse(uint response, const QVariantMap& results)
{
	disconnectResponseSignal();

	if (response != 0)
	{
		vWarning() << "Portal request in state" << static_cast<int>(m_state)
				   << "denied/cancelled (response=" << response << ")";
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	// Save restore token whenever the portal provides one
	if (results.contains(QStringLiteral("restore_token")))
	{
		m_restoreToken = results.value(QStringLiteral("restore_token")).toString();
		vDebug() << "Got portal restore_token";
		if (m_persistRestoreToken)
		{
			storeRestoreToken();
		}
	}

	switch (m_state)
	{
	case State::CreatingSession:
	{
		// session_handle can arrive as QDBusObjectPath or as a plain string
		QDBusObjectPath sessionPath = results.value(QStringLiteral("session_handle")).value<QDBusObjectPath>();
		m_sessionHandle = sessionPath.path().isEmpty() ? results.value(QStringLiteral("session_handle")).toString() : sessionPath.path();

		if (m_sessionHandle.isEmpty())
		{
			vCritical() << "CreateSession response missing session_handle";
			setState(State::Failed);
			Q_EMIT failed();
			return;
		}
		vDebug() << "Portal session created:" << m_sessionHandle;
		selectDevices();
		break;
	}

	case State::SelectingDevices:
		selectSources();
		break;

	case State::SelectingSources:
		startSession();
		break;

	case State::Starting:
		readStreams(results);
		openPipeWireRemote();
		break;

	default:
		vWarning() << "Unexpected portal response in state" << static_cast<int>(m_state);
		break;
	}
}

void PortalSession::readStreams(const QVariantMap& results)
{
	// a(ua{sv}): PipeWire node ID and properties of each stream; only one monitor is selected
	const auto streams = results.value(QStringLiteral("streams")).value<QDBusArgument>();
	if (streams.currentSignature() == QLatin1String("a(ua{sv})"))
	{
		streams.beginArray();
		if (!streams.atEnd())
		{
			QVariantMap properties;
			streams.beginStructure();
			streams >> m_pipeWireNodeId >> properties;
			streams.endStructure();

			m_streamGeometry.position = readIntPair(properties.value(QStringLiteral("position"))).value_or(QPoint{});
			const auto size = readIntPair(properties.value(QStringLiteral("size")));
			if (size.has_value() && size->x() > 0 && size->y() > 0)
			{
				m_streamGeometry.size = QSize(size->x(), size->y());
			}
		}
		streams.endArray();
	}

	m_pointerMappingMode = detectPointerMappingMode();

	vDebug() << "Portal session started, PipeWire node ID:" << m_pipeWireNodeId
			 << "stream position:" << m_streamGeometry.position << "size:" << m_streamGeometry.size
			 << "pointer mapping:" << (m_pointerMappingMode == PortalPointerMappingMode::Scale ? "scale" : "clamp only");

	if (m_streamGeometry.size.isEmpty())
	{
		vWarning() << "the portal reported no stream size - pointer positions are passed unscaled, "
					  "xdg-desktop-portal >= 1.14 rejects all of them";
	}
}



PortalPointerMappingMode PortalSession::detectPointerMappingMode() const
{
	// mutter takes positions in buffer pixels (see PortalPointerMappingMode::ClampOnly); its
	// RemoteDesktop service, which xdg-desktop-portal-gnome talks to, is the most reliable sign
	// that mutter is the compositor, also for other mutter based desktops
	if (const auto busInterface = m_sessionBus.interface())
	{
		const QDBusReply<bool> registered = busInterface->isServiceRegistered(MutterRemoteDesktopService);
		if (registered.isValid())
		{
			return registered.value() ? PortalPointerMappingMode::ClampOnly : PortalPointerMappingMode::Scale;
		}
	}

	// the bus could not be asked
	const auto desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(QLatin1Char(':'));
	if (desktops.contains(QStringLiteral("GNOME"), Qt::CaseInsensitive))
	{
		return PortalPointerMappingMode::ClampOnly;
	}

	return PortalPointerMappingMode::Scale;
}



std::optional<QPoint> PortalSession::readIntPair(const QVariant& value)
{
	if (value.canConvert<QDBusArgument>() == false)
	{
		return std::nullopt;
	}

	const auto argument = value.value<QDBusArgument>();
	if (argument.currentSignature() != QLatin1String("(ii)"))
	{
		return std::nullopt;
	}

	int first = 0;
	int second = 0;
	argument.beginStructure();
	argument >> first >> second;
	argument.endStructure();

	return QPoint(first, second);
}

// ---------------------------------------------------------------------------
// Restore token persistence
// ---------------------------------------------------------------------------

QString PortalSession::restoreTokenFilePath()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	const auto genericStateDir = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation);
#else
	const auto genericStateDir = QDir::homePath() + QStringLiteral("/.local/share");
#endif

	return genericStateDir + QStringLiteral("/veyon/portal-restore-token");
}



void PortalSession::loadRestoreToken()
{
	QFile file(restoreTokenFilePath());
	if (file.open(QFile::ReadOnly))
	{
		const auto token = QString::fromUtf8(file.read(MaximumRestoreTokenLength + 1)).trimmed();
		if (isValidRestoreToken(token))
		{
			m_restoreToken = token;
		}
	}
}



void PortalSession::storeRestoreToken() const
{
	const auto path = restoreTokenFilePath();
	QSaveFile file(path);
	if (QDir().mkpath(QFileInfo(path).path()) == false ||
		file.open(QFile::WriteOnly) == false ||
		file.setPermissions(QFile::ReadOwner | QFile::WriteOwner) == false ||
		file.write(m_restoreToken.toUtf8()) < 0 ||
		file.commit() == false)
	{
		vWarning() << "could not store portal restore token";
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void PortalSession::setState(State s)
{
	m_state = s;
}

QString PortalSession::makeRequestToken() const
{
	return QStringLiteral("veyon_%1").arg(QRandomGenerator::global()->generate());
}

QString PortalSession::senderToken() const
{
	return m_sessionBus.baseService().remove(QLatin1Char(':')).replace(QLatin1Char('.'), QLatin1Char('_'));
}

QString PortalSession::makeRequestPath(const QString& token) const
{
	return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(senderToken(), token);
}

// ---------------------------------------------------------------------------
// Input event forwarding
// ---------------------------------------------------------------------------

void PortalSession::notifyKeyboardKeysym(quint32 keySym, bool down)
{
	if (m_state != State::Running)
	{
		return;
	}

	if (down)
	{
		press({HeldInput::Type::Key, keySym});
	}
	else
	{
		release({HeldInput::Type::Key, keySym}, MaximumReleaseRetries);
	}
}

QDBusMessage PortalSession::inputMessage(HeldInput input, bool pressed) const
{
	// portal state: 0 = released, 1 = pressed
	return portalCall(PortalObjectPath, RemoteDesktopInterface,
					  input.type == HeldInput::Type::Button ? QStringLiteral("NotifyPointerButton")
															: QStringLiteral("NotifyKeyboardKeysym"),
					  {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), QVariantMap{}, int(input.code),
					   pressed ? 1u : 0u});
}



QDBusPendingCall PortalSession::sendInput(HeldInput input, bool pressed)
{
	return m_sessionBus.asyncCall(inputMessage(input, pressed));
}



void PortalSession::press(HeldInput input)
{
	// held until the portal accepted a release sent after this press
	m_heldInputs[input.id()] = ++m_pressSequence;

	m_inputDispatcher.enqueue(PortalInputDispatcher::RequestType::Default,
							  [this, input]() { return sendInput(input, true); });
}



void PortalSession::release(HeldInput input, int retries)
{
	const auto pressSequence = m_heldInputs.value(input.id(), 0);

	m_inputDispatcher.enqueue(PortalInputDispatcher::RequestType::Release,
							  [this, input]() { return sendInput(input, false); },
							  [this, input, pressSequence, retries](PortalInputDispatcher::Result result) {
		const auto currentPressSequence = m_heldInputs.value(input.id(), 0);
		// pressed again in the meantime: the release of that press takes over
		if (currentPressSequence != pressSequence)
		{
			return;
		}

		// an error reply is deterministic (e.g. no access to the device), a retry would fail as well
		if (result != PortalInputDispatcher::Result::NoReply || pressSequence == 0)
		{
			m_heldInputs.remove(input.id());
		}
		else if (retries > 0)
		{
			release(input, retries - 1);
		}
		else if (m_lastReleaseWarning.isValid() == false ||
				 m_lastReleaseWarning.elapsed() >= PortalInputDispatcher::ErrorLogInterval)
		{
			vWarning() << "could not release" << (input.type == HeldInput::Type::Key ? "key" : "button")
					   << input.code << "- releasing it when closing the session";
			m_lastReleaseWarning.start();
		}
	});
}



void PortalSession::releaseHeldInputs()
{
	if (m_state == State::Running && m_sessionHandle.isEmpty() == false)
	{
		// xdg-desktop-portal handles every call in a thread of its own, so Close() could overtake
		// the releases, which then fail with "Invalid session" and leave the input pressed (e.g. on
		// KWin's fake input), so wait for their replies. The wait is bounded, as an unresponsive
		// portal may be why the session is closed, and QDBus::Block doesn't process events, so
		// nothing can re-enter close() meanwhile.
		const QDeadlineTimer deadline(ReleaseOnCloseTimeout);
		for (auto it = m_heldInputs.constBegin(); it != m_heldInputs.constEnd(); ++it)
		{
			const auto input = HeldInput::fromId(it.key());
			const auto remainingTime = deadline.remainingTime();
			if (remainingTime > 0)
			{
				m_sessionBus.call(inputMessage(input, false), QDBus::Block, int(remainingTime));
			}
			else
			{
				sendInput(input, false);
			}
		}
	}

	m_heldInputs.clear();
}



void PortalSession::notifyPointer(int buttonMask, int x, int y, QSize framebufferSize)
{
	if (m_state != State::Running || m_pipeWireNodeId == 0)
	{
		return;
	}

	const auto sessionPath = QDBusObjectPath(m_sessionHandle);

	// Send motion event when the position changed
	const auto position = mapPointerToPortalStream({x, y}, framebufferSize, m_streamGeometry, m_pointerMappingMode);
	if (position.has_value() && (position->x() != m_lastPointerX || position->y() != m_lastPointerY))
	{
		const auto nodeId = m_pipeWireNodeId;
		const auto px = position->x();
		const auto py = position->y();
		m_inputDispatcher.enqueue(
			PortalInputDispatcher::RequestType::PointerMotion,
			[this, sessionPath, nodeId, px, py]() {
			return m_sessionBus.asyncCall(portalCall(PortalObjectPath, RemoteDesktopInterface,
													 QStringLiteral("NotifyPointerMotionAbsolute"),
													 {QVariant::fromValue(sessionPath), QVariantMap{}, nodeId, px, py}));
		},
			[this, px, py](PortalInputDispatcher::Result result) {
			// the pointer may not be there, so send the position again with the next event, e.g. a
			// click without moving
			if (result != PortalInputDispatcher::Result::Accepted && px == m_lastPointerX && py == m_lastPointerY)
			{
				m_lastPointerX = std::numeric_limits<double>::quiet_NaN();
				m_lastPointerY = std::numeric_limits<double>::quiet_NaN();
			}
		});
		m_lastPointerX = px;
		m_lastPointerY = py;
	}

	// Send button/scroll events for every bit that changed
	if (buttonMask != m_lastButtonMask)
	{
		// Linux evdev button codes matching VNC button mask bits 0–8
		// Bits 3-6 are scroll directions (no evdev button code — use axis)
		static const int evdevButtons[] = {
			BTN_LEFT,	// bit 0
			BTN_MIDDLE, // bit 1
			BTN_RIGHT,	// bit 2
			0,			// bit 3 – scroll up   (axis event)
			0,			// bit 4 – scroll down (axis event)
			0,			// bit 5 – scroll left (axis event)
			0,			// bit 6 – scroll right (axis event)
			BTN_SIDE,	// bit 7
			BTN_EXTRA,	// bit 8
		};
		static constexpr int numButtons = static_cast<int>(sizeof(evdevButtons) / sizeof(evdevButtons[0]));

		for (int i = 0; i < numButtons; ++i)
		{
			const int prev = (m_lastButtonMask >> i) & 1;
			const int current = (buttonMask >> i) & 1;
			if (prev == current)
			{
				continue;
			}

			if (evdevButtons[i] != 0)
			{
				// Regular button press/release
				const HeldInput button{HeldInput::Type::Button, quint32(evdevButtons[i])};
				if (current != 0)
				{
					press(button);
				}
				else
				{
					release(button, MaximumReleaseRetries);
				}
			}
			else if (current != 0)
			{
				// Scroll axis: only fire on press (the release has no discrete step)
				int axis = 0; // 0 = vertical
				int steps = 0;
				switch (i)
				{
				case 3:
					axis = 0;
					steps = -1;
					break; // scroll up
				case 4:
					axis = 0;
					steps = 1;
					break; // scroll down
				case 5:
					axis = 1;
					steps = -1;
					break; // scroll left
				case 6:
					axis = 1;
					steps = 1;
					break; // scroll right
				default: break;
				}
				if (steps != 0)
				{
					m_inputDispatcher.enqueue(PortalInputDispatcher::RequestType::Default,
											  [this, sessionPath, axis, steps]() {
						return m_sessionBus.asyncCall(portalCall(PortalObjectPath, RemoteDesktopInterface,
																 QStringLiteral("NotifyPointerAxisDiscrete"),
																 {QVariant::fromValue(sessionPath), QVariantMap{},
																  static_cast<uint>(axis), steps}));
					});
				}
			}
		}

		m_lastButtonMask = buttonMask;
	}
}
