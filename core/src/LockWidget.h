// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPixmap>
#include <QWidget>

#include "VeyonCore.h"


class VEYON_CORE_EXPORT LockWidget : public QWidget
{
	Q_OBJECT
public:
	enum Mode {
		DesktopVisible,
		BackgroundPixmap,
		NoBackground
	};

	explicit LockWidget(Mode mode, const QPixmap& background = QPixmap(), QWidget* parent = nullptr);
	~LockWidget() override;


private:
	void paintEvent(QPaintEvent*) override;

	QPixmap m_background;
	Mode m_mode;
};
