/*
 * MasterProcess.h - MasterProcess which manages (GUI) slave apps
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef _MASTER_PROCESS_H
#define _MASTER_PROCESS_H

#include "Ipc/Master.h"


class MasterProcess : protected Ipc::Master
{
	Q_OBJECT
public:
	MasterProcess();
	virtual ~MasterProcess();

	enum AccessDialogResult
	{
		AccessYes,
		AccessNo,
		AccessAlways,
		AccessNever
	} ;


public slots:
	void startDemo( const QString &masterHost, bool fullscreen);
	void stopDemo();

	void lockDisplay();
	void unlockDisplay();

	void messageBox( const QString &msg );
	void setSystemTrayToolTip( const QString &tooltip );
	void systemTrayMessage( const QString &title, const QString &msg );

	AccessDialogResult showAccessDialog( const QString &host );


private:
	virtual bool handleMessage( const Ipc::Msg &m );

} ;


#endif
