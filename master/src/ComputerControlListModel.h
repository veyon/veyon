// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QImage>

#include "ComputerControlInterface.h"
#include "ComputerListModel.h"

class VeyonMaster;

class ComputerControlListModel : public ComputerListModel
{
	Q_OBJECT
public:
	enum { SearchRole = Qt::UserRole + 50 };
	explicit ComputerControlListModel( VeyonMaster* masterCore, QObject* parent = nullptr );

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	void updateComputerScreenSize();

	QSize computerScreenSize() const
	{
		return m_computerScreenSize;
	}

	const ComputerControlInterfaceList& computerControlInterfaces() const
	{
		return m_computerControlInterfaces;
	}

	ComputerControlInterface::Pointer computerControlInterface(const QModelIndex& index) const;

	void reload();

Q_SIGNALS:
	void stateChanged(QModelIndex);
	void activeFeaturesChanged(QModelIndex);
	void computerScreenSizeChanged();

private:
	void update();

	QModelIndex interfaceIndex(ComputerControlInterface* controlInterface) const;
	QVariant uidRoleData(const ComputerControlInterface::Pointer& controlInterface) const;

	void updateState(const QModelIndex& index);
	void updateAccessControlDetails(const QModelIndex& index);
	void updateScreen(const QModelIndex& index);
	void updateActiveFeatures(const QModelIndex& index);
	void updateUser(const QModelIndex& index);
	void updateSessionInfo(const QModelIndex& index);

	void startComputerControlInterface(ComputerControlInterface* controlInterface);
	void stopComputerControlInterface(const ComputerControlInterface::Pointer& controlInterface);

	double averageAspectRatio() const;

	QImage scaleAndAlignIcon(const QImage& icon, QSize size) const;
	QImage computerDecorationRole(const ComputerControlInterface::Pointer& controlInterface) const;
	QString computerToolTipRole(const ComputerControlInterface::Pointer& controlInterface) const;
	QString computerDisplayRole(const ComputerControlInterface::Pointer& controlInterface) const;
	QString computerSortRole(const ComputerControlInterface::Pointer& controlInterface) const;
	static QString computerStateDescription(const ComputerControlInterface::Pointer& controlInterface);
	static QString userInformation(const ComputerControlInterface::Pointer& controlInterface);
	static QString activeFeaturesInformation(const ComputerControlInterface::Pointer& controlInterface);

	VeyonMaster* m_master;

	QImage m_iconHostOffline;
	QImage m_iconHostOnline;
	QImage m_iconHostNameResolutionFailed;
	QImage m_iconHostAccessDenied;
	QImage m_iconHostServiceError;

	QSize m_computerScreenSize{};

	ComputerControlInterfaceList m_computerControlInterfaces{};
};
