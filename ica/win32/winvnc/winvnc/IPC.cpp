/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <tchar.h>
#include <crtdbg.h>

#include "IPC.h"

const static LPCTSTR g_szIPCSharedMMF = _T("{34F673E0-878F-11D5-B98A-00B0D07B8C7C}");
const static LPCTSTR g_szIPCMutex = _T("{34F673E1-878F-11D5-B98A-00B0D07B8C7C}");
int oldcounter=0;



//***********************************************
CIPC::CIPC() : m_hFileMap(NULL), m_hMutex(NULL)
{
	plist=NULL;
	m_FileView=0;
	CreateIPCMMF();
	OpenIPCMMF();
}

//***********************************************
CIPC::~CIPC()
{
	CloseIPCMMF();
	Unlock();
}

//***********************************************
bool CIPC::CreateIPCMMF(void)
{
	bool bCreated = false;

	try
	{
		if(m_hFileMap != NULL)
			return false;	// Already created

		// Create an in-memory 4KB memory mapped file to share data
		m_hFileMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
			NULL,
			PAGE_READWRITE,
			0,
			sizeof(list),
			g_szIPCSharedMMF);
		if(m_hFileMap != NULL)
			bCreated = true;
	}
	catch(...) {}

	return bCreated;
}

//***********************************************
bool CIPC::OpenIPCMMF(void)
{
	bool bOpened = false;

	try
	{
		if(m_hFileMap == NULL)
		{

		m_hFileMap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
			FALSE,
			g_szIPCSharedMMF);
		}

		if(m_hFileMap != NULL)
		{
			bOpened = true;
			m_FileView = MapViewOfFile(m_hFileMap,
			FILE_MAP_READ | FILE_MAP_WRITE,
			0, 0, 0);
			if (m_FileView==0) bOpened = false;
			plist=(mystruct*) m_FileView;

		}
	}
	catch(...) {}

	return bOpened;
}

//***********************************************
void CIPC::CloseIPCMMF(void)
{
	try
	{
		if (m_FileView) UnmapViewOfFile(m_FileView);
		if(m_hFileMap != NULL)
			CloseHandle(m_hFileMap), m_hFileMap = NULL;
	}
	catch(...) {}
}

//***********************************************
void CIPC::Addrect(int type, int x1, int y1, int x2, int y2,int x11,int y11, int x22,int y22)
{
	if (plist==NULL) return;
	if (plist->locked==1) 
	{
		return;
	}
	int counter=plist->counter;
	plist->locked=1;
	counter++;
	if (counter>1999) counter=1;
	plist->rect1[counter].left=x1;
	plist->rect1[counter].right=x2;
	plist->rect1[counter].top=y1;
	plist->rect1[counter].bottom=y2;
	plist->rect2[counter].left=x11;
	plist->rect2[counter].right=x22;
	plist->rect2[counter].top=y11;
	plist->rect2[counter].bottom=y22;
	plist->type[counter]=type;
	plist->locked=0;
	plist->counter=counter;
}

void CIPC::Addcursor(ULONG cursor)
{
	if (plist==NULL) return;
	if (plist->locked==1) 
	{
		return;
	}
	int counter=plist->counter;
	plist->locked=1;
	counter++;
	if (counter>1999) counter=1;
	plist->type[counter]=1;
	plist->locked=0;
	plist->counter=counter;
}

mystruct* CIPC::listall()
{
	return plist;
}
//***********************************************
bool CIPC::Lock(void)
{
	bool bLocked = false;

	try
	{
		// First get the handle to the mutex
		m_hMutex = CreateMutex(NULL, FALSE, g_szIPCMutex);
		if(m_hMutex != NULL)
		{
			// Wait to get the lock on the mutex
			if(WaitForSingleObject(m_hMutex, INFINITE) == WAIT_OBJECT_0)
				bLocked = true;
		}
	}
	catch(...) {}

	return bLocked;
}

//***********************************************
void CIPC::Unlock(void)
{
	try
	{
		if(m_hMutex != NULL)
		{
			ReleaseMutex(m_hMutex);
			CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}
	}
	catch(...) {}
}
