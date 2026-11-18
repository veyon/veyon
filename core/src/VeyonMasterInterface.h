// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ComputerControlInterface.h"
#include "Configuration/Object.h"

class BuiltinFeatures;
class ComputerControlInterface;
class QWidget;
class QWindow;

class VEYON_CORE_EXPORT VeyonMasterInterface : public QObject
{
	Q_OBJECT
public:
	explicit VeyonMasterInterface(QObject* parent) :
		QObject(parent)
	{
	}

	~VeyonMasterInterface() override = default;

	virtual QWidget* mainWindow() = 0;
	virtual QWindow* mainQmlWindow() { return nullptr; }
	virtual Configuration::Object* userConfigurationObject() = 0;
	virtual void reloadSubFeatures() = 0;

	virtual ComputerControlInterface& localSessionControlInterface() = 0;

	virtual const ComputerControlInterfaceList& allComputerControlInterfaces() const = 0;
	virtual ComputerControlInterfaceList selectedComputerControlInterfaces() const = 0;
	virtual ComputerControlInterfaceList filteredComputerControlInterfaces() const = 0;

Q_SIGNALS:
	void computerControlListModelAboutToReset();
};
