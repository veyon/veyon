/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
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
// http://www.uvnc.com

#include <locale.h>
#include <wchar.h>
#include <tchar.h>
#include "MSLogonACL.h"
#include "vncImportACL.h"
#include "vncExportACL.h"


int _tmain(int argc, TCHAR *argv[])
{
	setlocale( LC_ALL, "" );
	bool append = false;
	int rc = 1;

	if (argc > 1) {
		if (_tcsicmp(argv[1], _T("/i")) == 0 || _tcsicmp(argv[1], _T("-i")) == 0) {
			if (argc < 4) {
				return 1;
			} else {
				if (_tcsicmp(argv[2], _T("/a")) == 0 || _tcsicmp(argv[2], _T("-a")) == 0)
					append = true;
				else if (_tcsicmp(argv[2], _T("/o")) == 0 || _tcsicmp(argv[2], _T("-o")) == 0)
					; //override
				else {
					usage_(argv[0]);
					return 1;
				}
				if (!_tfreopen(argv[3], _T("r"), stdin)) {
					_tprintf(_T("Error opening file %s"), argv[3]);
					usage_(argv[0]);
					return 1;
				}
			}
			rc = import_(append);
		} else if (_tcsicmp(argv[1], _T("/e")) == 0 || _tcsicmp(argv[1], _T("-e")) == 0) {
			if (argc > 2)
				if (!_tfreopen(argv[2], _T("w"), stdout)) {
					_tprintf(_T("Error opening file %s"), argv[2]);
					usage_(argv[0]);
					return 1;
				}
			rc = export_();
		} else {
			usage_(argv[0]);
		}
	} else {
		usage_(argv[0]);
	}
	return rc;
}

int 
import_(bool append){
	int rc = 0;
	vncImportACL importAcl;
	PACL pACL = NULL; //?


		if (append)
			importAcl.GetOldACL();
		if (importAcl.ScanInput())
			rc |= 2;
		pACL = importAcl.BuildACL();
		importAcl.SetACL(pACL);

		HeapFree(GetProcessHeap(), 0, pACL);

	return rc;
}

int
export_()
{
	PACL pACL = NULL;

	vncExportACL exportAcl;
	exportAcl.GetACL(&pACL);
	exportAcl.PrintAcl(pACL);
	if (pACL)
		LocalFree(pACL);

	return 0;
}


void usage_(const TCHAR *appname){
	_tprintf(_T("Usage:\n%s /e <file>\n\t for exporting an ACL to an (optional) file.\n"), appname);
	_tprintf(_T("%s /i <mode> <file>\n\t for importing an ACL where mode is either\n"), appname);
	_tprintf(_T("\t/o for override or /a for append and file holds the ACEs.\n"));
	_tprintf(_T("For the format of the ACEs first configure some groups/users\n"));
	_tprintf(_T("with the graphical VNC Properties and then export the ACL.\n"));
	_tprintf(_T("The computer name can be replaced by a \".\" (a dot),\n"));
	_tprintf(_T("the computer's domain name by \"..\" (two dots).\n"));
}

