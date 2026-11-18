// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>

#include "ComputerControlInterface.h"

class ComputerMonitoringModel;
class VeyonMaster;

// clazy:excludeall=copyable-polymorphic

class ComputerMonitoringView
{
public:
	static constexpr int MinimumComputerScreenSize = 50;
	static constexpr int MaximumComputerScreenSize = 1000;
	static constexpr int DefaultComputerScreenSize = 190;

	static constexpr auto IncreaseIconSizeStepSize = 20;
	static constexpr auto DecreaseIconSizeMinimumStepSize = 5;
	static constexpr auto IconSizeAdjustDelay = 250;

	ComputerMonitoringView();
	virtual ~ComputerMonitoringView() = default;

	void initializeView(QObject* self);

	void saveConfiguration();

	ComputerMonitoringModel* dataModel() const;

	virtual ComputerControlInterfaceList selectedComputerControlInterfaces() const = 0;
	ComputerControlInterfaceList filteredComputerControlInterfaces() const;
	void runFeatureOnComputers(const Feature& feature, const ComputerControlInterfaceList& computers, bool toggleActiveMode = true);

	QString searchFilter() const;
	void setSearchFilter(const QString& searchFilter);

	void setFilterPoweredOnComputers(bool enabled);
	void setFilterComputersWithLoggedOnUsers(bool enabled);

	void setComputerScreenSize(int size);
	int computerScreenSize() const;

	virtual void alignComputers() = 0;

	bool autoAdjustIconSize() const
	{
		return m_autoAdjustIconSize;
	}
	void setAutoAdjustIconSize(bool enabled);

protected:
	virtual void setColors(const QColor& backgroundColor, const QColor& textColor) = 0;
	virtual QJsonArray saveComputerPositions() = 0;
	virtual bool useCustomComputerPositions() = 0;
	virtual void loadComputerPositions(const QJsonArray& positions) = 0;
	virtual void setUseCustomComputerPositions(bool enabled) = 0;
	virtual void setIconSize(const QSize& size) = 0;

	virtual bool performIconSizeAutoAdjust();

	void initiateIconSizeAutoAdjust();
	void updateIconSize();

	VeyonMaster* master() const
	{
		return m_master;
	}

	void runFeature(const Feature& feature);

	bool isFeatureOrRelatedFeatureActive(const ComputerControlInterfaceList& computerControlInterfaces,
										 Feature::Uid featureUid) const;

private:
	VeyonMaster* m_master{nullptr};
	int m_computerScreenSize{DefaultComputerScreenSize};

	bool m_autoAdjustIconSize{false};
	QTimer m_iconSizeAutoAdjustTimer{};
};
