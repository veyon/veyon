//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2000 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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


// vncHTTPConnect.cpp

// Implementation of the HTTP server class

#include "stdhdrs.h"
#include "vsocket.h"
#include "vnchttpconnect.h"
#include "vncserver.h"
#include <omnithread.h>
#include "resource.h"

#include <ctype.h>

//	[v1.0.2-jp1 fix]
extern	HINSTANCE	hInstResDLL;
extern bool			fShutdownOrdered;

/* Old viewer - TODO: remove
// HTTP messages/message formats
const char HTTP_MSG_OK []			="HTTP/1.0 200 OK\r\n\r\n";
const char HTTP_FMT_INDEX[]			="<HTML><TITLE>VNC desktop [%.256s]</TITLE>\n"
	"<APPLET CODE=vncviewer.class ARCHIVE=vncviewer.jar WIDTH=%d HEIGHT=%d>\n"
	"<param name=PORT value=%d></APPLET></HTML>\n";
const char HTTP_MSG_NOSOCKCONN []	="<HTML><TITLE>UltraVNC desktop</TITLE>\n"
	"<BODY>The requested desktop is not configured to accept incoming connections.</BODY>\n"
	"</HTML>\n";
const char HTTP_MSG_NOSUCHFILE []	="HTTP/1.0 404 Not found\r\n\r\n"
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n"
    "<BODY><H1>The requested file could not be found</H1></BODY>\n";
*/

// HTTP messages / message formats
const char HTTP_MSG_OK[] = "HTTP/1.0 200 OK\n\n";

const char HTTP_FMT_INDEX[] =
"<HTML>\n"
"  <HEAD><TITLE> [%.256s] </TITLE></HEAD>\n"
"  <BODY>\n"
"  <SPAN style='position: absolute; top:0px;left:0px'>\n" // sf@2002 - Hopefully supported by all recent browsers... to be checked.
"<OBJECT \n"
"    ID='VncViewer'\n"
"    classid = 'clsid:8AD9C840-044E-11D1-B3E9-00805F499D93'\n"
"    codebase = 'http://java.sun.com/update/1.4.2/jinstall-1_4-windows-i586.cab#Version=1,4,0,0'\n"
"    WIDTH = %d HEIGHT = %d >\n"
"    <PARAM NAME = CODE VALUE = VncViewer.class >\n"
"    <PARAM NAME = ARCHIVE VALUE = VncViewer.jar >\n"
"    <PARAM NAME = 'type' VALUE = 'application/x-java-applet;version=1.4'>\n"
"    <PARAM NAME = 'scriptable' VALUE = 'false'>\n"
"    <PARAM NAME = PORT VALUE=%d>\n"
"    <PARAM NAME = ENCODING VALUE=Tight>\n"
"    <PARAM NAME = 'Open New Window' VALUE='Yes'>\n"
"    <COMMENT>\n"
"	<EMBED \n"
"            type = 'application/x-java-applet;version=1.4' \\\n"
"            CODE = VncViewer.class \\\n"
"            ARCHIVE = VncViewer.jar \\\n"
"            WIDTH = %d \\\n"
"            HEIGHT = %d \\\n"
"            PORT =%d \\\n"
"            ENCODING =Tight \\\n"
"	    scriptable = false \\\n"
"	    pluginspage ='http://java.sun.com/products/plugin/index.html#download'>\n"
"	    <NOEMBED>\n"            
"            </NOEMBED>\n"
"	</EMBED>\n"
"    </COMMENT>\n"
"</OBJECT>\n"
/*
"    <APPLET CODE=VncViewer.class ARCHIVE=VncViewer.jar WIDTH=%d HEIGHT=%d>\n"
"      <PARAM NAME=PORT VALUE=%d>\n"
"      <PARAM NAME=ENCODING VALUE=Tight>\n"
"    </APPLET>"*/
"  </SPAN>\n"
// "    <BR>\n"
// "    <A href=\"http://UltraVNC.sf.net\">UltraVNC Home Page</A></HTML>\n" // sf@2002: don't waste space 
"  </BODY>\n"
"</HTML>\n";

const char HTTP_MSG_NOSOCKCONN [] =
"<HTML>\n"
"  <HEAD><TITLE>UltraVNC desktop</TITLE></HEAD>\n"
"  <BODY>\n"
"    <H1>Connections Disabled</H1>\n"
"    The requested desktop is not configured to accept incoming connections.\n"
"  </BODY>\n"
"</HTML>\n";

const char HTTP_MSG_BADPARAMS [] =
"<HTML>\n"
"  <HEAD><TITLE>UltraVNC desktop</TITLE></HEAD>\n"
"  <BODY>\n"
"    <H1>Bad Parameters</H1>\n"
"    The sequence of applet parameters specified within the URL is invalid.\n"
"  </BODY>\n"
"</HTML>\n";

const char HTTP_MSG_NOSUCHFILE [] =
"HTTP/1.0 404 Not Found\n\n"
"<HTML>\n"
"  <HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
"  <BODY>\n"
"    <H1>Not Found</H1>\n"
"    The requested file could not be found.\n"
"  </BODY>\n"
"</HTML>\n";



// Filename to resource ID mappings for the Java class files:
typedef struct _FileToResourceMap {
	char *filename;
	char *type;
	int resourceID;
} FileMap;

const FileMap filemapping []	={
	{"/VncViewer.jar", "JavaArchive", IDR_VNCVIEWER_JAR},	
	};

const FileMap filemapping2 []	={
	{"/VncViewer.jar", "JavaArchive", IDR_JAVAARCHIVE1},	
	};
const int filemappingsize		= 1;

// The function for the spawned thread to run
class vncHTTPConnectThread : public omni_thread
{
public:
	// Init routine
	virtual BOOL Init(VSocket *socket, vncServer *server);
	void Inithttp(vncServer* svr) { m_server = svr; }

	// Code to be executed by the thread
	virtual void *run_undetached(void * arg);
	// Routines to handle HTTP requests
	virtual void DoHTTP(VSocket *socket);
	virtual char *ReadLine(VSocket *socket, char delimiter);


	// Fields used internally
	BOOL		m_shutdown;
protected:

	vncServer	*m_server;
	VSocket		*m_socket;
};


// Added for HTTP-via-RFB. This function is called when a connection is
// accepted on the RFB port. If the client sends an HTTP request we
// handle it here and return TRUE. Otherwise we return
// FALSE and the caller continues with the RFB handshake.
VBool maybeHandleHTTPRequest(VSocket* sock,vncServer* svr)
{
	if (!sock->ReadSelect(2000)) return false;

	// Client is sending data. Create a vncHTTPConnectThread to
	// handle it.
	vncHTTPConnectThread http;
	http.Inithttp(svr);
	http.DoHTTP(sock);
	sock->Shutdown();
	sock->Close();
	delete sock;
	return true;
}


// Method implementations
BOOL vncHTTPConnectThread::Init(VSocket *socket, vncServer *server)
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
void *vncHTTPConnectThread::run_undetached(void * arg)
{
	vnclog.Print(LL_INTINFO, VNCLOG("started HTTP server thread\n"));

	// Go into a loop, listening for connections on the given socket
	while (!m_shutdown)
	{
		// Accept an incoming connection
		VSocket *new_socket = m_socket->Accept();
		if (new_socket == NULL)
			break;
		if (m_shutdown)
		{
			delete new_socket;
			break;
		}
		vnclog.Print(LL_CLIENTS, VNCLOG("HTTP client connected\n"));
		Sleep(250);
		// Successful accept - perform the transaction
		new_socket->SetTimeout(15000); //ms
		DoHTTP(new_socket);
		Sleep(500);
		// And close the client
		new_socket->Shutdown();
		new_socket->Close();
		delete new_socket;
	}

	vnclog.Print(LL_INTINFO, VNCLOG("quitting HTTP server thread\n"));

	return NULL;
}

void vncHTTPConnectThread::DoHTTP(VSocket *socket)

{
	char filename[1024];
	char *line;

	// Read in the HTTP header
	if ((line = ReadLine(socket, '\n')) == NULL)
		return;

	// Scan the header for the filename and free the storage
	int result = sscanf(line, "GET %s ", (char*)&filename);
	delete [] line;
	if ((result == 0) || (result == EOF))
		return;

	vnclog.Print(LL_CLIENTS, VNCLOG("file %s requested\n"), filename);

	// Read in the rest of the browser's request data and discard...
	BOOL emptyline=TRUE;

	for (;;)
	{
		char c;

		if (!socket->ReadExactHTTP(&c, 1))
			return;
		if (c=='\n')
		{
			if (emptyline)
				break;
			emptyline = TRUE;
		}
		else
			if (c >= ' ')
			{
				emptyline = FALSE;
			}
	}

	vnclog.Print(LL_INTINFO, VNCLOG("parameters read\n"));

    if (filename[0] != '/')
	{
		vnclog.Print(LL_CONNERR, VNCLOG("filename didn't begin with '/'\n"));
		socket->SendExactHTTP(HTTP_MSG_NOSUCHFILE, strlen(HTTP_MSG_NOSUCHFILE));
		return;
	}

	// Switch, dependent upon the filename:
	if (strcmp(filename, "/") == 0)
	{
		char indexpage[2048 + MAX_COMPUTERNAME_LENGTH + 1];

		vnclog.Print(LL_CLIENTS, VNCLOG("sending main page\n"));

		// Send the OK notification message to the client
		if (!socket->SendExactHTTP(HTTP_MSG_OK, strlen(HTTP_MSG_OK)))
			return;

		// Compose the index page
		if (m_server->SockConnected())
		{
			int width, height, depth;

			// Get the screen's dimensions
			m_server->GetScreenInfo(width, height, depth);

			// Get the name of this desktop
			char desktopname[MAX_COMPUTERNAME_LENGTH+1];
			DWORD desktopnamelen = MAX_COMPUTERNAME_LENGTH + 1;
			if (GetComputerName(desktopname, &desktopnamelen))
			{
				// Make the name lowercase
				for (size_t x=0; x<strlen(desktopname); x++)
				{
					desktopname[x] = tolower(desktopname[x]);
				}
			}
			else
			{
				strcpy(desktopname, "WinVNC");
			}

			// Send the java applet page
			sprintf(indexpage, HTTP_FMT_INDEX,
				desktopname, width, height+32,
				m_server->GetPort(), width, height+32,
				m_server->GetPort()
				);
		}
		else
		{
			// Send a "sorry, not allowed" page
			sprintf(indexpage, HTTP_MSG_NOSOCKCONN);
		}

		// Send the page
		if (socket->SendExactHTTP(indexpage, strlen(indexpage)))
			vnclog.Print(LL_INTINFO, VNCLOG("sent page\n"));

		return;
	}

	// File requested was not the index so check the mappings
	// list for a different file.

	if (m_server->MSLogonRequired())
	{

		for (int x=0; x < filemappingsize; x++)
	{
		if (strcmp(filename, filemapping2[x].filename) == 0)
		{
			HRSRC resource;
			HGLOBAL resourcehan;
			char *resourceptr;
			int resourcesize;

			vnclog.Print(LL_INTINFO, VNCLOG("requested file recognised\n"));

			// Find the resource here
			//	[v1.0.2-jp1 fix]
			//resource = FindResource(NULL,
			resource = FindResource(hInstResDLL,
					MAKEINTRESOURCE(filemapping2[x].resourceID),
					filemapping2[x].type
					);
			if (resource == NULL)
				return;

			// Get its size
			//	[v1.0.2-jp1 fix]
			//resourcesize = SizeofResource(NULL, resource);
			resourcesize = SizeofResource(hInstResDLL, resource);

			// Load the resource
			//	[v1.0.2-jp1 fix]
			//resourcehan = LoadResource(NULL, resource);
			resourcehan = LoadResource(hInstResDLL, resource);
			if (resourcehan == NULL)
				return;

			// Lock the resource
			resourceptr = (char *)LockResource(resourcehan);
			if (resourceptr == NULL)
				return;

			vnclog.Print(LL_INTINFO, VNCLOG("sending file...\n"));

			// Send the OK message
			if (!socket->SendExactHTTP(HTTP_MSG_OK, strlen(HTTP_MSG_OK)))
				return;

			// Now send the entirety of the data to the client
			if (!socket->SendExactHTTP(resourceptr, resourcesize))
				return;

			vnclog.Print(LL_INTINFO, VNCLOG("file successfully sent\n"));

			return;
		}
	}
	}
	else
	{
	// Now search the mappings for the desired file
	for (int x=0; x < filemappingsize; x++)
	{
		if (strcmp(filename, filemapping[x].filename) == 0)
		{
			HRSRC resource;
			HGLOBAL resourcehan;
			char *resourceptr;
			int resourcesize;

			vnclog.Print(LL_INTINFO, VNCLOG("requested file recognised\n"));

			// Find the resource here
			//	[v1.0.2-jp1 fix]
			//resource = FindResource(NULL,
			resource = FindResource(hInstResDLL,
					MAKEINTRESOURCE(filemapping[x].resourceID),
					filemapping[x].type
					);
			if (resource == NULL)
				return;

			// Get its size
			//	[v1.0.2-jp1 fix]
			//resourcesize = SizeofResource(NULL, resource);
			resourcesize = SizeofResource(hInstResDLL, resource);

			// Load the resource
			//	[v1.0.2-jp1 fix]
			//resourcehan = LoadResource(NULL, resource);
			resourcehan = LoadResource(hInstResDLL, resource);
			if (resourcehan == NULL)
				return;

			// Lock the resource
			resourceptr = (char *)LockResource(resourcehan);
			if (resourceptr == NULL)
				return;

			vnclog.Print(LL_INTINFO, VNCLOG("sending file...\n"));

			// Send the OK message
			if (!socket->SendExactHTTP(HTTP_MSG_OK, strlen(HTTP_MSG_OK)))
				return;

			// Now send the entirety of the data to the client
			if (!socket->SendExactHTTP(resourceptr, resourcesize))
				return;

			vnclog.Print(LL_INTINFO, VNCLOG("file successfully sent\n"));

			return;
		}
	}
	}

	// Send the NoSuchFile notification message to the client
	if (!socket->SendExactHTTP(HTTP_MSG_NOSUCHFILE, strlen(HTTP_MSG_NOSUCHFILE)))
		return;
}

char *vncHTTPConnectThread::ReadLine(VSocket *socket, char delimiter)

{
	int max=1024;
	// Allocate the maximum required buffer
	char *buffer = new char[max+1];
	int buffpos = 0;

	// Read in data until a delimiter is read
	for (;;)
	{
		char c;

		if (!socket->ReadExactHTTP(&c, 1))
		{
			delete [] buffer;
			return NULL;
		}

		if (c == delimiter)
		{
			buffer[buffpos] = 0;
			return buffer;
		}

		buffer[buffpos] = c;
		buffpos++;

		if (buffpos == (max-1))
		{
			buffer[buffpos] = 0;
			return buffer;
		}
	}
}

// The vncSockConnect class implementation

vncHTTPConnect::vncHTTPConnect()
{
	m_thread = NULL;
}

vncHTTPConnect::~vncHTTPConnect()
{
   m_socket.Shutdown();

    // Join with our lovely thread
    if (m_thread != NULL)
    {
		// *** This is a hack to force the listen thread out of the accept call,
		// because Winsock accept semantics are broken.
		((vncHTTPConnectThread *)m_thread)->m_shutdown = TRUE;

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

BOOL vncHTTPConnect::Init(vncServer *server, UINT port)
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
	m_thread = new vncHTTPConnectThread;
	if (m_thread == NULL)
		return FALSE;

	// And start it running
	return ((vncHTTPConnectThread *)m_thread)->Init(&m_socket, server);
}

