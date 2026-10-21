/*
 * VeyonAuthHelper.cpp - main file for Veyon Authentication Helper
 *
 * Copyright (c) 2010-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>

#include "VeyonConfiguration.h"
#include "../LinuxDBusInterfaces.h"
#include "../LinuxPlatformConfiguration.h"

#include <cstdlib>
#include <cstring>
#include <security/pam_appl.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <syslog.h>

static QByteArray pam_username; // clazy:exclude=non-pod-global-static
static QByteArray pam_password; // clazy:exclude=non-pod-global-static
static QByteArray pam_service; // clazy:exclude=non-pod-global-static

static int pam_conv(int num_msg, const struct pam_message** msg, struct pam_response** resp, void *)
{
	auto reply = reinterpret_cast<pam_response *>(malloc(sizeof(struct pam_response) * size_t(num_msg)));
	if (reply == nullptr)
	{
		return PAM_CONV_ERR;
	}

	memset(reply, 0, sizeof(struct pam_response) * size_t(num_msg));

	for (int replies = 0; replies < num_msg; ++replies)
	{
		switch (msg[replies]->msg_style)
		{
		case PAM_PROMPT_ECHO_ON:
			reply[replies].resp_retcode = PAM_SUCCESS;
			reply[replies].resp = strdup(pam_username.constData());
			break;
		case PAM_PROMPT_ECHO_OFF:
			reply[replies].resp_retcode = PAM_SUCCESS;
			reply[replies].resp = strdup(pam_password.constData());
			break;
		case PAM_TEXT_INFO:
		case PAM_ERROR_MSG:
			reply[replies].resp_retcode = PAM_SUCCESS;
			reply[replies].resp = nullptr;
			break;
		default:
			for (int i = 0; i < replies; ++i)
			{
				if (reply[i].resp != nullptr)
				{
					explicit_bzero(reply[i].resp, strlen(reply[i].resp));
					free(reply[i].resp);
				}
			}
			free(reply);
			return PAM_CONV_ERR;
		}
	}

	*resp = reply;
	return PAM_SUCCESS;
}


class VeyonAuthHelper : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "io.veyon.AuthHelper")
public:
	explicit VeyonAuthHelper(QObject* parent = nullptr) :
		QObject(parent)
	{
	}

public Q_SLOTS:
	bool Authenticate(const QString& username, const QByteArray& password)
	{
		pam_username = username.toUtf8();
		pam_password = password;
		pam_service = LinuxPlatformConfiguration{&VeyonCore::config()}.pamServiceName().toUtf8();

		if (pam_service.isEmpty())
		{
			pam_service = QByteArrayLiteral("login");
		}

		struct pam_conv pconv = { &pam_conv, nullptr };
		pam_handle_t* pamh = nullptr;
		auto err = pam_start(pam_service.constData(), nullptr, &pconv, &pamh);
		if (err == PAM_SUCCESS)
		{
			err = pam_authenticate(pamh, PAM_SILENT);
			if (err != PAM_SUCCESS)
			{
				syslog(LOG_AUTHPRIV | LOG_NOTICE, "veyon-auth-helper: pam_authenticate failed: %s",
					   pam_strerror(pamh, err));
			}
			else
			{
				err = pam_acct_mgmt(pamh, PAM_SILENT);
				if (err != PAM_SUCCESS)
				{
					syslog(LOG_AUTHPRIV | LOG_NOTICE, "veyon-auth-helper: pam_acct_mgmt failed: %s",
						   pam_strerror(pamh, err));
				}
			}
		}
		else
		{
			syslog(LOG_AUTHPRIV | LOG_NOTICE, "veyon-auth-helper: pam_start failed: %s",
				   pam_strerror(pamh, err));
		}

		pam_end(pamh, err);

		explicit_bzero(pam_password.data(), size_t(pam_password.size()));
		pam_password.clear();
		explicit_bzero(pam_username.data(), size_t(pam_username.size()));
		pam_username.clear();

		return err == PAM_SUCCESS;
	}

};


int main(int argc, char** argv)
{
	// Make the process as hard to inspect/attach to as possible: it holds
	// a plaintext password in memory while running as root.
	prctl(PR_SET_DUMPABLE, 0);

	struct rlimit noCore {0, 0};
	setrlimit(RLIMIT_CORE, &noCore);

	// Sanitize the environment before touching PAM or D-Bus: PAM modules
	// (LDAP, Kerberos, custom stacks) may consult environment variables, and
	// this process inherits its environment from systemd, not from an
	// unprivileged caller. The system bus address is not taken from the
	// environment deliberately.
	static char safePath[] = "PATH=/usr/sbin:/usr/bin:/sbin:/bin";
	clearenv();
	putenv(safePath);

	umask(077);

	QCoreApplication app(argc, argv);
	VeyonCore core(&app, VeyonCore::Component::AuthHelper, QStringLiteral("AuthHelper"));

	auto bus = QDBusConnection::systemBus();
	if (bus.isConnected() == false)
	{
		syslog(LOG_AUTHPRIV | LOG_ERR, "veyon-auth-helper: system bus not available");
		return -1;
	}

	VeyonAuthHelper helper;
	if (bus.registerObject(AuthHelperInterface::ObjectPath, &helper, QDBusConnection::ExportAllSlots) == false ||
		bus.registerService(AuthHelperInterface::ServiceName) == false)
	{
		syslog(LOG_AUTHPRIV | LOG_ERR, "veyon-auth-helper: failed to register D-Bus service: %s",
			   qPrintable(bus.lastError().message()));
		return -1;
	}

	return app.exec();
}


IMPLEMENT_CONFIG_PROXY(LinuxPlatformConfiguration)

#include "VeyonAuthHelper.moc"
