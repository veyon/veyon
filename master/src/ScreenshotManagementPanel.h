// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFileSystemWatcher>
#include <QStringListModel>
#include <QTimer>
#include <QWidget>

class QModelIndex;
class Screenshot;

namespace Ui {
class ScreenshotManagementPanel;
}

class ScreenshotManagementPanel : public QWidget
{
	Q_OBJECT
public:
	explicit ScreenshotManagementPanel(QWidget* parent = nullptr);
	~ScreenshotManagementPanel() override;

	void setPreview(const Screenshot& screenshot);

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	void updateModel();

	QString filePath(const QModelIndex& index) const;

	void updateScreenshot(const QModelIndex& index);
	void screenshotDoubleClicked(const QModelIndex& index);

	void showScreenshot();
	void deleteScreenshot();

	Ui::ScreenshotManagementPanel* ui;

	QStringListModel m_model{this};
	QFileSystemWatcher m_fsWatcher{this};

	QTimer m_reloadTimer{this};

	static constexpr auto FsModelResetDelay = 1000;
};
