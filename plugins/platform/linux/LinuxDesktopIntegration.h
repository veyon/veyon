/*
 * LinuxDesktopIntegration.h - declaration of LinuxDesktopIntegration class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

class LinuxDesktopIntegration
{
public:
	class KDE {
	public:
		enum ShutdownConfirm {
			ShutdownConfirmDefault = -1,
			ShutdownConfirmNo = 0,
			ShutdownConfirmYes = 1
		};
		enum ShutdownMode {
			ShutdownModeDefault = -1,
			ShutdownModeSchedule = 0,
			ShutdownModeTryNow = 1,
			ShutdownModeForceNow = 2,
			ShutdownModeInteractive = 3
		};
		enum ShutdownType {
			ShutdownTypeDefault = -1,
			ShutdownTypeNone = 0,
			ShutdownTypeReboot = 1,
			ShutdownTypeHalt = 2,
			ShutdownTypeLogout = 3
		};
	};

	class Gnome {
	public:
		enum GsmManagerLogoutMode {
			GSM_MANAGER_LOGOUT_MODE_NORMAL = 0,
			GSM_MANAGER_LOGOUT_MODE_NO_CONFIRMATION,
			GSM_MANAGER_LOGOUT_MODE_FORCE
		} ;
	};

	class Mate {
	public:
		enum {
			GSM_LOGOUT_MODE_NORMAL = 0,
			GSM_LOGOUT_MODE_NO_CONFIRMATION,
			GSM_LOGOUT_MODE_FORCE
		};
	};
};

