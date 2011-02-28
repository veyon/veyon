//  Copyright (C) 2002-2003 RealVNC Ltd. All Rights Reserved.
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
// which you received this file, check http://www.realvnc.com/ or contact
// the authors on info@realvnc.com for information on obtaining it.

// Cross-platform Region class based on the X11 region implementation

#include "stdhdrs.h"
#include "rfbRegion_win32.h"

using namespace rfb;

void dump_rects(char *n, HRGN rgn, bool dbg = 0)
{
/*#if defined(_DEBUG)
  //vnclog.Print(LL_INTWARN, VNCLOG("Region %s %p contains rects:\n"), n, rgn);
	DWORD buffsize = GetRegionData(rgn, 0, NULL);
	if (!buffsize)
		return ;

	unsigned char* buffer = new unsigned char[buffsize];

	if (GetRegionData(rgn, buffsize, (LPRGNDATA)buffer) != buffsize)
	{
		delete [] buffer;
		return ;
	}

	bool stop_needed(dbg && strcmp(n, "rgncache")==0);
	LPRGNDATA region_data = (LPRGNDATA)buffer;
	DWORD nRects = region_data->rdh.nCount;

	if (nRects == 0)
	{
		delete [] buffer;
		return ;
	}

	static bool enabled(false);
	RECT *pRectangles = reinterpret_cast<RECT*>(&region_data->Buffer[0]);
	for (DWORD i = 0; i < nRects; ++i)
	{
		//vnclog.Print(LL_INTWARN, VNCLOG("           (%u,%u - %u,%u)\n"), pRectangles[i].left, pRectangles[i].top,pRectangles[i].right, pRectangles[i].bottom);

		if (stop_needed && enabled)
		{
			if (pRectangles[i].left == 0 &&pRectangles[i].top ==0 &&
				pRectangles[i].right == 1280 && pRectangles[i].bottom == 1024)
				DebugBreak();
		}
	}

	delete [] buffer;
#endif*/
}


Region::Region() : m_name(_strdup("")) {
  rgn = CreateRectRgn(0,0,0,0);
}

Region::Region(int x1, int y1, int x2, int y2) : m_name(_strdup("")) {
  rgn = CreateRectRgn(x1, y1, x2, y2);
}

Region::Region(const Rect& r) : m_name(_strdup("")) {
  rgn = CreateRectRgn(r.tl.x, r.tl.y, r.br.x, r.br.y);
}

Region::Region(const Region& r) : m_name(_strdup("")) {
  rgn = CreateRectRgn(0,0,0,0);
  CombineRgn(rgn, r.rgn, r.rgn, RGN_COPY);
#if defined(_DEBUG)
  //vnclog.Print(LL_INTWARN, VNCLOG("Copy-construct Region %p from %s %p\n"), rgn, r.m_name, r.rgn);
  dump_rects(m_name, rgn);
#endif
}

Region::~Region() {
#if defined(_DEBUG)
  //vnclog.Print(LL_INTWARN, VNCLOG("Deleting Region %s %p\n"), m_name, rgn);
#endif
  DeleteObject(rgn);
  free(m_name);
}

bool Region::IsPtInRegion(int x, int y)
{
	return PtInRegion(rgn,x,y);
}

rfb::Region& Region::operator=(const Region& r) {
  clear();
#if defined(_DEBUG)
  //vnclog.Print(LL_INTWARN, VNCLOG("Assign Region %s %p from %s %p\n"), m_name, rgn, r.m_name, r.rgn);
  dump_rects(m_name, rgn);
#endif
  CombineRgn(rgn, r.rgn, r.rgn, RGN_COPY);
  return *this;
}


void Region::clear() {
  SetRectRgn(rgn, 0, 0, 0, 0);
}

void Region::reset(const Rect& r) {
  SetRectRgn(rgn, r.tl.x, r.tl.y, r.br.x, r.br.y);
}

void Region::translate(const Point& delta) {
  OffsetRgn(rgn, delta.x, delta.y);
}


void Region::assign_intersect(const Region& r) {
  CombineRgn(rgn, rgn, r.rgn, RGN_AND);
#if defined(_DEBUG)
  //vnclog.Print(LL_INTWARN, VNCLOG("assign_intersect this %s %p, other %s %p\n"), m_name, rgn, r.m_name, r.rgn);
  dump_rects(m_name, rgn);
#endif
}

void Region::assign_union(const Region& r) {
  CombineRgn(rgn, rgn, r.rgn, RGN_OR);
#if defined(_DEBUG)
  //vnclog.Print(LL_INTWARN, VNCLOG("assign_union this %s %p, other %s %p\n"),  m_name, rgn, r.m_name, r.rgn);
  dump_rects(m_name, rgn, 1);
#endif
}

void Region::assign_subtract(const Region& r) {
  CombineRgn(rgn, rgn, r.rgn, RGN_DIFF);
#if defined(_DEBUG)
  //vnclog.Print(LL_INTWARN, VNCLOG("assign_subtract %s %p, other %s %p\n"), m_name, rgn, r.m_name, r.rgn);
  dump_rects(m_name, rgn);
#endif
}


rfb::Region Region::intersect(const Region& r) const {
  Region t = *this;
  t.assign_intersect(r);
  return t;
}

rfb::Region Region::union_(const Region& r) const {
  Region t = *this;
  t.assign_union(r);
  return t;
}

rfb::Region Region::subtract(const Region& r) const {
  Region t = *this;
  t.assign_subtract(r);
  return t;
}


bool Region::equals(const Region& b) const {
  return EqualRgn(rgn, b.rgn) ? true : false;
}

bool Region::is_empty() const {
  RECT rc;
  return (GetRgnBox(rgn, &rc) == NULLREGION);
}

bool Region::get_rects(std::vector<Rect>& rects,
					   bool left2right,
					   bool topdown) const {
#if defined(_DEBUG)
   //vnclog.Print(LL_INTWARN, VNCLOG("get_rects %s %p\n"), m_name, rgn);
#endif

   DWORD buffsize = GetRegionData(rgn, 0, NULL);
   if (!buffsize)
	   return false;


   unsigned char* buffer = new unsigned char[buffsize];
   memset(buffer,0,buffsize);

   if (GetRegionData(rgn, buffsize, (LPRGNDATA)buffer) != buffsize)
   {
	   delete [] buffer;
	   return false;
   }

   LPRGNDATA region_data = (LPRGNDATA)buffer;
   DWORD nRects = region_data->rdh.nCount;

   if (nRects == 0)
   {
	   delete [] buffer;
	   return false;
   }

   rects.clear();
   rects.reserve(nRects);
   RECT *pRectangles = reinterpret_cast<RECT*>(&region_data->Buffer[0]);

   int xInc = left2right ? 1 : -1;
   int yInc = topdown ? 1 : -1;
   int i = topdown ? 0 : nRects-1;
#if defined(_DEBUG)
   //vnclog.Print(LL_INTWARN, VNCLOG("Region %s %p contains rects:\n"), m_name, rgn);
#endif

   while (nRects > 0) {
	   int firstInNextBand = i;
	   int nRectsInBand = 0;

	   while (nRects > 0 && pRectangles[firstInNextBand].top == pRectangles[i].top) {
		   firstInNextBand += yInc;
		   nRects--;
		   nRectsInBand++;
	   }

	   if (xInc != yInc)
		   i = firstInNextBand - yInc;

	   while (nRectsInBand > 0) {
		   Rect r(pRectangles[i].left, pRectangles[i].top,
			   pRectangles[i].right, pRectangles[i].bottom);

#if defined(_DEBUG)
		   //vnclog.Print(LL_INTWARN, VNCLOG("           (%u,%u - %u,%u) %ux%u\n"), r.tl.x, r.tl.y, r.br.x, r.br.y, r.width(), r.height());
#endif
		   rects.push_back(r);
		   i += xInc;
		   nRectsInBand--;
	   }

	   i = firstInNextBand;
   }
   delete [] buffer;

   return !rects.empty();
}

rfb::Rect Region::get_bounding_rect() const {
  RECT rc;
  GetRgnBox(rgn, &rc);
  return Rect(rc.left, rc.top, rc.right, rc.bottom);
}

int Region::Numrects()
{
	char *pRgnData;
	DWORD dwCount(0);
	int nRects(0);

	dwCount = GetRegionData(rgn, 0, 0);
	pRgnData = new char[dwCount];
	if (pRgnData == 0) return 0;

	if (GetRegionData(rgn, dwCount, reinterpret_cast<LPRGNDATA>(pRgnData)) == dwCount)
		nRects = reinterpret_cast<LPRGNDATA>(pRgnData)->rdh.nCount;

	delete [] pRgnData;
	return nRects;
}

