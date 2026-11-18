// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QAction>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>

#include "ToolButton.h"

bool ToolButton::s_toolTipsDisabled = false;
bool ToolButton::s_iconOnlyMode = false;


ToolButton::ToolButton(const QIcon& icon,
					   const QString& label,
					   const QString& altLabel,
					   const QString& description,
					   const QKeySequence& shortcut) :
	m_icon(icon),
	m_label(label),
	m_altLabel(altLabel),
	m_descr(description)
{
	setShortcut(shortcut);

	setIcon(icon);
	setText(label);
	setAutoRaise(true);
	setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);

	if (m_altLabel.length() > 0)
	{
		connect(this, &ToolButton::toggled, this, [this](bool checked) {
			setText(checked ? m_altLabel : m_label);
		});
	}
}



void ToolButton::setIconOnlyMode(QWidget* mainWindow, bool enabled)
{
	s_iconOnlyMode = enabled;
	const auto toolButtons = mainWindow->findChildren<ToolButton*>();
	for (auto toolButton : toolButtons)
	{
		toolButton->setToolButtonStyle(enabled ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
	}
}



void ToolButton::addTo(QToolBar* toolBar)
{
	auto action = toolBar->addWidget(this);
	action->setText(m_label);
}



#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ToolButton::enterEvent(QEnterEvent* event)
#else
void ToolButton::enterEvent(QEvent* event)
#endif
{
	if (!s_toolTipsDisabled)
	{
		const auto pos = mapToGlobal(QPoint(width() / 2, height()));
		if (s_iconOnlyMode || m_descr.isEmpty())
		{
			QToolTip::showText(pos, text(), this);
		}
		else
		{
			QToolTip::showText(pos, m_descr, this);
		}
	}

	QToolButton::enterEvent(event);
}



void ToolButton::leaveEvent(QEvent* event)
{
	if (checkForLeaveEvent())
	{
		QToolButton::leaveEvent(event);
	}
}



void ToolButton::mousePressEvent(QMouseEvent* event)
{
	QToolTip::hideText();
	QToolButton::mousePressEvent(event);
}



QSize ToolButton::sizeHint() const
{
	const auto sh = QToolButton::sizeHint();
	return QSize(std::max<int>(sh.height() * 1.3, sh.width()), sh.height());
}



bool ToolButton::checkForLeaveEvent()
{
	if (QRect(mapToGlobal(QPoint(0, 0)), size()).contains(QCursor::pos()))
	{
		QTimer::singleShot(20, this, &ToolButton::checkForLeaveEvent);
	}
	else
	{
		QToolTip::hideText();
		return true;
	}

	return false;
}
