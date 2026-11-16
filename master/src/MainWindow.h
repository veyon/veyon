// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMainWindow>

#include "ComputerControlInterface.h"

class QButtonGroup;
class QToolButton;

class VeyonMaster;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(VeyonMaster& masterCore, QWidget* parent = nullptr);
	~MainWindow() override;

	static bool initAuthentication();
	static bool initAccessControl();

	VeyonMaster& masterCore()
	{
		return m_master;
	}

	void reloadSubFeatures();

	ComputerControlInterfaceList selectedComputerControlInterfaces() const;

protected:
	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;
	void keyPressEvent(QKeyEvent* e) override;


private Q_SLOTS:
	void showAboutDialog();

private:
	static int buttonId(const Feature& feature) // clazy:exclude=qt6-qhash-signature
	{
		return int(qHash(feature.uid()));
	}

	static constexpr const char* originalSizePropertyName()
	{
		return "originalSize";
	}

	void addFeaturesToToolBar();
	void addSubFeaturesToToolButton(QToolButton* button, const Feature& parentFeature);

	void updateModeButtonGroup();

	void loadComputerPositions();
	void saveComputerPositions();

	Ui::MainWindow* ui;

	VeyonMaster& m_master;

	QButtonGroup* m_modeGroup;
};
