//  Copyright (C) 2000 Const Kaplinsky. All Rights Reserved.

#include "vncEncoder.h"
#include "vncBuffer.h"
#include "vncDesktop.h"

//
// New code implementing cursor shape updates.
//
BOOL
vncEncoder::IsXCursorSupported()
{
	if (m_use_xcursor) return true;
	if (m_use_richcursor) return true;
	return false;
}
BOOL
vncEncoder::SendEmptyCursorShape(VSocket *outConn)
{
	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = Swap16IfLE(0);
	hdr.r.y = Swap16IfLE(0);
	hdr.r.w = Swap16IfLE(0);
	hdr.r.h = Swap16IfLE(0);
	if (m_use_xcursor) {
		hdr.encoding = Swap32IfLE(rfbEncodingXCursor);
	} else {
		hdr.encoding = Swap32IfLE(rfbEncodingRichCursor);
	}

	return outConn->SendExactQueue((char *)&hdr, sizeof(hdr));
}

BOOL
vncEncoder::SendCursorShape(VSocket *outConn, vncDesktop *desktop)
{
	// Make sure the function is used correctly
	if (!m_use_xcursor && !m_use_richcursor)
		return FALSE;

	// Check mouse cursor handle
	HCURSOR hcursor = desktop->GetCursor();
	if (hcursor == NULL) {
		vnclog.Print(LL_INTINFO, VNCLOG("cursor handle is NULL.\n"));
		return FALSE;
	}

	// Get cursor info
	ICONINFO IconInfo;
	if (!GetIconInfo(hcursor, &IconInfo)) {
		vnclog.Print(LL_INTINFO, VNCLOG("GetIconInfo() failed.\n"));
		return FALSE;
	}
	BOOL isColorCursor = FALSE;
	if (IconInfo.hbmColor != NULL) {
		isColorCursor = TRUE;
		DeleteObject(IconInfo.hbmColor);
	}
	if (IconInfo.hbmMask == NULL) {
		vnclog.Print(LL_INTINFO, VNCLOG("cursor bitmap handle is NULL.\n"));
		return FALSE;
	}

	// Check bitmap info for the cursor
	BITMAP bmMask;
	if (!GetObject(IconInfo.hbmMask, sizeof(BITMAP), (LPVOID)&bmMask)) {
		vnclog.Print(LL_INTINFO, VNCLOG("GetObject() for bitmap failed.\n"));
		DeleteObject(IconInfo.hbmMask);
		return FALSE;
	}
	if (bmMask.bmPlanes != 1 || bmMask.bmBitsPixel != 1) {
		vnclog.Print(LL_INTINFO, VNCLOG("incorrect data in cursor bitmap.\n"));
		DeleteObject(IconInfo.hbmMask);
		return FALSE;
	}

	// Get monochrome bitmap data for cursor
	// NOTE: they say we should use GetDIBits() instead of GetBitmapBits().
	BYTE *mbits = new BYTE[bmMask.bmWidthBytes * bmMask.bmHeight];
	if (mbits == NULL)
		return FALSE;

	BOOL success = GetBitmapBits(IconInfo.hbmMask,
								 bmMask.bmWidthBytes * bmMask.bmHeight, mbits);
	DeleteObject(IconInfo.hbmMask);

	if (!success) {
		vnclog.Print(LL_INTINFO, VNCLOG("GetBitmapBits() failed.\n"));
		delete[] mbits;
		return FALSE;
	}

	// Compute cursor dimensions
	int width = bmMask.bmWidth;
	int height = (isColorCursor) ? bmMask.bmHeight : bmMask.bmHeight/2;

	// Call appropriate routine to send cursor shape update
	if (!isColorCursor && m_use_xcursor) {
		FixCursorMask(mbits, NULL, width, bmMask.bmHeight, bmMask.bmWidthBytes);
		success = SendXCursorShape(outConn, mbits,
								   IconInfo.xHotspot, IconInfo.yHotspot,
								   width, height);
	}
	else if (m_use_richcursor) {
		int cbits_size = width * height * 4;
		BYTE *cbits = new BYTE[cbits_size];
		if (cbits == NULL) {
			delete[] mbits;
			return FALSE;
		}
		if (!desktop->GetRichCursorData(cbits, hcursor, width, height)) {
			vnclog.Print(LL_INTINFO, VNCLOG("vncDesktop::GetRichCursorData() failed.\n"));
			delete[] mbits;
			delete[] cbits;
			return FALSE;
		}
		FixCursorMask(mbits, cbits, width, height, bmMask.bmWidthBytes);
		success = SendRichCursorShape(outConn, mbits, cbits,
									  IconInfo.xHotspot, IconInfo.yHotspot,
									  width, height);
		delete[] cbits;
	}
	else {
		success = FALSE;	// FIXME: We could convert RichCursor -> XCursor.
	}

	// Cleanup
	delete[] mbits;

	return success;
}

BOOL
vncEncoder::SendXCursorShape(VSocket *outConn, BYTE *mask,
							 int xhot, int yhot, int width, int height)
{
	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = Swap16IfLE(xhot);
	hdr.r.y = Swap16IfLE(yhot);
	hdr.r.w = Swap16IfLE(width);
	hdr.r.h = Swap16IfLE(height);
	hdr.encoding = Swap32IfLE(rfbEncodingXCursor);

	BYTE colors[6] = { 0, 0, 0, 0xFF, 0xFF, 0xFF };
	int maskRowSize = (width + 7) / 8;
	int maskSize = maskRowSize * height;

	if ( !outConn->SendExactQueue((char *)&hdr, sizeof(hdr)) ||
		 !outConn->SendExactQueue((char *)colors, 6) ||
		 !outConn->SendExactQueue((char *)&mask[maskSize], maskSize) ||
		 !outConn->SendExactQueue((char *)mask, maskSize) ) {
		return FALSE;
	}
	return TRUE;
}

BOOL
vncEncoder::SendRichCursorShape(VSocket *outConn, BYTE *mbits, BYTE *cbits,
								int xhot, int yhot, int width, int height)
{
	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = Swap16IfLE(xhot);
	hdr.r.y = Swap16IfLE(yhot);
	hdr.r.w = Swap16IfLE(width);
	hdr.r.h = Swap16IfLE(height);
	hdr.encoding = Swap32IfLE(rfbEncodingRichCursor);

	// Cet cursor image in local pixel format
	int srcbuf_rowsize = width * (m_localformat.bitsPerPixel / 8);
	while (srcbuf_rowsize % sizeof(DWORD))
		srcbuf_rowsize++;	// Actually, this should never happen

	// Translate image to client pixel format
	int dstbuf_size = width * height * (m_remoteformat.bitsPerPixel / 8);
	BYTE *dstbuf = new BYTE[dstbuf_size];
	Translate(cbits, dstbuf, width, height, srcbuf_rowsize);

	// Send the data
	int mask_rowsize = (width + 7) / 8;
	int mask_size = mask_rowsize * height;
	if ( !outConn->SendExactQueue((char *)&hdr, sizeof(hdr)) ||
		 !outConn->SendExactQueue((char *)dstbuf, dstbuf_size) ||
		 !outConn->SendExactQueue((char *)mbits, mask_size) ) {
		delete[] dstbuf;
		return FALSE;
	}
	delete[] dstbuf;
	return TRUE;
}

void
vncEncoder::FixCursorMask(BYTE *mbits, BYTE *cbits,
						  int width, int height, int width_bytes)
{
	int packed_width_bytes = (width + 7) / 8;

	// Pack and invert bitmap data (mbits)
	int x, y;
	for (y = 0; y < height; y++)
		for (x = 0; x < packed_width_bytes; x++)
			mbits[y * packed_width_bytes + x] = ~mbits[y * width_bytes + x];

	// Replace "inverted background" bits with black color to ensure
	// cross-platform interoperability. Not beautiful but necessary code.
	if (cbits == NULL) {
		BYTE m, c;
		height /= 2;
		for (y = 0; y < height; y++) {
			for (x = 0; x < packed_width_bytes; x++) {
				m = mbits[y * packed_width_bytes + x];
				c = mbits[(height + y) * packed_width_bytes + x];
				mbits[y * packed_width_bytes + x] |= ~(m | c);
				mbits[(height + y) * packed_width_bytes + x] |= ~(m | c);
			}
		}
	} else {
		int bytes_pixel = m_localformat.bitsPerPixel / 8;
		int bytes_row = width * bytes_pixel;
		while (bytes_row % sizeof(DWORD))
			bytes_row++;	// Actually, this should never happen

		BYTE bitmask;
		int b1, b2;
		for (y = 0; y < height; y++) {
			bitmask = 0x80;
			for (x = 0; x < width; x++) {
				if ((mbits[y * packed_width_bytes + x / 8] & bitmask) == 0) {
					for (b1 = 0; b1 < bytes_pixel; b1++) {
						if (cbits[y * bytes_row + x * bytes_pixel + b1] != 0) {
							mbits[y * packed_width_bytes + x / 8] ^= bitmask;
							for (b2 = b1; b2 < bytes_pixel; b2++)
								cbits[y * bytes_row + x * bytes_pixel + b2] = 0x00;
							break;
						}
					}
				}
				if ((bitmask >>= 1) == 0)
					bitmask = 0x80;
			}
		}
	}
}

// Translate a rectangle (using arbitrary m_bytesPerRow value,
// always translating from the beginning of the source pixel array)
// NOTE: overloaded function!
inline void
vncEncoder::Translate(BYTE *source, BYTE *dest, int w, int h, int bytesPerRow)
{
	// Call the translation function
	(*m_transfunc) (m_transtable, &m_localformat, &m_transformat,
					(char *)source, (char *)dest, bytesPerRow, w, h);
}