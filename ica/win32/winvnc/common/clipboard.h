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
// http://ultravnc.sourceforge.net/
//
////////////////////////////////////////////////////////////////////////////


// Clipboard.h

// adzm - July 2010
//
// Common classes for dealing with the clipboard, including serializing and deserializing compressed data, hashing and comparing, etc.
// Used by server and viewer.

#pragma once

#define VC_EXTRALEAN
#include <windows.h>
#include <string>
#include <rdr/MemOutStream.h>
#include "rfb.h"

struct ExtendedClipboardDataMessage {
	ExtendedClipboardDataMessage();
	~ExtendedClipboardDataMessage();

	void Reset();
	void AddFlag(CARD32 flag);
	bool HasFlag(CARD32 flag);

	int GetMessageLength();	// does not include rfbExtendedClipboardData
	int GetDataLength();	// does include rfbExtendedClipboardData
	const BYTE* GetData();

	BYTE* GetBuffer();		// writable buffer
	int GetBufferLength();	// does include rfbExtendedClipboardData

	const BYTE* GetCurrentPos();

	rfbExtendedClipboardData* m_pExtendedData;

	void AppendInt(CARD32 val); // swaps if LE
	void AppendBytes(BYTE* pData, int length);
	void Advance(int len);
	CARD32 ReadInt();

	void EnsureBufferLength(int len, bool bGrowBeyond = true);

	int CountFormats();
	CARD32 GetFlags();

protected:

	int m_nInternalLength;
	BYTE* m_pCurrentPos;
	BYTE* m_pData;
};

struct ClipboardSettings {
	ClipboardSettings(CARD32 caps);

	static CARD32 defaultCaps;
	static CARD32 defaultViewerCaps;
	static CARD32 defaultServerCaps;

	static const UINT formatHTML;
	static const UINT formatRTF;
	static const UINT formatUnicodeText;

	static const int defaultLimitText;
	static const int defaultLimitRTF;
	static const int defaultLimitHTML;

	static const int defaultLimit;

	///////

	bool m_bSupportsEx;

	int m_nLimitText;
	int m_nLimitRTF;
	int m_nLimitHTML;

	int m_nRequestedLimitText;
	int m_nRequestedLimitRTF;
	int m_nRequestedLimitHTML;

	CARD32 m_myCaps;

	CARD32 m_remoteCaps; // messages and formats that will be handled by the remote application

	void PrepareCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage);

	void HandleCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage, bool bSetLimits = true);
};

struct ClipboardHolder {
	ClipboardHolder(HWND hwndOwner);

	~ClipboardHolder();

	bool m_bIsOpen;
};

struct ClipboardData {
	ClipboardData();

	~ClipboardData();

	DWORD m_crc;

	int m_lengthText;
	int m_lengthRTF;
	int m_lengthHTML;

	BYTE* m_pDataText;
	BYTE* m_pDataRTF;
	BYTE* m_pDataHTML;

	void FreeData();

	bool Load(HWND hwndOwner); // will return false on failure

	bool Restore(HWND hwndOwner, ExtendedClipboardDataMessage& extendedClipboardDataMessage);
};

struct Clipboard {
	Clipboard(CARD32 caps);

	bool UpdateClipTextEx(ClipboardData& clipboardData, CARD32 overrideFlags = 0); // returns true if something changed

	ClipboardSettings settings;
	DWORD m_crc;
	std::string m_strLastCutText; // for non-extended clipboards

	bool m_bNeedToProvide;
	bool m_bNeedToNotify;

	CARD32 m_notifiedRemoteFormats;

	ExtendedClipboardDataMessage extendedClipboardDataMessage;
	ExtendedClipboardDataMessage extendedClipboardDataNotifyMessage;
};

