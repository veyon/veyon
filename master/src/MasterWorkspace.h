/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <QWidget>
#include "ComputerControlInterface.h"
class QLabel;
class QLineEdit;
class QSplitter;
class QToolButton;
class QMenu;
class QVBoxLayout;
class ComputerMonitoringWidget;
class VeyonMaster;
class MasterWorkspace : public QWidget
{
	Q_OBJECT
public:
	MasterWorkspace(VeyonMaster& master, ComputerMonitoringWidget* monitoring,
		QSplitter* splitter, QLineEdit* search, QWidget* parent = nullptr);
	ComputerControlInterfaceList actionTargets() const;
	void reloadFeatures();
private:
	void updateOverview();
	void updateDetails();
	void runFeature(const QString& name, bool selectedOnly = false);
	void populateFeatures(QMenu* menu, const QStringList& names, bool selectedOnly = false);
	void addFeature(QMenu* menu, const Feature& feature, bool selectedOnly, bool flat = false);
	void populateMore(QMenu* menu);
	QToolButton* featureButton(QWidget* parent, const QString& name, const QString& icon, const QString& label = {});
	VeyonMaster& m_master;
	ComputerMonitoringWidget* m_monitoring;
	QLabel* m_summary{};
	QLabel* m_statusValues[3]{};
	QWidget* m_banner{};
	QLabel* m_bannerDescription{};
	QToolButton* m_bannerAction{};
	QToolButton* m_scopeAll{};
	QToolButton* m_scopeSelected{};
	QToolButton* m_demo{};
	QToolButton* m_lock{};
	QToolButton* m_message{};
	QWidget* m_details{};
	QLabel* m_detailsTitle{};
	QLabel* m_preview{};
	QLabel* m_detailValues[4]{};
	QLabel* m_detailState{};
	QLabel* m_empty{};
	QToolButton* m_view{};
	QToolButton* m_control{};
};
