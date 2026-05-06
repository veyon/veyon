/*
 * WindowsDeviceFunctions.cpp - implementation of WindowsDeviceFunctions class
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <cfgmgr32.h>

#include "VeyonCore.h"
#include "WindowsDeviceFunctions.h"


static QString getDeviceInstanceId(SP_DEVINFO_DATA& deviceInfoData)
{
	wchar_t deviceId[MAX_DEVICE_ID_LEN] = {};
	const auto cr = CM_Get_Device_IDW(deviceInfoData.DevInst, deviceId, MAX_DEVICE_ID_LEN, 0);
	if (cr != CR_SUCCESS)
	{
		return {};
	}

	return QString::fromWCharArray(deviceId);
}


static QString getRegistryPropertyString(HDEVINFO deviceInfoSet,
										 SP_DEVINFO_DATA& deviceInfoData,
										 DWORD property)
{
	DWORD propertyType = 0;
	DWORD requiredSize = 0;

	SetupDiGetDeviceRegistryPropertyW(deviceInfoSet,
									  &deviceInfoData,
									  property,
									  &propertyType,
									  nullptr,
									  0,
									  &requiredSize);

	if (requiredSize == 0)
	{
		return {};
	}

	QByteArray buffer(int(requiredSize), '\0');

	if (!SetupDiGetDeviceRegistryPropertyW(deviceInfoSet,
										   &deviceInfoData,
										   property,
										   &propertyType,
										   reinterpret_cast<PBYTE>(buffer.data()),
										   static_cast<DWORD>(buffer.size()),
										   nullptr))
	{
		return {};
	}

	if (propertyType != REG_SZ && propertyType != REG_MULTI_SZ && propertyType != REG_EXPAND_SZ)
	{
		return {};
	}

	const auto wideBuffer = reinterpret_cast<const wchar_t*>(buffer.constData());

	if (propertyType == REG_MULTI_SZ)
	{
		QStringList values;
		const wchar_t* current = wideBuffer;
		while (*current != L'\0')
		{
			values.append(QString::fromWCharArray(current));
			current += wcslen(current) + 1;
		}
		return values.join(u'\n');
	}

	return QString::fromWCharArray(wideBuffer);
}



static QString bestDeviceName(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfoData)
{
	const auto friendlyName = getRegistryPropertyString(deviceInfoSet, deviceInfoData, SPDRP_FRIENDLYNAME);
	if (!friendlyName.isEmpty())
	{
		return friendlyName;
	}

	const auto description = getRegistryPropertyString(deviceInfoSet, deviceInfoData, SPDRP_DEVICEDESC);
	if (!description.isEmpty())
	{
		return description;
	}

	return QStringLiteral("<unknown>");
}



static bool looksLikeTouchpad(const QString& name,
							  const QString& hardwareIds,
							  const QString& compatibleIds,
							  const QString& manufacturer)
{
	const auto nameText = name.toLower();
	const auto hardwareText = hardwareIds.toLower();
	const auto compatibleText = compatibleIds.toLower();
	const auto manufacturerText = manufacturer.toLower();

	const QString idText = hardwareText + u'\n' + compatibleText;
	const QString allText = nameText + u'\n' + manufacturerText + u'\n' + idText;

	const bool mentionsTouchpad =
			nameText.contains(QStringLiteral("touchpad")) ||
			nameText.contains(QStringLiteral("touch pad")) ||
			nameText.contains(QStringLiteral("precision touchpad")) ||
			nameText.contains(QStringLiteral("precision touch pad"));

	const bool mentionsKnownTouchpadVendor =
			manufacturerText.contains(QStringLiteral("synaptics")) ||
			manufacturerText.contains(QStringLiteral("elan")) ||
			manufacturerText.contains(QStringLiteral("alps")) ||
			manufacturerText.contains(QStringLiteral("sentelic")) ||
			manufacturerText.contains(QStringLiteral("cypress")) ||
			nameText.contains(QStringLiteral("synaptics")) ||
			nameText.contains(QStringLiteral("elan")) ||
			nameText.contains(QStringLiteral("alps")) ||
			nameText.contains(QStringLiteral("sentelic")) ||
			nameText.contains(QStringLiteral("cypress"));

	const bool mentionsI2cHid =
			allText.contains(QStringLiteral("i2c hid")) ||
			allText.contains(QStringLiteral("hidi2c"));

	const bool looksLikeTouchscreenOrPen =
			allText.contains(QStringLiteral("touch screen")) ||
			allText.contains(QStringLiteral("touchscreen")) ||
			allText.contains(QStringLiteral("digitizer")) ||
			allText.contains(QStringLiteral("pen")) ||
			allText.contains(QStringLiteral("stylus"));

	const bool looksLikeMouseOnly =
			nameText.contains(QStringLiteral("mouse")) &&
			!mentionsTouchpad;

	if (looksLikeTouchscreenOrPen || looksLikeMouseOnly)
	{
		return false;
	}

	if (mentionsTouchpad)
	{
		return true;
	}

	if (mentionsKnownTouchpadVendor && mentionsI2cHid)
	{
		return true;
	}

	return false;
}



static bool looksLikeTouchscreen(const QString& name,
								 const QString& hardwareIds,
								 const QString& compatibleIds,
								 const QString& manufacturer)
{
	const auto nameText = name.toLower();
	const auto hardwareText = hardwareIds.toLower();
	const auto compatibleText = compatibleIds.toLower();
	const auto manufacturerText = manufacturer.toLower();

	const QString idText = hardwareText + u'\n' + compatibleText;
	const QString allText = nameText + u'\n' + manufacturerText + u'\n' + idText;

	const bool mentionsTouchscreen =
			nameText.contains(QStringLiteral("touch screen")) ||
			nameText.contains(QStringLiteral("touchscreen")) ||
			nameText.contains(QStringLiteral("hid-compliant touch screen")) ||
			nameText.contains(QStringLiteral("hid compliant touch screen"));

	const bool mentionsTouchscreenInIds =
			idText.contains(QStringLiteral("touch screen")) ||
			idText.contains(QStringLiteral("touchscreen"));

	const bool mentionsDigitizer =
			nameText.contains(QStringLiteral("digitizer")) ||
			manufacturerText.contains(QStringLiteral("digitizer"));

	const bool looksLikeTouchpad =
			allText.contains(QStringLiteral("touchpad")) ||
			allText.contains(QStringLiteral("touch pad")) ||
			allText.contains(QStringLiteral("precision touchpad")) ||
			allText.contains(QStringLiteral("precision touch pad"));

	const bool looksLikePenOnly =
			allText.contains(QStringLiteral("pen")) ||
			allText.contains(QStringLiteral("stylus"));

	const bool looksLikeMouse =
			nameText.contains(QStringLiteral("mouse"));

	if (looksLikeTouchpad || looksLikeMouse)
	{
		return false;
	}

	if (mentionsTouchscreen || mentionsTouchscreenInIds)
	{
		return true;
	}

	if (mentionsDigitizer && !looksLikePenOnly)
	{
		return true;
	}

	return false;
}



bool WindowsDeviceFunctions::setDeviceState(const Device& device, State state)
{
	if (device.instanceId.isEmpty())
	{
		vWarning() << "empty instance ID";
		return false;
	}

	const HDEVINFO deviceInfoSet =
			SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);

	if (deviceInfoSet == INVALID_HANDLE_VALUE)
	{
		vWarning() << "SetupDiGetClassDevsW failed";
		return false;
	}

	bool found = false;
	bool success = false;

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); ++index)
	{
		const auto currentInstanceId = getDeviceInstanceId(deviceInfoData);
		if (currentInstanceId.compare(device.instanceId, Qt::CaseInsensitive) != 0)
		{
			continue;
		}

		found = true;

		SP_PROPCHANGE_PARAMS params = {};
		params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		params.StateChange = state == State::Disabled ? DICS_DISABLE : DICS_ENABLE;
		params.Scope = DICS_FLAG_GLOBAL;
		params.HwProfile = 0;

		if (!SetupDiSetClassInstallParamsW(deviceInfoSet,
										   &deviceInfoData,
										   &params.ClassInstallHeader,
										   sizeof(params)))
		{
			vWarning() << "SetupDiSetClassInstallParamsW failed for"
					   << device.instanceId << "error" << GetLastError();
			break;
		}

		if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, deviceInfoSet, &deviceInfoData))
		{
			vWarning() << "SetupDiCallClassInstaller failed for"
					   << device.instanceId << "error" << GetLastError();
			break;
		}

		success = true;
		break;
	}

	SetupDiDestroyDeviceInfoList(deviceInfoSet);

	if (!found)
	{
		vWarning() << "device not found:" << device.instanceId;
	}

	return found && success;
}



bool WindowsDeviceFunctions::setDevicesState(const DeviceList& devices, State state)
{
	bool success = true;

	for (const auto& device : devices)
	{
		success &= setDeviceState(device, state);
	}

	return success;
}



WindowsDeviceFunctions::DeviceList WindowsDeviceFunctions::findDevicesByClass(const GUID& classGuid)
{
	DeviceList devices;
	QSet<QString> seenInstanceIds;

	const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&classGuid, nullptr, nullptr, DIGCF_PRESENT);

	if (deviceInfoSet == INVALID_HANDLE_VALUE)
	{
		vWarning() << "SetupDiGetClassDevsW failed";
		return devices;
	}

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); ++index)
	{
		const auto instanceId = getDeviceInstanceId(deviceInfoData);
		if (instanceId.isEmpty())
		{
			continue;
		}

		const auto normalizedInstanceId = instanceId.toLower();
		if (seenInstanceIds.contains(normalizedInstanceId))
		{
			continue;
		}

		seenInstanceIds.insert(normalizedInstanceId);
		devices.append({instanceId, bestDeviceName(deviceInfoSet, deviceInfoData)});
	}

	SetupDiDestroyDeviceInfoList(deviceInfoSet);
	return devices;
}



WindowsDeviceFunctions::DeviceList WindowsDeviceFunctions::findKeyboardDevices()
{
	return findDevicesByClass(GUID_DEVCLASS_KEYBOARD);
}



WindowsDeviceFunctions::DeviceList WindowsDeviceFunctions::findMouseDevices()
{
	return findDevicesByClass(GUID_DEVCLASS_MOUSE);
}



WindowsDeviceFunctions::DeviceList WindowsDeviceFunctions::findTouchDevices()
{
	DeviceList devices;
	QSet<QString> seenInstanceIds;

	const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVCLASS_HIDCLASS, nullptr, nullptr, DIGCF_PRESENT);

	if (deviceInfoSet == INVALID_HANDLE_VALUE)
	{
		vWarning() << "SetupDiGetClassDevsW failed";
		return devices;
	}

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); ++index)
	{
		const auto instanceId = getDeviceInstanceId(deviceInfoData);
		if (instanceId.isEmpty())
		{
			continue;
		}

		const auto name = bestDeviceName(deviceInfoSet, deviceInfoData);
		const auto hardwareIds = getRegistryPropertyString(deviceInfoSet, deviceInfoData, SPDRP_HARDWAREID);
		const auto compatibleIds = getRegistryPropertyString(deviceInfoSet, deviceInfoData, SPDRP_COMPATIBLEIDS);
		const auto manufacturer = getRegistryPropertyString(deviceInfoSet, deviceInfoData, SPDRP_MFG);

		const bool isTouchpad = looksLikeTouchpad(name, hardwareIds, compatibleIds, manufacturer);
		const bool isTouchscreen = looksLikeTouchscreen(name, hardwareIds, compatibleIds, manufacturer);

		if (!isTouchpad && !isTouchscreen)
		{
			continue;
		}

		const auto normalizedInstanceId = instanceId.toLower();
		if (seenInstanceIds.contains(normalizedInstanceId))
		{
			continue;
		}

		seenInstanceIds.insert(normalizedInstanceId);
		devices.append({instanceId, name});
	}

	SetupDiDestroyDeviceInfoList(deviceInfoSet);
	return devices;
}
