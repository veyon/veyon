//  Copyright (C) 2006 Constantin Kaplinsky. All Rights Reserved.
//
//  TightVNC is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// TightVNC distribution homepage on the Web: http://www.tightvnc.com/

#include <stdlib.h>
#include <string.h>
#include "ParseHost.h"

//
// Parse a VNC host name string, return port number.
// See the details in ParseHost.h.
//

int ParseHostPort(char *str, int base_port)
{
	int port, disp;
	char *port_ptr;

	port = base_port;
	port_ptr = strchr(str, ':');
	if (port_ptr) {
		*port_ptr++ = '\0';
		if (*port_ptr == ':') {
			port = atoi(++port_ptr);	// port number after "::"
		} else {
			disp = atoi(port_ptr);
			if (disp < 100) {
				port += disp;			// display number after ":"
			} else {
				port = disp;			// port number after ":"
			}
		}
	}

	return port;
}
