// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFileDialog>
#include <QLineEdit>

#include "FileSystemBrowser.h"
#include "Filesystem.h"


QString FileSystemBrowser::exec(const QString& path,
								const QString& title,
								const QString& filter)
{
	QString browsePath = path;
	if (m_expandPath)
	{
		browsePath = VeyonCore::filesystem().expandPath(browsePath);
	}

	switch (m_browseMode)
	{
	case ExistingDirectory:
		if (!QFileInfo(browsePath).isDir())
		{
			browsePath = QDir::homePath();
		}
		break;
	case ExistingFile:
	case SaveFile:
		if (QFileInfo(browsePath).isFile())
		{
			browsePath = QFileInfo(browsePath).absolutePath();
		}
		else
		{
			browsePath = QDir::homePath();
		}
		break;

	default: break;
	}

	QString chosenPath;
	switch (m_browseMode)
	{
	case ExistingDirectory:
		chosenPath = QFileDialog::getExistingDirectory(m_parent, title,
													   browsePath,
													   QFileDialog::ShowDirsOnly |
														   QFileDialog::DontResolveSymlinks);
		break;
	case ExistingFile:
		chosenPath = QFileDialog::getOpenFileName(m_parent, title, browsePath, filter);
		break;
	case SaveFile:
		chosenPath = QFileDialog::getSaveFileName(m_parent, title, browsePath, filter);
		break;

	default: break;
	}

	if (!chosenPath.isEmpty())
	{
		if (m_shrinkPath)
		{
			return VeyonCore::filesystem().shrinkPath(chosenPath);
		}
		return chosenPath;
	}

	return path;
}



void FileSystemBrowser::exec(QLineEdit* lineEdit, const QString& title,
							 const QString& filter)
{
	lineEdit->setText(exec(lineEdit->text(), title, filter));
}
