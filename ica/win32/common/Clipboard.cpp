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


// Clipboard.h

// adzm - July 2010
//
// Common classes for dealing with the clipboard, including serializing and deserializing compressed data, hashing and comparing, etc.
// Used by server and viewer.

#include "Clipboard.h"


#define VC_EXTRALEAN
#include <winsock2.h>
#include <windows.h>

#include <string>

#include <rdr/MemInStream.h>
#include <rdr/ZlibOutStream.h>
#include <rdr/ZlibInStream.h>
#ifdef IPP
#include "..\..\ipp_zlib\src\zlib\zlib.h"
#else
#include <zlib-1.2.5/zlib.h>
#endif


ExtendedClipboardDataMessage::ExtendedClipboardDataMessage()
	: m_pExtendedData(NULL), m_pData(NULL), m_pCurrentPos(NULL), m_nInternalLength(0)
{
	EnsureBufferLength(sz_rfbExtendedClipboardData);
	m_pCurrentPos += sz_rfbExtendedClipboardData;
}

ExtendedClipboardDataMessage::~ExtendedClipboardDataMessage()
{
	delete[] m_pData;
}

void ExtendedClipboardDataMessage::Reset()
{
	delete[] m_pData;
	m_pData = NULL;
	m_pCurrentPos = NULL;
	m_pExtendedData = NULL;
	m_pCurrentPos = NULL;
	m_nInternalLength = 0;

	EnsureBufferLength(sz_rfbExtendedClipboardData);
	m_pCurrentPos += sz_rfbExtendedClipboardData;
}

void ExtendedClipboardDataMessage::AddFlag(CARD32 flag)
{
	m_pExtendedData->flags |= Swap32IfLE(flag);
}

bool ExtendedClipboardDataMessage::HasFlag(CARD32 flag)
{
	return (m_pExtendedData->flags & Swap32IfLE(flag)) ? true : false;
}

int ExtendedClipboardDataMessage::GetMessageLength()
{
	return GetDataLength() - sz_rfbExtendedClipboardData;
}

int ExtendedClipboardDataMessage::GetDataLength()
{
	return m_pCurrentPos - m_pData;
}

const BYTE* ExtendedClipboardDataMessage::GetData()
{
	return m_pData;
}

BYTE* ExtendedClipboardDataMessage::GetBuffer()
{
	return m_pData;
}

int ExtendedClipboardDataMessage::GetBufferLength()
{
	return m_nInternalLength;
}

const BYTE* ExtendedClipboardDataMessage::GetCurrentPos()
{
	return m_pCurrentPos;
}

void ExtendedClipboardDataMessage::AppendInt(CARD32 val)
{
	EnsureBufferLength(GetDataLength() + sizeof(val));

	val = Swap32IfLE(val);

	memcpy(m_pCurrentPos, &val, sizeof(val));
	m_pCurrentPos += sizeof(val);
}

void ExtendedClipboardDataMessage::AppendBytes(BYTE* pData, int length)
{
	EnsureBufferLength(GetDataLength() + length, false);

	memcpy(m_pCurrentPos, pData, length);
	m_pCurrentPos += length;
}

void ExtendedClipboardDataMessage::Advance(int len)
{
	EnsureBufferLength(GetDataLength() + len, false);
	m_pCurrentPos += len;
}

CARD32 ExtendedClipboardDataMessage::ReadInt()
{
	CARD32 val = 0;

	memcpy(&val, m_pCurrentPos, sizeof(val));

	m_pCurrentPos += sizeof(val);

	return Swap32IfLE(val);
}

void ExtendedClipboardDataMessage::EnsureBufferLength(int len, bool bGrowBeyond)
{
	if (m_nInternalLength < len) {
		int nCurrentOffset = GetDataLength();

		int nNewLength = bGrowBeyond ? len * 2 : len;
		BYTE* pNewBuffer = new BYTE[nNewLength];
		::ZeroMemory(pNewBuffer, nNewLength);

		if (m_pData) {
			memcpy(pNewBuffer, m_pData, GetDataLength());
			delete[] m_pData;
		}

		m_pData = pNewBuffer;
		m_pExtendedData = (rfbExtendedClipboardData*)m_pData;
		m_pCurrentPos = m_pData + nCurrentOffset;
		m_nInternalLength = nNewLength;
	}
}

int ExtendedClipboardDataMessage::CountFormats()
{
	CARD32 flags = GetFlags();

	flags = flags & 0x0000FFFF;

	// from Brian W. Kernighan
	int c = 0;
	for (c = 0; flags; c++) {
		flags &= flags - 1;
	}

	return c;
}

CARD32 ExtendedClipboardDataMessage::GetFlags()
{
	return Swap32IfLE(m_pExtendedData->flags);
}

const UINT ClipboardSettings::formatUnicodeText =	CF_UNICODETEXT;
const UINT ClipboardSettings::formatRTF =			RegisterClipboardFormat("Rich Text Format");
const UINT ClipboardSettings::formatHTML =			RegisterClipboardFormat("HTML Format");
const UINT ClipboardSettings::formatDIB =			CF_DIBV5;

const int ClipboardSettings::defaultLimitText =		((int)0x00A00000); // 10 megabytes uncompressed. Pretty huge, but not a problem for a LAN. Better than the previous no limit, though.
const int ClipboardSettings::defaultLimitRTF =		((int)0x00200000); // Limit these to 2 megabytes so they don't end up adding too much strain by being enabled by default
const int ClipboardSettings::defaultLimitHTML =		((int)0x00200000); // besides, every 2mb rtf/html data I have seen will DEFLATE very well.
const int ClipboardSettings::defaultLimitDIB =		((int)0x00000000); // no DIB by default

const int ClipboardSettings::defaultLimit =			((int)0x00200000);

ClipboardSettings::ClipboardSettings(CARD32 caps)
	: m_bSupportsEx(false)
	, m_nLimitText(defaultLimitText)
	, m_nLimitRTF(defaultLimitRTF)
	, m_nLimitHTML(defaultLimitHTML)
	, m_nLimitDIB(defaultLimitDIB)
	, m_nRequestedLimitText(m_nLimitText)
	, m_nRequestedLimitRTF(m_nLimitRTF)
	, m_nRequestedLimitHTML(m_nLimitHTML)
	, m_nRequestedLimitDIB(m_nLimitDIB)
	, m_remoteCaps(ClipboardSettings::defaultCaps)
	, m_myCaps(caps)
{
}

CARD32 ClipboardSettings::defaultCaps = 
	(clipCaps | clipRequest | clipProvide)  // capabilities
	| 
	(clipText | clipRTF | clipHTML | clipDIB); // supports Unicode text, RTF, and HTML, and DIB
CARD32 ClipboardSettings::defaultViewerCaps = defaultCaps | clipNotify;
CARD32 ClipboardSettings::defaultServerCaps = defaultCaps | clipPeek;


void ClipboardSettings::PrepareCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage)
{
	// messages and formats that we can handle
	extendedDataMessage.m_pExtendedData->flags = Swap32IfLE(m_myCaps);

	// now include our limits in order of enum value
	extendedDataMessage.AppendInt(m_nLimitText);
	extendedDataMessage.AppendInt(m_nLimitRTF);
	extendedDataMessage.AppendInt(m_nLimitHTML);
	extendedDataMessage.AppendInt(m_nLimitDIB);
}

void ClipboardSettings::HandleCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage, bool bSetLimits)
{
	int nCount = extendedDataMessage.CountFormats();

	m_remoteCaps = extendedDataMessage.GetFlags();	

	if (m_remoteCaps & clipText) {
		m_nRequestedLimitText = (int)extendedDataMessage.ReadInt();		
		nCount--;
	} else {
		m_nRequestedLimitText = 0;
	}
	if (m_remoteCaps & clipRTF) {
		m_nRequestedLimitRTF = (int)extendedDataMessage.ReadInt();
		nCount--;
	} else {
		m_nRequestedLimitRTF = 0;
	}
	if (m_remoteCaps & clipHTML) {
		m_nRequestedLimitHTML = (int)extendedDataMessage.ReadInt();
		nCount--;
	} else {
		m_nRequestedLimitHTML = 0;
	}
	if (m_remoteCaps & clipDIB) {
		m_nRequestedLimitDIB = (int)extendedDataMessage.ReadInt();
		nCount--;
	} else {
		m_nRequestedLimitDIB = 0;
	}

	if (bSetLimits) {
		m_nLimitText = m_nRequestedLimitText;
		m_nLimitRTF = m_nRequestedLimitRTF;
		m_nLimitHTML = m_nRequestedLimitHTML;
		m_nLimitDIB = m_nRequestedLimitDIB;
	}

	// read any unsupported values
	while (nCount) {
		extendedDataMessage.ReadInt();
		nCount--;
	}
}

ClipboardHolder::ClipboardHolder(HWND hwndOwner)
{
	m_bIsOpen = ::OpenClipboard(hwndOwner) ? true : false;
}

ClipboardHolder::~ClipboardHolder()
{
	if (m_bIsOpen) {
		::CloseClipboard();
	}
}

ClipboardData::ClipboardData()
	: m_lengthText(0)
	, m_lengthRTF(0)
	, m_lengthHTML(0)
	, m_lengthDIB(0)
	, m_pDataText(NULL)
	, m_pDataRTF(NULL)
	, m_pDataHTML(NULL)
	, m_pDataDIB(NULL)
	, m_crc(0)
{
}

ClipboardData::~ClipboardData()
{
	FreeData();
}

void ClipboardData::FreeData()
{
	delete[] m_pDataText;
	delete[] m_pDataRTF;
	delete[] m_pDataHTML;
	delete[] m_pDataDIB;

	m_pDataText = NULL;
	m_pDataRTF = NULL;
	m_pDataHTML = NULL;
	m_pDataDIB = NULL;
}

bool ClipboardData::Load(HWND hwndOwner) // will return false on failure
{
	// This can be improved by not making copies of the clipboard data and simply keeping the handles open; however
	// I seemed to notice some issues with having multiple clipboard format data handles open when an application 
	// uses clipboard formats that it must render on demand. Regardless, I am not too concerned about this now.

	ClipboardHolder holder(hwndOwner);

	if (!holder.m_bIsOpen) {
		return false;
	}

	m_crc = 0;
	FreeData();
	
	m_crc = crc32(0L, Z_NULL, 0);

	// text
	{
		m_lengthText = 0;

		HANDLE hText = NULL;
		if (IsClipboardFormatAvailable(ClipboardSettings::formatUnicodeText)) {
			hText = ::GetClipboardData(ClipboardSettings::formatUnicodeText);
		}

		if (hText) {
			BYTE* pData = (BYTE*)GlobalLock(hText);
			int nLength = (int)GlobalSize(hText);

			if (pData != NULL && nLength > 0) {

				// Convert from UTF-16 to UTF-8
				int nConvertedSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pData, -1, NULL, 0, NULL, NULL);

				if (nConvertedSize > 0) {				
					m_pDataText = new BYTE[nConvertedSize];
					//memcpy(m_pDataText, pData, nLength);

					int nFinalConvertedSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pData, -1, (LPSTR)m_pDataText, nConvertedSize, NULL, NULL);

					if (nFinalConvertedSize > 0) {
						m_lengthText = nFinalConvertedSize;

						m_crc = crc32(m_crc, m_pDataText, nFinalConvertedSize);
					} else {
						delete[] m_pDataText;
					}
				}

				GlobalUnlock(hText);
			}
		}
	}

	// RTF
	{
		m_lengthRTF = 0;

		HANDLE hRTF = NULL;
		if (IsClipboardFormatAvailable(ClipboardSettings::formatRTF)) {
			hRTF = ::GetClipboardData(ClipboardSettings::formatRTF);
		}

		if (hRTF) {
			BYTE* pData = (BYTE*)GlobalLock(hRTF);
			int nLength = (int)GlobalSize(hRTF);

			if (pData != NULL && nLength > 0) {
				
				m_pDataRTF = new BYTE[nLength];
				memcpy(m_pDataRTF, pData, nLength);

				m_lengthRTF = nLength;

				GlobalUnlock(hRTF);
			}
		}
	}

	// HTML
	{
		m_lengthHTML = 0;

		HANDLE hHTML = NULL;
		if (IsClipboardFormatAvailable(ClipboardSettings::formatHTML)) {
			hHTML = ::GetClipboardData(ClipboardSettings::formatHTML);
		}

		if (hHTML) {
			BYTE* pData = (BYTE*)GlobalLock(hHTML);
			int nLength = (int)GlobalSize(hHTML);

			if (pData != NULL && nLength > 0) {
				
				m_pDataHTML = new BYTE[nLength];
				memcpy(m_pDataHTML, pData, nLength);

				m_lengthHTML = nLength;

				GlobalUnlock(hHTML);
			}
		}
	}

	// DIB
	{
		m_lengthDIB = 0;

		HANDLE hDIB = NULL;
		if (IsClipboardFormatAvailable(ClipboardSettings::formatDIB)) {
			hDIB = ::GetClipboardData(ClipboardSettings::formatDIB);
		}

		if (hDIB) {
			BYTE* pData = (BYTE*)GlobalLock(hDIB);
			int nLength = (int)GlobalSize(hDIB);

			if (pData != NULL && nLength > 0) {
				
				m_pDataDIB = new BYTE[nLength];
				memcpy(m_pDataDIB, pData, nLength);

				m_lengthDIB = nLength;

				GlobalUnlock(hDIB);
			}
		}
	}

	return m_lengthText + m_lengthRTF + m_lengthHTML + m_lengthDIB > 0;
}

bool ClipboardData::Restore(HWND hwndOwner, ExtendedClipboardDataMessage& extendedClipboardDataMessage)
{
	ClipboardHolder holder(hwndOwner);

	if (!holder.m_bIsOpen) {
		return false;
	}

	if (!::EmptyClipboard()) {
		return false;
	}

	m_lengthText = m_lengthRTF = m_lengthHTML = m_lengthDIB = 0;

	int nCompressedDataLength = extendedClipboardDataMessage.GetBufferLength() - extendedClipboardDataMessage.GetDataLength();

	m_crc = crc32(0L, Z_NULL, 0);
	if (nCompressedDataLength == 0) {
		// no data beyond the flags
		return true;
	}

	rdr::MemInStream inStream(extendedClipboardDataMessage.GetCurrentPos(), nCompressedDataLength);
	rdr::ZlibInStream compressedStream;

	compressedStream.setUnderlying(&inStream, nCompressedDataLength);

	int length = 0;
	bool bFailed = false;
	if (extendedClipboardDataMessage.HasFlag(clipText)) {
		length = (int)compressedStream.readU32();

		if (length > 0) {
			// get the incoming UTF-8 text
			BYTE* pIncomingText = new BYTE[length];
			compressedStream.readBytes(pIncomingText, length);

			m_crc = crc32(m_crc, pIncomingText, length);

			// now we have to translate to UTF-16
			int nConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pIncomingText, length, NULL, 0);

			if (nConvertedSize > 0) {
				HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, nConvertedSize * sizeof(wchar_t));

				if (hData) {
					BYTE* pData = (BYTE*)GlobalLock(hData);

					if (pData) {
						int nFinalConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pIncomingText, length, (LPWSTR)pData, nConvertedSize);

						GlobalUnlock(hData);

						if (nFinalConvertedSize > 0) {
							if (::SetClipboardData(ClipboardSettings::formatUnicodeText, hData)) {
								hData = NULL;
								m_lengthText = nConvertedSize * sizeof(wchar_t);
							} else {
								bFailed = true;
							}
						}
					}
				}

				if (hData) {
					GlobalFree(hData);
				}
			}
			if (pIncomingText) {
				delete[] pIncomingText;
			}
			if (bFailed) {
				return false;
			}
		}
	}

	if (extendedClipboardDataMessage.HasFlag(clipRTF)) {
		length = (int)compressedStream.readU32();

		if (length > 0) {
			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, length);

			if (!hData) return false;

			BYTE* pData = (BYTE*)GlobalLock(hData);

			if (pData) {
				compressedStream.readBytes(pData, length);

				GlobalUnlock(hData);

				if (::SetClipboardData(ClipboardSettings::formatRTF, hData)) {
					hData = NULL;
					m_lengthRTF = length;
				} else {
					bFailed = true;
				}
			}

			if (hData) {
				GlobalFree(hData);
			}
		}
	}
	
	if (extendedClipboardDataMessage.HasFlag(clipHTML)) {
		length = (int)compressedStream.readU32();

		if (length > 0) {
			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, length);

			if (!hData) return false;

			BYTE* pData = (BYTE*)GlobalLock(hData);

			if (pData) {
				compressedStream.readBytes(pData, length);

				GlobalUnlock(hData);

				if (!::SetClipboardData(ClipboardSettings::formatHTML, hData)) {
					hData = NULL;
					m_lengthHTML = length;
				} else {
					bFailed = true;
				}
			}

			if (hData) {
				GlobalFree(hData);
			}
		}
	}

	if (extendedClipboardDataMessage.HasFlag(clipDIB)) {
		length = (int)compressedStream.readU32();

		if (length > 0) {
			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, length);

			if (!hData) return false;

			BYTE* pData = (BYTE*)GlobalLock(hData);

			if (pData) {
				compressedStream.readBytes(pData, length);

				GlobalUnlock(hData);

				if (!::SetClipboardData(ClipboardSettings::formatDIB, hData)) {
					hData = NULL;
					m_lengthDIB = length;
				} else {
					bFailed = true;
				}
			}

			if (hData) {
				GlobalFree(hData);
			}
		}
	}

	// we can ignore everything else

	return true;
}

Clipboard::Clipboard(CARD32 caps)
	: m_bNeedToProvide(false)
	, m_bNeedToNotify(false)
	, m_crc(0)
	, m_notifiedRemoteFormats(0)
	, settings(caps)
{
}

// returns true if something changed
bool Clipboard::UpdateClipTextEx(ClipboardData& clipboardData, CARD32 overrideFlags)
{
	if (m_crc == clipboardData.m_crc && overrideFlags == 0) {
		return false;
	}

	if (overrideFlags & clipPeek) {
		// don't reset anything regarding providing new data, just 'include' any
		// formats into the notify response.
		if (clipboardData.m_lengthText != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipText);
			m_bNeedToNotify = true;
		}
		if (clipboardData.m_lengthRTF != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipRTF);
			m_bNeedToNotify = true;
		}
		if (clipboardData.m_lengthHTML != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipHTML);
			m_bNeedToNotify = true;
		}
		if (clipboardData.m_lengthDIB != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipDIB);
			m_bNeedToNotify = true;
		}
	} else {

		m_bNeedToProvide = false;
		m_bNeedToNotify = false;
		extendedClipboardDataMessage.Reset();
		extendedClipboardDataNotifyMessage.Reset();

		extendedClipboardDataMessage.AddFlag(clipProvide);
		extendedClipboardDataNotifyMessage.AddFlag(clipNotify);

		rdr::MemOutStream memStream;
		{
			rdr::ZlibOutStream compressedStream(&memStream, 0, 9); //Z_BEST_COMPRESSION

			if (clipboardData.m_lengthText != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipText);

				if (clipboardData.m_lengthText <= settings.m_nLimitText || (overrideFlags & clipText)) {
					compressedStream.writeU32(clipboardData.m_lengthText);
					compressedStream.writeBytes(clipboardData.m_pDataText, clipboardData.m_lengthText);
					extendedClipboardDataMessage.AddFlag(clipText);
					m_bNeedToProvide = true;
				} else {
					m_bNeedToNotify = true;
				}
			}
			if (clipboardData.m_lengthRTF != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipRTF);

				if (clipboardData.m_lengthRTF <= settings.m_nLimitRTF || (overrideFlags & clipRTF)) {
					compressedStream.writeU32(clipboardData.m_lengthRTF);
					compressedStream.writeBytes(clipboardData.m_pDataRTF, clipboardData.m_lengthRTF);
					extendedClipboardDataMessage.AddFlag(clipRTF);
					m_bNeedToProvide = true;
				} else {
					m_bNeedToNotify = true;
				}
			}
			if (clipboardData.m_lengthHTML != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipHTML);

				if (clipboardData.m_lengthHTML <= settings.m_nLimitHTML || (overrideFlags & clipHTML)) {
					compressedStream.writeU32(clipboardData.m_lengthHTML);
					compressedStream.writeBytes(clipboardData.m_pDataHTML, clipboardData.m_lengthHTML);
					extendedClipboardDataMessage.AddFlag(clipHTML);
					m_bNeedToProvide = true;
				} else {
					m_bNeedToNotify = true;
				}
			}
			if (clipboardData.m_lengthDIB != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipDIB);

				if (clipboardData.m_lengthDIB <= settings.m_nLimitDIB || (overrideFlags & clipDIB)) {
					compressedStream.writeU32(clipboardData.m_lengthDIB);
					compressedStream.writeBytes(clipboardData.m_pDataDIB, clipboardData.m_lengthDIB);
					extendedClipboardDataMessage.AddFlag(clipDIB);
					m_bNeedToProvide = true;
				} else {
					m_bNeedToNotify = true;
				}
			}

			compressedStream.flush();
		}

		if (m_bNeedToProvide) {
			extendedClipboardDataMessage.AppendBytes((BYTE*)memStream.data(), memStream.length());
			memStream.clear();
		}

		m_strLastCutText = "";
		if (!settings.m_bSupportsEx && clipboardData.m_lengthText > 0 && clipboardData.m_lengthText < settings.m_nLimitText) {			
			// now we have to translate to UTF-16
			int nConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)clipboardData.m_pDataText, clipboardData.m_lengthText, NULL, 0);

			if (nConvertedSize > 0) {
				wchar_t* clipStr = new wchar_t[nConvertedSize];
				int nFinalConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)clipboardData.m_pDataText, clipboardData.m_lengthText, (LPWSTR)clipStr, nConvertedSize);

				if (nFinalConvertedSize > 0) {
					std::wstring wstrClipboard(clipStr);
					m_strLastCutText.assign(wstrClipboard.begin(), wstrClipboard.end());
				}

				delete[] clipStr;
			}
		}

		m_crc = clipboardData.m_crc;

		if ( (!(settings.m_remoteCaps & clipNotify)) || (overrideFlags != 0) ) {
			m_bNeedToNotify = false;
			extendedClipboardDataNotifyMessage.Reset();
		}
	}

	return m_bNeedToProvide || m_bNeedToNotify;
}
