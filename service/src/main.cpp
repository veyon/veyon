/*
 * main.cpp - main file for Veyon Service
 *
 * Copyright (c) 2006-2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include "VeyonCore.h"

#include "WindowsService.h"
#include "VeyonConfiguration.h"
#include "VeyonServiceControl.h"


int main( int argc, char **argv )
{
	VeyonCore core( nullptr, QStringLiteral("Service") );

#ifdef VEYON_BUILD_WIN32
	return WindowsService( VeyonServiceControl::name() ).runAsService() ? 0 : -1;
#endif

	return 0;
}
