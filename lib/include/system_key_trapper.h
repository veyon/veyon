/*
 * system_key_trapper.h - class for trapping system-keys and -key-sequences
 *                        such as Alt+Ctrl+Del, Alt+Tab etc.
 *           
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifndef _SYSTEM_KEY_TRAPPER_H
#define _SYSTEM_KEY_TRAPPER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "italc_rfb_ext.h"

#include <QtCore/QMutex>
#include <QtCore/QObject>


class systemKeyTrapper : public QObject
{
	Q_OBJECT
public:
	enum trappedKeys
	{
		None,
		AltCtrlDel,
		AltTab,
		AltEsc,
		AltSpace,
		AltF4,
		CtrlEsc,
		MetaKey
	} ;


	systemKeyTrapper( bool _enable = TRUE );
	~systemKeyTrapper();

	void setEnabled( bool _on );
	bool isEnabled( void ) const
	{
		return( m_enabled );
	}

	void disableAllKeys( bool _on );


private:
	static QMutex s_refCntMutex;
	static int s_refCnt;

	bool m_enabled;


private slots:
	void checkForTrappedKeys( void );


signals:
	void keyEvent( Q_UINT32, bool );

} ;


#endif

