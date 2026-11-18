// SPDX-FileCopyrightText: 2011-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AboutDialog.h"
#include "QuickUi.h"
#include <QDesktopServices>
#include <QFile>
#include <QUrl>
AboutDialog::AboutDialog(QWidget*) : QObject(nullptr) {}
int AboutDialog::exec()
{
	QString text = tr("Veyon %1").arg(VeyonCore::versionString());
	for (const auto& path : {QStringLiteral(":/CONTRIBUTORS"), QStringLiteral(":/core/COPYING")})
	{
		QFile file(path);
		if (file.open(QFile::ReadOnly)) text += QStringLiteral("\n\n") + QString::fromUtf8(file.readAll());
	}
	const QVariantList buttons{
		QVariantMap{{QStringLiteral("text"), tr("Donate")}, {QStringLiteral("value"), QStringLiteral("donate")}},
		QVariantMap{{QStringLiteral("text"), tr("Close")}, {QStringLiteral("value"), QStringLiteral("close")}}};
	const auto result = QuickUi::execDialog(QStringLiteral("about"), {
		{QStringLiteral("title"), tr("About Veyon %1").arg(VeyonCore::versionString())},
		{QStringLiteral("text"), text}, {QStringLiteral("buttons"), buttons}});
	if (result.value(QStringLiteral("choice")).toString() == QStringLiteral("donate"))
		QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.paypal.com/cgi-bin/webscr?item_name=Donation+to+Veyon+-+OpenSource+classroom+management&cmd=_donations&business=donate%40veyon.io")));
	return result.value(QStringLiteral("accepted")).toBool() ? 1 : 0;
}
