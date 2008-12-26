/*
 * ivs.h - class IVS, a VNC-server-abstraction for platform-independent
 *         VNC-server-usage
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


#ifndef _IVS_H
#define _IVS_H

#include <QtCore/QThread>



class IVS : public QThread
{
public:
	IVS( const quint16 _ivs_port, int _argc, char * * _argv,
						bool _no_threading = FALSE );
	virtual ~IVS();

	quint16 serverPort( void ) const
	{
		return( m_port );
	}

	bool runningInSeparateProcess( void ) const
	{
		return( m_runningInSeparateProcess );
	}

#ifdef ITALC_BUILD_LINUX
	void restart( void )
	{
		m_restart = TRUE;
	}
#endif


private:
	virtual void run( void );

	int m_argc;
	char * * m_argv;

	quint16 m_port;
	bool m_runningInSeparateProcess;

#ifdef ITALC_BUILD_LINUX
	bool m_restart;
#endif

} ;


#endif

