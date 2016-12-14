//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncSockConnect.cpp

// Implementation of the listening socket class

#include "stdhdrs.h"
#include "vsocket.h"
#include "vncsockconnect.h"
#include "vncserver.h"
#include <omnithread.h>


VBool maybeHandleHTTPRequest(VSocket* sock,vncServer* svr);

// The function for the spawned thread to run
class vncSockConnectThread : public omni_thread
{
public:
	// Init routine
	virtual BOOL Init(VSocket *socket, vncServer *server);

	// Code to be executed by the thread
	virtual void *run_undetached(void * arg);

	// Fields used internally
	BOOL		m_shutdown;
protected:
	VSocket		*m_socket;
	vncServer	*m_server;
};

// Method implementations
BOOL vncSockConnectThread::Init(VSocket *socket, vncServer *server)
{
	// Save the server pointer
	m_server = server;

	// Save the socket pointer
	m_socket = socket;

	// Start the thread
	m_shutdown = FALSE;
	start_undetached();

	return TRUE;
}

// Code to be executed by the thread
void *vncSockConnectThread::run_undetached(void * arg)
{
	vnclog.Print(LL_STATE, VNCLOG("started socket connection thread\n"));
	// Go into a loop, listening for connections on the given socket
	while (!m_shutdown)
	{
		// Accept an incoming connection
		VSocket *new_socket = m_socket->Accept();
		if (new_socket == NULL)
			break;
		else
		{
			if( m_server->GetHttpPort()== m_server->GetPort())
			{
				if (maybeHandleHTTPRequest(new_socket,m_server)) {
 					// HTTP request has been handled and new_socket closed. The client will
 					// now reconnect to the RFB port and we will carry on.
 					vnclog.Print(LL_CLIENTS,VNCLOG("Woo hoo! Served Java applet via RFB!"));
 					continue;
 				}
			}

			vnclog.Print(LL_CLIENTS, VNCLOG("accepted connection from %s\n"), new_socket->GetPeerName());
			if (!m_shutdown && !fShutdownOrdered) m_server->AddClient(new_socket, FALSE, FALSE,NULL,false);
		}

		if (m_shutdown) 
		{
			delete new_socket;
			break;
		}
		
	}
	//if (new_socket != null)
	//	delete new_socket;

	vnclog.Print(LL_STATE, VNCLOG("quitting socket connection thread\n"));

	return NULL;
}

// The vncSockConnect class implementation

vncSockConnect::vncSockConnect()
{
	m_thread = NULL;
}

vncSockConnect::~vncSockConnect()
{
    m_socket.Shutdown();
	m_socket.Close();

    // Join with our lovely thread
    if (m_thread != NULL)
    {
	// *** This is a hack to force the listen thread out of the accept call,
	// because Winsock accept semantics are broken
	((vncSockConnectThread *)m_thread)->m_shutdown = TRUE;

	VSocket socket;
#ifdef IPV6V4
	socket.CreateBindConnect("localhost", m_port);
#else
	socket.Create();
	socket.Bind(0);
	socket.Connect("localhost", m_port);
#endif
	socket.Close();

	void *returnval;
	m_thread->join(&returnval);
	m_thread = NULL;

	m_socket.Close();
    }
}

BOOL vncSockConnect::Init(vncServer *server, UINT port)
{
	// Save the port id
	m_port = port;

#ifdef IPV6V4
	if (!m_socket.CreateBindListen(m_port, server->LoopbackOnly()))
		return FALSE;
#else
	// Create the listening socket
	if (!m_socket.Create())
		return FALSE;

	// Bind it
	if (!m_socket.Bind(m_port, server->LoopbackOnly()))
		return FALSE;

	// Set it to listen
	if (!m_socket.Listen())
		return FALSE;
#endif
	// Create the new thread
	m_thread = new vncSockConnectThread;
	if (m_thread == NULL)
		return FALSE;

	// And start it running
	return ((vncSockConnectThread *)m_thread)->Init(&m_socket, server);
}

