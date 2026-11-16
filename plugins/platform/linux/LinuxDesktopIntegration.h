// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class LinuxDesktopIntegration
{
public:
	class KDE
	{
	public:
		enum ShutdownConfirm : int {
			ShutdownConfirmDefault = -1,
			ShutdownConfirmNo = 0,
			ShutdownConfirmYes = 1
		};
		enum ShutdownMode : int {
			ShutdownModeDefault = -1,
			ShutdownModeSchedule = 0,
			ShutdownModeTryNow = 1,
			ShutdownModeForceNow = 2,
			ShutdownModeInteractive = 3
		};
		enum ShutdownType : int {
			ShutdownTypeDefault = -1,
			ShutdownTypeNone = 0,
			ShutdownTypeReboot = 1,
			ShutdownTypeHalt = 2,
			ShutdownTypeLogout = 3
		};
	};

	class Gnome
	{
	public:
		enum GsmLogoutMode : uint {
			GSM_MANAGER_LOGOUT_MODE_NORMAL = 0,
			GSM_MANAGER_LOGOUT_MODE_NO_CONFIRMATION,
			GSM_MANAGER_LOGOUT_MODE_FORCE
		};
	};

	class Mate
	{
	public:
		enum GsmLogoutMode : uint {
			GSM_LOGOUT_MODE_NORMAL = 0,
			GSM_LOGOUT_MODE_NO_CONFIRMATION,
			GSM_LOGOUT_MODE_FORCE
		};
	};
};
