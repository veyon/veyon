// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMainWindow>

class QAbstractButton;

namespace Ui {
class MainWindow;
}

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

	void reset(bool onlyUI = false);
	void apply();

protected:
	void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
	void configurationChanged();
	void resetOrApply(QAbstractButton* btn);
	void loadSettingsFromFile();
	void saveSettingsToFile();
	void resetConfiguration();
	void aboutVeyon();

private:
	void updateSizes();
	void updateView();

	void switchToStandardView();
	void switchToAdvancedView();

	void loadConfigurationPagePlugins();

	void closeEvent(QCloseEvent* closeEvent) override;

	void buildNavigation();
	void selectNavigationPage(QTreeWidgetItem* item);
	void updateStatus();
	void filterPages();
	QTreeWidget* m_navigation{};
	QLabel* m_heading{};
	QLabel* m_description{};
	QLabel* m_status{};
	QLabel* m_statusIndicator{};
	QPushButton* m_serviceAction{};
	QLabel* m_serviceStatus{};
	QLabel* m_authenticationStatus{};
	QLabel* m_directoryStatus{};
	QLabel* m_ldapStatus{};
	QLabel* m_accessStatus{};
	QLineEdit* m_pageSearch{};
	Ui::MainWindow* ui;
	bool m_configChanged;
};
