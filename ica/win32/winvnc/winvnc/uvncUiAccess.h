/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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

#ifndef _G_UIACCESS_H_
#define _G_UIACCESS_H_

#include "stdhdrs.h"
#include <sddl.h>
#pragma warning (disable:6387)
#include <shellapi.h>

struct keyEventdata
{
	BYTE bVk;
	BYTE bScan;
	DWORD dwflags;
};

class comm_serv
{
public:
    comm_serv();
	virtual ~comm_serv();
	bool Init(char *name,int datasize_IN,int datasize_OUT,bool app,bool master);
	HANDLE GetEvent();
	char *Getsharedmem();
	void ReadData(char *databuffer);
	void SetData(char *databuffer);
	void Call_Fnction(char *databuffer_IN,char *databuffer_OUT);
	void Call_Fnction_no_feedback();
	void Call_Fnction_no_feedback_data(char *databuffer_IN,char *databuffer_OUT);
	void Call_Fnction_Long(char *databuffer_IN,char *databuffer_OUT);
	void Call_Fnction_Long_Timeout(char *databuffer_IN,char *databuffer_OUT,int timeout);
	HANDLE InitFileHandle(char *name,int IN_datasize_IN,int IN_datasize_OUT,bool app,bool master);
	void Force_unblock();
	void Release();

private:
	HANDLE event_E_IN;
	HANDLE event_E_IN_DONE;
	HANDLE event_E_OUT;
	HANDLE event_E_OUT_DONE;
	HANDLE hMapFile_IN;
	HANDLE hMapFile_OUT;
	char *data_IN;
	char *data_OUT;
	CRITICAL_SECTION CriticalSection_IN;
	CRITICAL_SECTION CriticalSection_OUT;
	SECURITY_ATTRIBUTES secAttr;
	void create_sec_attribute();
	char filemapping_IN[64];
	char filemapping_OUT[64];
	char event_IN[64];
	char event_IN_DONE[64];
	char event_OUT[64];
	char event_OUT_DONE[64];
	int datasize_IN;
	int datasize_OUT;
	int timeout_counter;
	bool GLOBAL_RUNNING;
};

void keybd_uni_event(_In_  BYTE bVk,_In_  BYTE bScan,_In_  DWORD dwFlags,_In_  ULONG_PTR dwExtraInfo);
void keybd_initialize();
void keybd_initialize_no_crit();
void keybd_delete();
void keepalive();
#endif