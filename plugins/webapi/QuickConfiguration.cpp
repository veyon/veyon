// SPDX-License-Identifier: GPL-2.0-or-later
#include "WebApiPlugin.h"
#include "WebApiConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* WebApiPlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new WebApiConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->httpServerEnabledProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("Enable HTTP server"), Editor::Automatic, {}, {}, {});
	restart(config->httpServerEnabledProperty());
	describe(config->httpServerPortProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("HTTP server port"), Editor::Integer, 1, 65535, QStringLiteral(""));
	restart(config->httpServerPortProperty());
	describe(config->connectionLifetimeProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("Connection lifetime"), Editor::Integer, 1, 24, QStringLiteral(" h"));
	restart(config->connectionLifetimeProperty());
	describe(config->connectionIdleTimeoutProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("Idle timeout"), Editor::Integer, 10, 3600, QStringLiteral(" s"));
	restart(config->connectionIdleTimeoutProperty());
	describe(config->connectionAuthenticationTimeoutProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("Authentication timeout"), Editor::Integer, 10, 3600, QStringLiteral(" s"));
	restart(config->connectionAuthenticationTimeoutProperty());
	describe(config->connectionLimitProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("Connection limit"), Editor::Integer, 1, 1000, QStringLiteral(""));
	restart(config->connectionLimitProperty());
	describe(config->httpsEnabledProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("Enable HTTPS"), Editor::Automatic, {}, {}, {});
	restart(config->httpsEnabledProperty());
	describe(config->tlsCertificateFileProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("TLS certificate"), Editor::File, {}, {}, {});
	restart(config->tlsCertificateFileProperty());
	describe(config->tlsPrivateKeyFileProperty(), QStringLiteral("webapi"), tr("HTTP server"), tr("TLS private key"), Editor::File, {}, {}, {});
	restart(config->tlsPrivateKeyFileProperty());
	depends(config->httpServerPortProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled(); });
	depends(config->connectionLifetimeProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled(); });
	depends(config->connectionIdleTimeoutProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled(); });
	depends(config->connectionAuthenticationTimeoutProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled(); });
	depends(config->connectionLimitProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled(); });
	depends(config->httpsEnabledProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled(); });
	depends(config->tlsCertificateFileProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled() && config->httpsEnabled(); });
	depends(config->tlsPrivateKeyFileProperty(), {QStringLiteral("WebAPI/HttpServerEnabled"), QStringLiteral("WebAPI/HttpsEnabled")}, [config]() { return config->httpServerEnabled() && config->httpsEnabled(); });
	{ auto ui = config->httpServerPortProperty().uiMetadata(); ui.step = 10; config->httpServerPortProperty().setUiMetadata(ui); }
	{ auto ui = config->connectionIdleTimeoutProperty().uiMetadata(); ui.step = 10; config->connectionIdleTimeoutProperty().setUiMetadata(ui); }
	{ auto ui = config->connectionAuthenticationTimeoutProperty().uiMetadata(); ui.step = 10; config->connectionAuthenticationTimeoutProperty().setUiMetadata(ui); }
	{ auto ui = config->connectionLimitProperty().uiMetadata(); ui.step = 10; config->connectionLimitProperty().setUiMetadata(ui); }
	config->setObjectName(QStringLiteral("webapi"));
	return config;
}
