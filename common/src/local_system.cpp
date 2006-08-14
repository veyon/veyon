/*
 * local_system.cpp - namespace localSystem, providing an interface for
 *                    transparent usage of operating-system-dependent functions
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>


#ifdef BUILD_WIN32

#include <windows.h>

#else

/*
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif

#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>
#endif

#ifdef HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#endif

#define USE_SEND
*/
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif


#endif


#include "local_system.h"



namespace localSystem
{


void initialize( void )
{
	QCoreApplication::setOrganizationName( "EasySchoolSolutions" );
	QCoreApplication::setOrganizationDomain( "ess.org" );
	QCoreApplication::setApplicationName( "iTALC" );
}




void sleep( const int _ms )
{
#ifdef BUILD_WIN32
	Sleep( static_cast<unsigned int>( _ms ) );
#else
	struct timespec ts = { _ms / 1000, ( _ms % 1000 ) * 1000 * 1000 } ;
	nanosleep( &ts, NULL );
#endif
}




void execInTerminal( const QString & _cmds )
{
	QProcess::startDetached(
#ifdef BUILD_WIN32
			"cmd " +
#else
			"xterm -e " +
#endif
			_cmds );
}




void sendWakeOnLANPacket( const QString & _mac )
{
#ifdef BUILD_WIN32

#warning TODO: add according WOL-code for win32

#else
	QProcess::startDetached( "etherwake " + _mac );
#if 0
	const char * ifname = "eth0";

	unsigned char outpack[1000];

	int opt_no_src_addr = 0, opt_broadcast = 0;

#if defined(PF_PACKET)
	struct sockaddr_ll whereto;
#else
	struct sockaddr whereto;	// who to wake up
#endif
	struct ether_addr eaddr;


	const int wol_passwd_size = 6;
	unsigned char wol_passwd[wol_passwd_size];

	const char * mac = _mac.toAscii().constData();

	if( sscanf( mac, "%2x:%2x:%2x:%2x:%2x:%2x",
				(unsigned int *) &wol_passwd[0],
				(unsigned int *) &wol_passwd[1],
				(unsigned int *) &wol_passwd[2],
				(unsigned int *) &wol_passwd[3],
				(unsigned int *) &wol_passwd[4],
				(unsigned int *) &wol_passwd[5] ) != wol_passwd_size )
	{
		printf( "Invalid MAC-address\n" );
		return;
	}


	/* Note: PF_INET, SOCK_DGRAM, IPPROTO_UDP would allow SIOCGIFHWADDR to
	   work as non-root, but we need SOCK_PACKET to specify the Ethernet
	   destination address. */

#if defined(PF_PACKET)
	int s = socket( PF_PACKET, SOCK_RAW, 0 );
#else
	int s = socket( AF_INET, SOCK_PACKET, SOCK_PACKET );
#endif
	if( s < 0 )
	{
		if( errno == EPERM )
		{
			fprintf( stderr, "ether-wake: This program must be run as root.\n" );
		}
		else
		{
			perror( "ether-wake: socket" );
		}
		return;
	}

#if 0
	/* We look up the station address before reporting failure so that
	   errors may be reported even when run as a normal m_user.
	*/
	struct ether_addr * eap = ether_aton( mac );
	if( eap )
	{
		eaddr = *eap;
		printf( "The target station address is %s.\n",
							ether_ntoa( eap ) );
/*	} else if (ether_hostton(m_mac.ascii(), &eaddr) == 0) {
		fprintf(stderr, "Station address for hostname %s is %s.\n",
					m_mac.ascii(), ether_ntoa(&eaddr));*/
	}
	else
	{
		printf ("Invalid MAC-address\n");
		return;
	}
#endif


	if( opt_broadcast )
	{
		memset( outpack+0, 0xff, 6 );
	}
	else
	{
		memcpy( outpack, eaddr.ether_addr_octet, 6 );
	}

	memcpy( outpack+6, eaddr.ether_addr_octet, 6 );
	outpack[12] = 0x08;				/* Or 0x0806 for ARP, 0x8035 for RARP */
	outpack[13] = 0x42;
	int pktsize = 14;

	memset( outpack+pktsize, 0xff, 6 );
	pktsize += 6;

	for( int i = 0; i < 16; i++ )
	{
		memcpy( outpack+pktsize, eaddr.ether_addr_octet, 6 );
		pktsize += 6;
	}
//	fprintf(stderr, "Packet is ");
//	for (i = 0; i < pktsize; i++)
//		fprintf(stderr, " %2.2x", outpack[i]);
//	fprintf(stderr, ".\n");

	/* Fill in the source address, if possible.
	   The code to retrieve the local station address is Linux specific. */
	if( !opt_no_src_addr )
	{
		struct ifreq if_hwaddr;
		//unsigned char * hwaddr = if_hwaddr.ifr_hwaddr.sa_data;

		strcpy( if_hwaddr.ifr_name, ifname );
		if( ioctl( s, SIOCGIFHWADDR, &if_hwaddr ) < 0 )
		{
			fprintf( stderr, "SIOCGIFHWADDR on %s failed: %s\n", ifname, strerror( errno ) );
			/* Magic packets still work if our source address is bogus, but
			   we fail just to be anal. */
			return;
		}
		memcpy( outpack+6, if_hwaddr.ifr_hwaddr.sa_data, 6 );

		//printf("The hardware address (SIOCGIFHWADDR) of %s is type %d  %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n", ifname, if_hwaddr.ifr_hwaddr.sa_family, hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	}

	memcpy( outpack+pktsize, wol_passwd, wol_passwd_size );
	pktsize += wol_passwd_size;



#if defined(PF_PACKET)
	struct ifreq ifr;
	strncpy( ifr.ifr_name, ifname, sizeof( ifr.ifr_name ) );
	if( ioctl( s, SIOCGIFINDEX, &ifr ) == -1 )
	{
		fprintf( stderr, "SIOCGIFINDEX on %s failed: %s\n", ifname,
							strerror( errno ) );
		return;
	}
	memset( &whereto, 0, sizeof( whereto ) );
	whereto.sll_family = AF_PACKET;
	whereto.sll_ifindex = ifr.ifr_ifindex;
	/* The manual page incorrectly claims the address must be filled.
	   We do so because the code may change to match the docs. */
	whereto.sll_halen = ETH_ALEN;
	memcpy( whereto.sll_addr, outpack, ETH_ALEN );

#else
	whereto.sa_family = 0;
	strcpy( whereto.sa_data, ifname );
#endif

	if( sendto( s, outpack, pktsize, 0, (struct sockaddr *) &whereto,
						sizeof( whereto ) ) < 0 )
	{
		perror( "sendto" );
	}

#ifdef USE_SEND
	if( bind( s, (struct sockaddr *) &whereto, sizeof( whereto ) ) < 0 )
	{
		perror( "bind" );
	}
	else if( send( s, outpack, 100, 0 ) < 0 )
	{
		perror( "send" );
	}
#elif USE_SENDMSG
	struct msghdr msghdr = { 0,};
	struct iovec iovector[1];
	msghdr.msg_name = &whereto;
	msghdr.msg_namelen = sizeof( whereto );
	msghdr.msg_iov = iovector;
	msghdr.msg_iovlen = 1;
	iovector[0].iov_base = outpack;
	iovector[0].iov_len = pktsize;
	if( ( i = sendmsg( s, &msghdr, 0 ) ) < 0 )
	{
		perror( "sendmsg" );
	}
	else if( debug )
	{
		printf( "sendmsg worked, %d (%d).\n", i, errno );
	}
#else

#error no method for sending raw-packet

#endif

#endif

#endif
}




void powerDown( void )
{
	QProcess::startDetached(
#ifdef BUILD_WIN32
			"shutdown -s -t 0 -f"
#else
			"halt"
#endif
					);
}




void reboot( void )
{
	QProcess::startDetached(
#ifdef BUILD_WIN32
			"shutdown -r -t 0 -f"
#else
			"reboot"
#endif
					);
}



void logoutUser( void )
{
	QProcess::startDetached(
#ifdef BUILD_WIN32
			"shutdown -l -t 0 -f"
#else
			"killall X"
#endif
					);
}



QString currentUser( void )
{
#ifdef BUILD_WIN32

	OSVERSIONINFO osversioninfo;
	osversioninfo.dwOSVersionInfoSize = sizeof( osversioninfo );

	// Get the current OS version
	if( !GetVersionEx( &osversioninfo ) )
	{
		osversioninfo.dwPlatformId = 0;
	}
	if( osversioninfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		// Windows NT, service-mode

		// verify that a user is logged on

		// Get the current Window station
		HWINSTA station = GetProcessWindowStation();
		if( station == NULL )
		{
			return FALSE;
		}

		// Get the current user SID size
		DWORD usersize;
		GetUserObjectInformation( station, UOI_USER_SID, NULL, 0,
								&usersize );

		// Check the required buffer size isn't zero
		if( usersize == 0 )
		{
			// No user is logged in - ensure we're not
			// impersonating anyone
			RevertToSelf();
			return "";
		}
	}

	// When we reach here, we're either running under Win9x, or we're
	// running under NT as an application or as a service impersonating a
	// user Either way, we should find a suitable user name.

	switch( osversioninfo.dwPlatformId )
	{
		case VER_PLATFORM_WIN32_WINDOWS:
		case VER_PLATFORM_WIN32_NT:
		{
			// Just call GetCurrentUser
			DWORD len = 256;
			char buf[len];
			if( GetUserName( buf, &len ) != 0 )
			{
				return buf;
			}
		}
	}

/*	const int MAX_USERNAME_LEN = 256;
	char user_name[MAX_USERNAME_LEN];
	if( vncService::GetCurrentUser( user_name, MAX_USERNAME_LEN ) )
	{
		return user_name;
	}*/

#else

#ifdef HAVE_PWD_H
	struct passwd * pw_entry = getpwuid( getuid() );
	if( pw_entry != NULL )
	{
		return pw_entry->pw_name;
	}
#endif

	char * user_name = getenv( "USER" );
	if( user_name != NULL )
	{
		return user_name;
	}

#endif

	return "Default";

}


static const QString userRoleNames[] =
{
	"none",
	"teacher",
	"supporter",
	"admin"
} ;

inline QString keyPath( const ISD::userRoles _role, const QString _group )
{
	QSettings settings( QSettings::SystemScope, "EasySchoolSolutions",
								"iTALC" );
	if( _role <= ISD::RoleNone || _role >= ISD::RoleCount )
	{
		return( "" );
	}
	const QString fallback_dir =
#ifdef BUILD_LINUX
		"/etc/italc/keys/"
#elif BUILD_WIN32
		"c:\\italc\\keys\\"
#endif
		+ _group + QDir::separator() + userRoleNames[_role] +
						QDir::separator() + "key";
	const QString val = settings.value( "keypaths" + _group + "/" +
					userRoleNames[_role] ).toString();
	if( val.isEmpty() )
	{
		settings.setValue( "keypaths" + _group + "/" +
					userRoleNames[_role], fallback_dir );
		return( fallback_dir );
	}
	return( val );
}


QString privateKeyPath( const ISD::userRoles _role )
{
	return( keyPath( _role, "private" ) );
}


QString publicKeyPath( const ISD::userRoles _role )
{
	return( keyPath( _role, "public" ) );
}




QString snapshotDir( void )
{
	QSettings settings;
	return( settings.value( "paths/snapshots", personalConfigDir() +
						"snapshots" ).toString() +
							QDir::separator() );
}




QString globalConfigPath( void )
{
	QSettings settings;
	return( settings.value( "paths/globalconfig", personalConfigDir() +
					"globalconfig.xml" ).toString() );
}




QString personalConfigDir( void )
{
	QSettings settings;
	const QString d = settings.value( "paths/personalconfig" ).toString();
	return( d.isEmpty() ?
				QDir::homePath() + QDir::separator() +
				".italc" + QDir::separator()
			:
				d );
}




QString personalConfigPath( void )
{
	QSettings settings;
	const QString d = settings.value( "paths/personalconfig" ).toString();
	return( d.isEmpty() ?
				personalConfigDir() + "personalconfig.xml"
			:
				d );
}


bool ensurePathExists( const QString & _path )
{
	QString p = QDir( _path ).absolutePath();
	if( !QFileInfo( _path ).isDir() )
	{
		p = QFileInfo( _path ).absolutePath();
	}
	QStringList dirs;
	while( !QDir( p ).exists() && !p.isEmpty() )
	{
		dirs.push_front( QDir( p ).dirName() );
		p.chop( dirs.front().size() + 1 );
	}
	if( !p.isEmpty() )
	{
		return( QDir( p ).mkpath( dirs.join( QDir::separator() ) ) );
	}
	return( FALSE );
}


} // end of namespace localSystem

