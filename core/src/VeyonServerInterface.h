// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class FeatureMessage;
class FeatureWorkerManager;
class MessageContext;
class VncServerClient;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT VeyonServerInterface : public QObject
{
	Q_OBJECT
public:
	explicit VeyonServerInterface(QObject* parent) :
		QObject(parent)
	{
	}

	virtual ~VeyonServerInterface() = default;

	virtual FeatureWorkerManager& featureWorkerManager() = 0;
	virtual bool sendFeatureMessageReply(const MessageContext& context, const FeatureMessage& reply) = 0;

	virtual int vncServerBasePort() const = 0;

	// Unix domain socket of the local VNC server if it doesn't listen on a TCP port
	virtual QString vncServerLocalSocketPath() const = 0;

	virtual void setMinimumFramebufferUpdateInterval(const MessageContext& context, int interval) = 0;

	// authentication and access control state of the connection a message was received on;
	// nullptr if unknown (appended last to keep the vtable layout of 4.11 for existing plugins)
	virtual const VncServerClient* vncServerClient(const MessageContext& context) const = 0;
};
