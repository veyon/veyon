//   Copyright (C) 2002-2003 RealVNC Ltd. All Rights Reserved.
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

// rfb::Region2D class for Win32

#ifndef __RFB_REGION_WIN32_INCLUDED__
#define __RFB_REGION_WIN32_INCLUDED__

#include "rfbRect.h"
#include <vector>

namespace rfb {

	// rfb::Region2D
	//
	// See the rfbRegion.h header for documentation.

  class Region {
  public:
    // Create an empty region
    Region();
    // Create a rectangular region
    Region(int x1, int y1, int x2, int y2);
    Region(const Rect& r);

    Region(const Region& r);
    Region &operator=(const Region& src);
	bool IsPtInRegion(int x, int y);

    ~Region();

    // the following methods alter the region in place:

    void clear();
    void reset(const Rect& r);
    void translate(const rfb::Point& delta);
    void setOrderedRects(const std::vector<Rect>& rects);

	void name(const char *n)
	{
		free(m_name);
		m_name = _strdup(n);
	}

    void assign_intersect(const Region& r);
    void assign_union(const Region& r);
    void assign_subtract(const Region& r);

    // the following three operations return a new region:

    Region intersect(const Region& r) const;
    Region union_(const Region& r) const;
    Region subtract(const Region& r) const;

    bool equals(const Region& b) const;
    bool is_empty() const;

    bool get_rects(std::vector<Rect>& rects, bool left2right=true,
                   bool topdown=true) const;
    Rect get_bounding_rect() const;
	int Numrects();

    void debug_print(const char *prefix) const;

  protected:
	  HRGN rgn;
	  char *m_name;
  };
  typedef Region Region2D;

};

#endif /* __RFB_REGION_WIN32_INCLUDED__ */


