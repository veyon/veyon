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
#if (!defined(_WINVNC_INIFILE))
#define _WINVNC_INIFILE

bool Copy_to_Temp(char *tempfile);
bool Copy_to_Secure_from_temp(char *tempfile);
#define INIFILE_NAME "ultravnc.ini"

class IniFile
{

// Fields
public:
	char myInifile[MAX_PATH];

// Methods
public:
	// Make the desktop thread & window proc friends

	IniFile();
	~IniFile();
	bool WriteString(char *key1, char *key2,char *value);
	bool WritePassword(char *value);
	bool WritePassword2(char *value); //PGM
	bool WriteInt(char *key1, char *key2,int value);
	int ReadInt(char *key1, char *key2,int Defaultvalue);
	void ReadString(char *key1, char *key2,char *value,int valuesize);
	void ReadPassword(char *value,int valuesize);
	void ReadPassword2(char *value,int valuesize); //PGM
	void IniFileSetSecure();
	//void IniFileSetTemp();
	void IniFileSetTemp(char *lpCmdLine);
	void copy_to_secure();

    bool IsWritable();

protected:
		
};
#endif
