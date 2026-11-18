// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialogButtonBox>
#include <QGuiApplication>

#include <QPushButton>
#include <QTimer>

#include "DesktopAccessDialog.h"
#include "QuickUi.h"
#include "FeatureWorkerManager.h"
#include "HostAddress.h"
#include "PlatformCoreFunctions.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


DesktopAccessDialog::DesktopAccessDialog(QObject* parent) :
	QObject(parent),
	m_desktopAccessDialogFeature(Feature(QLatin1String(staticMetaObject.className()),
										 Feature::Flag::Service | Feature::Flag::Worker | Feature::Flag::Builtin,
										 Feature::Uid("3dd8ec3e-7004-4936-8f2a-70699b9819be"),
										 Feature::Uid(),
										 tr("Desktop access dialog"), {}, {})),
	m_features({m_desktopAccessDialogFeature}),
	m_choice(ChoiceNone),
	m_abortTimer(this)
{
	m_abortTimer.setSingleShot(true);
}



bool DesktopAccessDialog::isBusy(FeatureWorkerManager* featureWorkerManager) const
{
	return featureWorkerManager->isWorkerRunning(m_desktopAccessDialogFeature.uid());
}



void DesktopAccessDialog::exec(FeatureWorkerManager* featureWorkerManager, const QString& user, const QString& host)
{
	m_choice = ChoiceNone;

	featureWorkerManager->sendMessageToManagedSystemWorker(
		FeatureMessage(m_desktopAccessDialogFeature.uid(), FeatureCommand::RequestDesktopAccess)
			.addArgument(Argument::User, user)
			.addArgument(Argument::Host, host));

	m_abortTimer.disconnect();
	connect(&m_abortTimer, &QTimer::timeout, this, [=, this]() { abort(featureWorkerManager); });
	m_abortTimer.start(DialogTimeout);
}



void DesktopAccessDialog::abort(FeatureWorkerManager* featureWorkerManager)
{
	featureWorkerManager->stopWorker(m_desktopAccessDialogFeature.uid());

	m_choice = ChoiceNone;

	Q_EMIT finished();
}



bool DesktopAccessDialog::handleFeatureMessageFromWorker(VeyonServerInterface& server,
														 const FeatureMessage& message)
{
	if (m_desktopAccessDialogFeature.uid() == message.featureUid() &&
		message.command<FeatureCommand>() == FeatureCommand::ReportDesktopAccessChoice)
	{
		m_choice = message.argument(Argument::Choice).value<Choice>();

		server.featureWorkerManager().stopWorker(m_desktopAccessDialogFeature.uid());

		m_abortTimer.stop();

		Q_EMIT finished();

		return true;
	}

	return false;
}



bool DesktopAccessDialog::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	if (message.featureUid() != m_desktopAccessDialogFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::RequestDesktopAccess)
	{
		return false;
	}

	const auto result = requestDesktopAccess(message.argument(Argument::User).toString(),
											 message.argument(Argument::Host).toString());

	FeatureMessage reply(m_desktopAccessDialogFeature.uid(), FeatureCommand::ReportDesktopAccessChoice);
	reply.addArgument(Argument::Choice, result);

	return worker.sendFeatureMessageReply(reply);
}



DesktopAccessDialog::Choice DesktopAccessDialog::requestDesktopAccess(const QString& user, const QString& host)
{
	auto hostName = HostAddress(host).convert(HostAddress::Type::FullyQualifiedDomainName);
	if (hostName.isEmpty())
	{
		hostName = host;
	}

	qApp->setQuitOnLastWindowClosed(false);

	const QVariantList buttons{
		QVariantMap{{QStringLiteral("text"), tr("Never for this session")}, {QStringLiteral("value"), ChoiceNever}},
		QVariantMap{{QStringLiteral("text"), tr("No")}, {QStringLiteral("value"), ChoiceNo}},
		QVariantMap{{QStringLiteral("text"), tr("Yes")}, {QStringLiteral("value"), ChoiceYes}},
		QVariantMap{{QStringLiteral("text"), tr("Always for this session")}, {QStringLiteral("value"), ChoiceAlways}}};
	const auto result = QuickUi::execDialog(QStringLiteral("access"), {
		{QStringLiteral("title"), tr("Confirm desktop access")},
		{QStringLiteral("text"), tr("The user %1 at computer %2 wants to access your desktop. Do you want to grant access?").arg(user, hostName)},
		{QStringLiteral("buttons"), buttons}});
	return result.value(QStringLiteral("accepted")).toBool() ? static_cast<Choice>(result.value(QStringLiteral("choice")).toInt()) : ChoiceNo;
}
