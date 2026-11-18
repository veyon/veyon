// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "ComputerMonitoringModel.h"
#include "ComputerMonitoringView.h"
#include "FeatureManager.h"
#include "UserConfig.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"


ComputerMonitoringView::ComputerMonitoringView() :
	m_master(VeyonCore::instance()->findChild<VeyonMaster*>())
{
	m_autoAdjustIconSize = VeyonCore::config().autoAdjustMonitoringIconSize() ||
		master()->userConfig().autoAdjustMonitoringIconSize();

	m_iconSizeAutoAdjustTimer.setInterval(IconSizeAdjustDelay);
	m_iconSizeAutoAdjustTimer.setSingleShot(true);
}



void ComputerMonitoringView::initializeView(QObject* self)
{
	const auto autoAdjust = [this]() { initiateIconSizeAutoAdjust(); };

	QObject::connect(&m_iconSizeAutoAdjustTimer, &QTimer::timeout, self, [this]() { performIconSizeAutoAdjust(); });
	QObject::connect(dataModel(), &ComputerMonitoringModel::rowsInserted, self, autoAdjust);
	QObject::connect(dataModel(), &ComputerMonitoringModel::rowsRemoved, self, autoAdjust);
	QObject::connect(&m_master->computerControlListModel(), &ComputerControlListModel::computerScreenSizeChanged, self,
					 [this]() { updateIconSize(); });

	setColors(VeyonCore::config().computerMonitoringBackgroundColor(),
			  VeyonCore::config().computerMonitoringTextColor());

	setComputerScreenSize(m_master->userConfig().monitoringScreenSize());
	updateIconSize();

	loadComputerPositions(m_master->userConfig().computerPositions());
	setUseCustomComputerPositions(m_master->userConfig().useCustomComputerPositions());
}



void ComputerMonitoringView::saveConfiguration()
{
	m_master->userConfig().setFilterPoweredOnComputers(dataModel()->stateFilter() != ComputerControlInterface::State::None);
	m_master->userConfig().setFilterComputersWithLoggedOnUsers(dataModel()->filterNonEmptyUserLoginNames());
	m_master->userConfig().setComputerPositions(saveComputerPositions());
	m_master->userConfig().setUseCustomComputerPositions(useCustomComputerPositions());
}



ComputerMonitoringModel* ComputerMonitoringView::dataModel() const
{
	return m_master->computerMonitoringModel();
}



QString ComputerMonitoringView::searchFilter() const
{
	return dataModel()->filterRegularExpression().pattern();
}



void ComputerMonitoringView::setSearchFilter(const QString& searchFilter)
{
	dataModel()->setFilterRegularExpression(searchFilter);
}



void ComputerMonitoringView::setFilterPoweredOnComputers(bool enabled)
{
	dataModel()->setStateFilter(enabled ? ComputerControlInterface::State::Connected :
										  ComputerControlInterface::State::None);
}



void ComputerMonitoringView::setFilterComputersWithLoggedOnUsers(bool enabled)
{
	dataModel()->setFilterNonEmptyUserLoginNames(enabled);
}



void ComputerMonitoringView::setComputerScreenSize(int size)
{
	if (m_computerScreenSize != size)
	{
		const auto minSize = MinimumComputerScreenSize;
		const auto maxSize = MaximumComputerScreenSize;
		size = qBound<int>(minSize, size, maxSize);

		m_computerScreenSize = size;

		m_master->userConfig().setMonitoringScreenSize(size);

		m_master->computerControlListModel().updateComputerScreenSize();
	}
}



int ComputerMonitoringView::computerScreenSize() const
{
	return m_computerScreenSize;
}



void ComputerMonitoringView::setAutoAdjustIconSize(bool enabled)
{
	m_autoAdjustIconSize = enabled;

	if (m_autoAdjustIconSize)
	{
		performIconSizeAutoAdjust();
	}
}



bool ComputerMonitoringView::performIconSizeAutoAdjust()
{
	m_iconSizeAutoAdjustTimer.stop();

	return m_autoAdjustIconSize && dataModel()->rowCount() > 0;
}



void ComputerMonitoringView::initiateIconSizeAutoAdjust()
{
	m_iconSizeAutoAdjustTimer.start();
}



void ComputerMonitoringView::updateIconSize()
{
	setIconSize(m_master->computerControlListModel().computerScreenSize());
}



void ComputerMonitoringView::runFeature(const Feature& feature)
{
	auto computerControlInterfaces = selectedComputerControlInterfaces();
	if (computerControlInterfaces.isEmpty())
	{
		computerControlInterfaces = m_master->filteredComputerControlInterfaces();
	}

	runFeatureOnComputers(feature, computerControlInterfaces);
}

void ComputerMonitoringView::runFeatureOnComputers(const Feature& feature, const ComputerControlInterfaceList& computerControlInterfaces, bool toggleActiveMode)
{
	if (computerControlInterfaces.isEmpty()) return;
	const auto alreadyActive = toggleActiveMode && feature.testFlag(Feature::Flag::Mode) &&
		std::all_of(computerControlInterfaces.cbegin(), computerControlInterfaces.cend(), [this, &feature](const auto& computer) {
			return isFeatureOrRelatedFeatureActive({computer}, feature.uid());
		});

	if (feature.testFlag(Feature::Flag::Mode))
	{
		for (const auto& currentFeature : m_master->features())
		{
			// stop already active or all other active mode features
			if (currentFeature.testFlag(Feature::Flag::Mode) && (alreadyActive || currentFeature != feature))
			{
				VeyonCore::featureManager().stopFeature(*m_master, currentFeature, computerControlInterfaces);

				const auto subFeatures = m_master->subFeatures(currentFeature.uid());
				for (const auto& subFeature : subFeatures)
				{
					VeyonCore::featureManager().stopFeature(*m_master, subFeature, computerControlInterfaces);
				}
			}
		}
	}

	if (alreadyActive == false)
	{
		VeyonCore::featureManager().startFeature(*m_master, feature, computerControlInterfaces);
	}
}



bool ComputerMonitoringView::isFeatureOrRelatedFeatureActive(const ComputerControlInterfaceList& computerControlInterfaces,
															 Feature::Uid featureUid) const
{
	const auto& relatedFeatures = VeyonCore::featureManager().relatedFeatures( featureUid );
	const auto metaUid = VeyonCore::featureManager().metaFeatureUid(featureUid);

	for (const auto& controlInterface : computerControlInterfaces)
	{
		if( controlInterface->activeFeatures().contains( featureUid ) ||
			(!metaUid.isNull() && controlInterface->activeFeatures().contains(metaUid)) ||
			std::find_if( relatedFeatures.begin(), relatedFeatures.end(),
						  [&controlInterface, featureUid](const auto& relatedFeature) {
							  return relatedFeature.parentUid() == featureUid && controlInterface->activeFeatures().contains(relatedFeature.uid());
						  } )
			!= relatedFeatures.end() )
		{
			return true;
		}
	}

	return false;
}
