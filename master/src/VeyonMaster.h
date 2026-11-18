// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "Feature.h"
#include "VeyonMasterInterface.h"

class QModelIndex;

class BuiltinFeatures;
class ComputerControlListModel;
class ComputerManager;
class ComputerMonitoringModel;
class MainWindow;
class UserConfig;

class VeyonMaster : public VeyonMasterInterface
{
	Q_OBJECT
public:
	explicit VeyonMaster(QObject* parent = nullptr);
	~VeyonMaster() override;

	UserConfig& userConfig()
	{
		return *m_userConfig;
	}

	ComputerManager& computerManager()
	{
		return *m_computerManager;
	}

	ComputerControlListModel& computerControlListModel()
	{
		return *m_computerControlListModel;
	}

	ComputerMonitoringModel* computerMonitoringModel() const
	{
		return m_computerMonitoringModel;
	}

	const FeatureList& features() const
	{
		return m_features;
	}

	FeatureList subFeatures(Feature::Uid parentFeatureUid) const;

	FeatureList allFeatures() const;

	const Feature::Uid& currentMode() const
	{
		return m_currentMode;
	}

	QWidget* mainWindow() override;
	QWindow* mainQmlWindow() override;
	Configuration::Object* userConfigurationObject() override;
	void reloadSubFeatures() override;

	ComputerControlInterface& localSessionControlInterface() override
	{
		return m_localSessionControlInterface;
	}

	const ComputerControlInterfaceList& allComputerControlInterfaces() const override;

	ComputerControlInterfaceList selectedComputerControlInterfaces() const override;

	ComputerControlInterfaceList filteredComputerControlInterfaces() const override;

public Q_SLOTS:
	void runFeature(const Feature& feature);
	void enforceDesignatedMode(const QModelIndex& index);
	void stopAllFeatures(const ComputerControlInterfaceList& computerControlInterfaces);

private:
	void shutdown();

	FeatureList featureList() const;

	UserConfig* m_userConfig;
	const FeatureList m_features;
	ComputerManager* m_computerManager;
	ComputerControlListModel* m_computerControlListModel;
	ComputerMonitoringModel* m_computerMonitoringModel;

	ComputerControlInterface m_localSessionControlInterface;

	MainWindow* m_mainWindow;

	Feature::Uid m_currentMode;
};
