// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class QLineEdit;

class VEYON_CORE_EXPORT FileSystemBrowser
{
public:
	enum BrowseModes {
		ExistingDirectory,
		ExistingFile,
		SaveFile,
		NumBrowseModes
	};
	using BrowseMode = BrowseModes;

	explicit FileSystemBrowser(BrowseMode m, QWidget* parent) :
		m_parent(parent),
		m_browseMode(m),
		m_expandPath(true),
		m_shrinkPath(true)
	{
	}

	void setExpandPath(bool enabled)
	{
		m_expandPath = enabled;
	}

	void setShrinkPath(bool enabled)
	{
		m_shrinkPath = enabled;
	}

	QString exec(const QString& path, const QString& title = {},
				 const QString& filter = {});
	void exec(QLineEdit* lineEdit, const QString& title = {},
			  const QString& filter = {});


private:
	QWidget* m_parent;
	BrowseMode m_browseMode;
	bool m_expandPath;
	bool m_shrinkPath;
};
