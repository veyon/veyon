/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "DesktopStyle.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QToolTip>

void DesktopStyle::apply(QApplication* app)
{
	const bool dark = VeyonCore::useDarkMode();
	const QColor background(dark ? QStringLiteral("#181716") : QStringLiteral("#f3f2f2"));
	const QColor surface(dark ? QStringLiteral("#22201f") : QStringLiteral("#eae9e9"));
	const QColor text(dark ? QStringLiteral("#edebea") : QStringLiteral("#201e1d"));
	const QColor accent(dark ? QStringLiteral("#ff563c") : QStringLiteral("#ec3013"));
	const QColor muted(dark ? QStringLiteral("#bab6b6") : QStringLiteral("#605d5d"));
	const QColor divider(dark ? QStringLiteral("#595554") : QStringLiteral("#aaa6a5"));
	const QColor hover(dark ? QStringLiteral("#343130") : QStringLiteral("#dedada"));
	auto palette = app->palette();
	palette.setColor(QPalette::Window, background);
	palette.setColor(QPalette::WindowText, text);
	palette.setColor(QPalette::Base, background);
	palette.setColor(QPalette::AlternateBase, surface);
	palette.setColor(QPalette::Text, text);
	palette.setColor(QPalette::Button, surface);
	palette.setColor(QPalette::ButtonText, text);
	palette.setColor(QPalette::Highlight, accent);
	palette.setColor(QPalette::HighlightedText, dark ? background : QColor(Qt::white));
	palette.setColor(QPalette::Link, accent);
	palette.setColor(QPalette::Mid, divider);
	palette.setColor(QPalette::ToolTipBase, text);
	palette.setColor(QPalette::ToolTipText, background);
	palette.setColor(QPalette::PlaceholderText, muted);
	for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
	{
		palette.setColor(QPalette::Disabled, role, dark ? QColor(QStringLiteral("#7d7979")) : QColor(QStringLiteral("#9b9797")));
	}
	app->setPalette(palette);
	QToolTip::setPalette(palette);

	QFile file(QStringLiteral(":/core/desktop.qss"));
	if (file.open(QFile::ReadOnly))
	{
		auto sheet = QString::fromUtf8(file.readAll());
		sheet += QStringLiteral(R"QSS(
QWidget { font-family: sans-serif; font-size: 13px; }
QWidget#masterWorkspace, QWidget#classroomCenter, QWidget#remoteAccessWindow {
 background: @background@; color: @text@;
}
QWidget#classroomSidebar, QWidget#ConfiguratorSidebar {
 background: @surface@; border: 0; border-right: 2px solid @divider@;
}
QWidget#classroomSidebar QLineEdit, QWidget#ConfiguratorSidebar QLineEdit {
 background: @background@; padding: 3px 7px; font-size: 12px;
}
QLabel#sectionLabel, QLabel#classroomTargetLabel {
 color: @muted@; font-size: 11px; font-weight: 600; background: transparent;
}
QListWidget#classroomRooms {
 background: @surface@; border: 0; padding: 0; outline: 0;
}
QWidget#classroomCommands {
 background: @background@; border: 0; border-bottom: 2px solid @divider@;
}
QWidget#classroomCommands QToolButton {
 background: transparent; border: 1px solid transparent; padding: 2px 7px; font-weight: 600;
}
QWidget#classroomCommands QToolButton:hover {
 background: @hover@; border-color: transparent;
}
QWidget#classroomCommands QToolButton[bordered="true"] {
 border-color: @divider@; padding-left: 10px; padding-right: 10px;
}
QWidget#classroomCommands QToolButton[dropdownIndicator="true"] { padding-right: 24px; }
QWidget#classroomCommands QToolButton::menu-indicator { image: none; width: 0; height: 0; }
QWidget#classroomCommands QToolButton[segment="true"] {
 background: @surface@; border: 1px solid @divider@; padding: 2px 10px; font-size: 12px;
}
QWidget#classroomCommands QToolButton:checked,
QWidget#classroomCommands QToolButton[segment="true"]:checked {
 background: @accent@; color: @onAccent@; border-color: @accent@;
}
QWidget#classroomCommands QToolButton:disabled {
 color: palette(disabled, button-text); background: transparent; border-color: transparent;
}
QWidget#classroomCommands QToolButton[segment="true"]:disabled {
 background: @surface@; border-color: @divider@;
}
QWidget#classroomCommands QToolButton:focus {
 border-color: @accent@;
}
QWidget#classroomModeBanner {
 background: #ae1800; color: #ffffff; font-weight: 600;
}
QWidget#classroomModeBanner QLabel { background: transparent; color: #ffffff; }
QWidget#classroomModeBanner QToolButton {
 background: transparent; color: #ffffff; border: 1px solid #ffffff; padding: 2px 10px;
 font-size: 12px; font-weight: 600;
}
QWidget#classroomModeBanner QToolButton:hover { background: #c03a25; }
QWidget#classroomStatus, QWidget#remoteStatusBar {
 background: @surface@; border: 0; border-top: 2px solid @divider@;
}
QWidget#classroomStatus QLabel, QWidget#remoteStatusBar QLabel {
 background: transparent; font-size: 12px;
}
QLabel#workspaceSummary { color: @text@; }
QWidget#classroomStatus QToolButton {
 padding: 0; border: 1px solid @divider@; background: @surface@;
}
QWidget#classroomStatus QToolButton:checked {
 background: @text@; color: @background@; border-color: @text@;
}
QWidget#computerDetails {
 background: @surface@; border: 0; border-left: 2px solid @divider@;
}
QLabel#detailsTitle { font-size: 17px; font-weight: 800; }
QLabel#detailsPreview { background: @background@; border: 1px solid @divider@; }
QLabel#detailState { font-weight: 600; }
QWidget#computerDetails QLabel#detailMetadataKey { color: @muted@; font-size: 12px; }
QWidget#computerDetails QLabel#detailMetadataValue { font-size: 12px; }
QFrame#detailSeparator { color: @divider@; background: @divider@; max-height: 1px; border: 0; }
QWidget#computerDetails QToolButton {
 background: transparent; border: 1px solid transparent; padding: 2px 8px;
}
QWidget#computerDetails QToolButton:hover { background: @hover@; }
QWidget#computerDetails QToolButton[dangerous="true"] { color: @accent@; }
QWidget#computerDetails QToolButton[bordered="true"] { border-color: @divider@; font-weight: 700; font-size: 12px; }
QWidget#computerDetails QToolButton[primary="true"] {
 background: @text@; color: @background@; border-color: @text@; font-weight: 700; font-size: 12px;
}
QWidget#computerDetails QToolButton:disabled {
 color: palette(disabled, button-text); background: @surface@; border-color: @divider@;
}
QTreeWidget#ConfiguratorNavigation {
 background: @surface@; border: 0; padding: 0; outline: 0; show-decoration-selected: 1;
}
QTreeWidget#ConfiguratorNavigation::item {
 border: 0; border-left: 3px solid transparent; padding: 0 9px; height: 30px; font-size: 12px;
}
QTreeWidget#ConfiguratorNavigation::item:has-children {
 color: @muted@; font-size: 11px; font-weight: 600; padding-top: 8px; background: @surface@;
}
QTreeWidget#ConfiguratorNavigation::item:hover {
 background: @hover@; color: @text@;
}
QTreeWidget#ConfiguratorNavigation::item:selected {
 background: @background@; color: @text@; border-left: 3px solid @accent@; font-weight: 600;
}
QTreeWidget#ConfiguratorNavigation::branch { background: transparent; border: 0; }
QLabel#ConfiguratorHeading { font-size: 20px; font-weight: 800; }
QLabel#ConfiguratorDescription { color: @muted@; }
QWidget#ConfiguratorHeadingDivider { background: @divider@; }
QLabel#ConfiguratorSearchShortcut { color: @muted@; font-size: 11px; background: transparent; border: 0; }
QPushButton#ConfiguratorImportExport {
 background: transparent; border: 1px solid @divider@; padding: 0 10px; font-weight: 600;
}
QPushButton#ConfiguratorFactoryReset { background: transparent; border: 0; color: @accent@; }
QLabel#ConfiguratorOverviewValue { color: @muted@; font-size: 12px; }
QWidget#ConfiguratorOverviewRow {
 background: @background@; border: 0; border-bottom: 1px solid @divider@;
}
QWidget#ConfiguratorOverviewRow QPushButton {
 background: transparent; padding: 5px 10px; font-size: 12px; font-weight: 600;
}
QWidget#ConfiguratorOverviewRow QPushButton:hover { background: @hover@; }
QWidget#ConfiguratorActionBar {
 background: @surface@; border: 0; border-top: 2px solid @divider@;
}
QLabel#ConfiguratorStatus { color: @text@; font-size: 12px; font-weight: 600; }
QWidget#ConfiguratorActionBar QPushButton, QWidget#ConfiguratorActionBar QToolButton {
 padding: 5px 10px; font-weight: 600;
}
QStackedWidget#configPages QGroupBox {
 background: transparent; border: 0; border-top: 2px solid @divider@;
 margin-top: 24px; padding: 16px 0 12px; font-weight: 700;
}
QStackedWidget#configPages QGroupBox::title {
 subcontrol-origin: margin; subcontrol-position: top left; left: 0; padding: 0 0 6px;
}
QWidget#remoteSessionHeader {
 background: @surface@; border: 0; border-bottom: 2px solid @divider@;
}
QWidget#remoteSessionHeader QLabel { font-weight: 600; background: transparent; }
QWidget#remoteSessionHeader QToolButton {
 background: transparent; border: 0; border-right: 1px solid @divider@; padding: 2px 12px 2px 0;
 font-weight: 600;
}
QWidget#remoteToolbar {
 background: @background@; border: 0; border-bottom: 2px solid @divider@;
}
QWidget#remoteToolbar QToolButton {
 background: transparent; border: 1px solid transparent; padding: 2px 8px; font-weight: 600;
}
QWidget#remoteToolbar QToolButton:hover, QWidget#remoteSessionHeader QToolButton:hover {
 background: @hover@;
}
QWidget#remoteModeSwitch {
 background: @divider@; border: 1px solid @divider@;
}
QWidget#remoteModeSwitch QToolButton {
 background: @background@; padding: 2px 14px; border: 0; font-weight: 700;
}
QWidget#remoteModeSwitch QToolButton:checked {
 background: @accent@; color: @onAccent@;
}
QWidget#remoteToolbar QToolButton:checked {
 background: @accent@; color: @onAccent@;
}
QWidget#remoteToolbar QToolButton#remoteEndSession { border-color: @divider@; padding: 2px 10px; }
QWidget#remoteToolbar QToolButton:disabled { color: palette(disabled, button-text); }
QWidget#remoteToolbar QToolButton:focus { border-color: @accent@; }
QWidget#remoteControlBanner QLabel { font-weight: 600; }
QWidget#remoteControlBanner QToolButton {
 border: 1px solid #ffffff; background: transparent; padding: 2px 10px; font-size: 12px; font-weight: 700;
}
QWidget#remoteControlBanner QToolButton:hover { background: #c03a25; }
)QSS");
		for (const auto& token : {qMakePair("@background@", background), qMakePair("@surface@", surface),
			qMakePair("@text@", text), qMakePair("@accent@", accent), qMakePair("@muted@", muted),
			qMakePair("@divider@", divider), qMakePair("@hover@", hover),
			qMakePair("@onAccent@", dark ? background : QColor(Qt::white))})
		{
			sheet.replace(QLatin1String(token.first), token.second.name());
		}
		app->setStyleSheet(sheet);
	}
}
