/*
 * PowerControl.cpp - namespace PowerControl
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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
#include "PowerControl.h"

#ifdef VEYON_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef VEYON_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef VEYON_HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef VEYON_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef VEYON_HAVE_ERRNO_H
#include <errno.h>
#endif


namespace PowerControl
{

void broadcastWOLPacket( QString macAddress )
{
	const int PORT_NUM = 65535;
	const int MAC_SIZE = 6;
	const int OUTBUF_SIZE = MAC_SIZE*17;
	unsigned char mac[MAC_SIZE];
	char out_buf[OUTBUF_SIZE];

	if( macAddress.isEmpty() )
	{
		return;
	}

	const auto originalMacAddress = macAddress;

	// remove all possible delimiters
	macAddress.replace( ':', QStringLiteral("") );
	macAddress.replace( '-', QStringLiteral("") );
	macAddress.replace( '.', QStringLiteral("") );

	if( sscanf( macAddress.toUtf8().constData(),
				"%2x%2x%2x%2x%2x%2x",
				(unsigned int *) &mac[0],
				(unsigned int *) &mac[1],
				(unsigned int *) &mac[2],
				(unsigned int *) &mac[3],
				(unsigned int *) &mac[4],
				(unsigned int *) &mac[5] ) != MAC_SIZE )
	{
		qWarning() << "PowerControl::broadcastWOLPacket(): invalid MAC address" << originalMacAddress;
		return;
	}

	for( int i = 0; i < MAC_SIZE; ++i )
	{
		out_buf[i] = 0xff;
	}

	for( int i = 1; i < 17; ++i )
	{
		for(int j = 0; j < MAC_SIZE; ++j )
		{
			out_buf[i*MAC_SIZE+j] = mac[j];
		}
	}

	// UDP-broadcast the MAC-address
	unsigned int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	struct sockaddr_in my_addr;
	my_addr.sin_family	  = AF_INET;			// Address family to use
	my_addr.sin_port		= htons( PORT_NUM );	// Port number to use
	my_addr.sin_addr.s_addr = inet_addr( "255.255.255.255" ); // send to
								  // IP_ADDR

	int optval = 1;
	if( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char *) &optval,
							sizeof( optval ) ) < 0 )
	{
		qCritical( "can't set sockopt (%d).", errno );
		return;
	}

	if( sendto( sock, out_buf, sizeof( out_buf ), 0,
			(struct sockaddr*) &my_addr, sizeof( my_addr ) ) != sizeof( out_buf ) )
	{
		qCritical( "PowerControl::broadcastWOLPacket(): error while sending WOL packet (%d)", errno );
	}

#ifdef VEYON_BUILD_WIN32
	closesocket( sock );
#else
	close( sock );
#endif


#if 0
#ifdef VEYON_BUILD_LINUX
	QProcess::startDetached( "etherwake " + _mac );
#endif
#endif
}


} // end of namespace PowerControl
