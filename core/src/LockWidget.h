// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
#include <QPixmap>
#include <QWindow>
#include <QPointer>
#include "VeyonCore.h"
class QQmlApplicationEngine;
class VEYON_CORE_EXPORT LockWidget : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int mode READ mode CONSTANT)
public:
	enum Mode { DesktopVisible, BackgroundPixmap, NoBackground, ClassroomLock };
	explicit LockWidget(Mode mode, const QPixmap& background = {}, QObject* parent = nullptr);
	~LockWidget() override;
	int mode() const { return m_mode; }
	QWindow* window() const { return m_window; }
	bool hasQmlContent() const { return m_hasQmlContent; }
private:
	void cleanup();
	Mode m_mode;
	QQmlApplicationEngine* m_engine = nullptr;
	QPointer<QWindow> m_window;
	bool m_hasQmlContent{false};
	bool m_locked{false};
};
