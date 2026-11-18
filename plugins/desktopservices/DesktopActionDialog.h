/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

#include "DesktopIcons.h"

namespace DesktopActionDialog
{
inline void arrange(QDialog* dialog, QGridLayout* oldLayout, const QString& heading,
	const QString& icon, const QString& recipientSummary, const QString& fieldLabel,
	QWidget* editor, QCheckBox* remember, QLabel* nameLabel, QLineEdit* name,
	QDialogButtonBox* buttons, const QString& action)
{
	while (auto item = oldLayout->takeAt(0))
	{
		delete item;
	}
	delete oldLayout;
	dialog->setObjectName(QStringLiteral("desktopActionDialog"));
	dialog->setWindowTitle(heading);
	dialog->setWindowIcon(DesktopIcons::icon(icon));
	dialog->setMinimumWidth(500);
	dialog->resize(500, 280);
	auto layout = new QVBoxLayout(dialog);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	auto header = new QWidget(dialog);
	auto headerLayout = new QHBoxLayout(header);
	headerLayout->setContentsMargins(20, 20, 20, 16);
	headerLayout->setSpacing(12);
	auto symbol = new QLabel(header);
	symbol->setPixmap(DesktopIcons::icon(icon).pixmap(24, 24));
	headerLayout->addWidget(symbol, 0, Qt::AlignTop);
	auto titles = new QVBoxLayout;
	titles->setSpacing(3);
	auto title = new QLabel(heading, header);
	title->setObjectName(QStringLiteral("desktopActionHeading"));
	title->setWordWrap(true);
	titles->addWidget(title);
	if (!recipientSummary.isEmpty())
	{
		auto recipients = new QLabel(recipientSummary, header);
		recipients->setObjectName(QStringLiteral("desktopActionRecipients"));
		recipients->setTextFormat(Qt::PlainText);
		recipients->setWordWrap(true);
		titles->addWidget(recipients);
	}
	headerLayout->addLayout(titles, 1);
	layout->addWidget(header);
	auto content = new QWidget(dialog);
	auto contentLayout = new QVBoxLayout(content);
	contentLayout->setContentsMargins(56, 0, 20, 16);
	contentLayout->setSpacing(6);
	auto label = new QLabel(fieldLabel, content);
	label->setObjectName(QStringLiteral("desktopActionFieldLabel"));
	label->setBuddy(editor);
	contentLayout->addWidget(label);
	contentLayout->addWidget(editor);
	contentLayout->addSpacing(8);
	contentLayout->addWidget(remember);
	auto savedName = new QWidget(content);
	auto savedNameLayout = new QHBoxLayout(savedName);
	savedNameLayout->setContentsMargins(0, 0, 0, 0);
	savedNameLayout->setSpacing(8);
	savedNameLayout->addWidget(nameLabel);
	savedNameLayout->addWidget(name, 1);
	contentLayout->addWidget(savedName);
	contentLayout->addStretch();
	savedName->setVisible(remember->isChecked());
	QObject::connect(remember, &QCheckBox::toggled, savedName, &QWidget::setVisible);
	layout->addWidget(content, 1);
	auto footer = new QWidget(dialog);
	footer->setObjectName(QStringLiteral("desktopActionFooter"));
	auto footerLayout = new QHBoxLayout(footer);
	footerLayout->setContentsMargins(20, 12, 20, 12);
	footerLayout->addWidget(buttons);
	layout->addWidget(footer);
	auto primary = buttons->button(QDialogButtonBox::Ok);
	primary->setText(action);
	const auto palette = dialog->palette();
	auto actionPixmap = DesktopIcons::icon(QStringLiteral("arrow-right")).pixmap(16, 16);
	QPainter painter(&actionPixmap);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.fillRect(actionPixmap.rect(), palette.color(QPalette::Window));
	painter.end();
	primary->setIcon(QIcon(actionPixmap));
	primary->setObjectName(QStringLiteral("desktopActionPrimary"));
	primary->setDefault(true);
	dialog->setStyleSheet(QStringLiteral(
		"QDialog#desktopActionDialog { background: %5; border: 2px solid %3; }"
		"QLabel#desktopActionHeading { font-size: 18px; font-weight: 800; }"
		"QLabel#desktopActionRecipients { color: %1; font-size: 12px; }"
		"QLabel#desktopActionFieldLabel { font-size: 12px; font-weight: 600; }"
		"QDialog#desktopActionDialog QLineEdit, QDialog#desktopActionDialog QTextEdit { background: %2; padding: 6px 8px; }"
		"QDialog#desktopActionDialog QCheckBox { font-size: 12px; }"
		"QWidget#desktopActionFooter { border-top: 2px solid %4; }"
		"QPushButton#desktopActionPrimary { background: %3; color: %2; border: 1px solid %3; padding: 6px 14px; font-weight: 700; }"
		"QPushButton#desktopActionPrimary:disabled { background: %4; color: %1; border-color: %4; }")
		.arg(palette.color(QPalette::PlaceholderText).name(), palette.color(QPalette::Window).name(),
			palette.color(QPalette::WindowText).name(), palette.color(QPalette::Mid).name(),
			palette.color(QPalette::AlternateBase).name()));
}
}
