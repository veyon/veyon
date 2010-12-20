/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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


// System headers
#include <assert.h>
#include "stdhdrs.h"

// Custom headers
#include <omnithread.h>
#include "winvnc.h"
#include "vnchooks/VNCHooks.h"
#include "vncserver.h"
#include "rfbRegion.h"
#include "rfbRect.h"
#include "vncdesktop.h"
#include "vncservice.h"

BOOL vncDesktop:: CalculateSWrect(RECT &rect)
{

	if (!m_Single_hWnd)
	{
		m_server->SingleWindow(false);
		m_SWtoDesktop=TRUE;
		rect.top=0;
		rect.left=0;
		rect.right=m_scrinfo.framebufferWidth;
		rect.bottom=m_scrinfo.framebufferHeight;
		return FALSE;
	}

	if (IsIconic(m_Single_hWnd))
	{
		m_server->SingleWindow(false);
		m_Single_hWnd=NULL;
		m_SWtoDesktop=TRUE;
		rect.top=0;
		rect.left=0;
		rect.right=m_scrinfo.framebufferWidth;
		rect.bottom=m_scrinfo.framebufferHeight;
		return FALSE;
	}
	if ( IsWindowVisible(m_Single_hWnd) && GetWindowRect(m_Single_hWnd,&rect)) 
		{
			RECT taskbar;
			rect.top=Max(rect.top,0);
			rect.left=Max(rect.left,0);
			rect.right=Min(rect.right,m_scrinfo.framebufferWidth);
			rect.bottom=Min(rect.bottom,m_scrinfo.framebufferHeight);
			APPBARDATA pabd;
			pabd.cbSize=sizeof(APPBARDATA);
			SHAppBarMessage(ABM_GETTASKBARPOS, &pabd);
			taskbar.top=Max(pabd.rc.top,0);
			taskbar.left=Max(pabd.rc.left,0);
			taskbar.right=Min(pabd.rc.right,m_scrinfo.framebufferWidth);
			taskbar.bottom=Min(pabd.rc.bottom,m_scrinfo.framebufferHeight);
			///desktop
			if (rect.top==0 && rect.left== 0&& rect.right==m_scrinfo.framebufferWidth && rect.bottom==m_scrinfo.framebufferHeight)
				{
					m_server->SingleWindow(false);
					m_Single_hWnd=NULL;
					rect.top=0;
					rect.left=0;
					rect.right=m_scrinfo.framebufferWidth;
					rect.bottom=m_scrinfo.framebufferHeight;
					return TRUE;
				}
			//taskbar
			if (rect.top>=taskbar.top && rect.left== taskbar.left&& rect.right==taskbar.right && rect.bottom==taskbar.bottom)
			
				{
					rect.top=taskbar.top;
					rect.left=taskbar.left;
					rect.right=taskbar.right;
					rect.bottom=taskbar.bottom;
					if ((m_SWHeight!=(rect.bottom-rect.top)) || (m_SWWidth!=(rect.right-rect.left)))
					m_SWSizeChanged=TRUE;
					m_SWHeight=rect.bottom-rect.top;
					m_SWWidth=rect.right-rect.left;
					return TRUE;	
				}
			//eliminate other little windows
			if ((m_SWHeight!=(rect.bottom-rect.top)) || (m_SWWidth!=(rect.right-rect.left)))
				m_SWSizeChanged=TRUE;
			//vnclog.Print(LL_INTINFO, VNCLOG("screen format %d %d %d %d\n"),
			//		rect.top,
			//		rect.bottom,rect.right,rect.left);
			if ((rect.bottom-rect.top)<64||(rect.right-rect.left)<128 || rect.bottom<0 ||rect.top<0 || rect.right<0 ||
				rect.left<0 || rect.bottom>m_scrinfo.framebufferHeight||rect.top>m_scrinfo.framebufferHeight||
				rect.right>m_scrinfo.framebufferWidth||rect.left>m_scrinfo.framebufferWidth)
			{
				m_server->SingleWindow(false);
				m_Single_hWnd=NULL;
				m_SWtoDesktop=TRUE;
				rect.top=0;
				rect.left=0;
				rect.right=m_scrinfo.framebufferWidth;
				rect.bottom=m_scrinfo.framebufferHeight;
				m_SWSizeChanged=FALSE;
				return FALSE;
			}


			m_SWHeight=rect.bottom-rect.top;
			m_SWWidth=rect.right-rect.left;
			return TRUE;
		}		
	m_server->SingleWindow(false);
	m_Single_hWnd=NULL;
	m_SWtoDesktop=TRUE;
	rect.top=0;
	rect.left=0;
	rect.right=m_scrinfo.framebufferWidth;
	rect.bottom=m_scrinfo.framebufferHeight;
	return FALSE;
}

void vncDesktop::SWinit()
{
	m_Single_hWnd=NULL;
	m_Single_hWnd_backup=NULL;
	m_SWHeight=0;
	m_SWWidth=0;
	m_SWSizeChanged=FALSE;
	m_SWmoved=FALSE;
	m_SWOffsetx=0;
	m_SWOffsety=0;
	vnclog.Print(LL_INTINFO, VNCLOG("SWinit \n"));
}

rfb::Rect
vncDesktop::GetSize()
{
	//vnclog.Print(LL_INTINFO, VNCLOG("GetSize \n"));
	if (m_server->SingleWindow())
		{
		RECT rect;
		rect.left=m_Cliprect.tl.x;
		rect.top=m_Cliprect.tl.y;
		rect.right=m_Cliprect.br.x;
		rect.bottom=m_Cliprect.br.y;
		return rfb::Rect(rect.left, rect.top, rect.right, rect.bottom);
		
		}
	else if (!m_videodriver)
		{
		m_SWOffsetx=0;
		m_SWOffsety=0;
		return rfb::Rect(0, 0, m_scrinfo.framebufferWidth, m_scrinfo.framebufferHeight);
		}
	 else
		{
			if (multi_monitor)
				return rfb::Rect(0,0,mymonitor[2].Width,mymonitor[2].Height);	
			else
				return rfb::Rect(0,0,mymonitor[0].Width,mymonitor[0].Height);
		}
}

rfb::Rect
vncDesktop::GetQuarterSize()
{
	vnclog.Print(LL_INTINFO, VNCLOG("GetQuarterSize \n"));
if (m_server->SingleWindow())
	{
	RECT rect;
	if (CalculateSWrect(rect))
	{
		rect.left=rect.left-m_ScreenOffsetx;
		rect.right=rect.right-m_ScreenOffsetx;
		rect.bottom=rect.bottom-m_ScreenOffsety;
		rect.top=rect.top-m_ScreenOffsety;
		if (m_SWOffsetx!=rect.left || m_SWOffsety!=rect.top) m_SWmoved=TRUE;
		m_SWOffsetx=rect.left;
		m_SWOffsety=rect.top;
		m_Cliprect.tl.x=rect.left;
		m_Cliprect.tl.y=rect.top;
		m_Cliprect.br.x=rect.right;
		m_Cliprect.br.y=rect.bottom;
		return rfb::Rect(rect.left, rect.bottom, rect.right-rect.left, (rect.bottom-rect.top)/4);
	}
	else
	{
	m_SWOffsetx=0;
	m_SWOffsety=0;
	m_Cliprect.tl.x=0;
	m_Cliprect.tl.y=0;
	m_Cliprect.br.x=m_bmrect.br.x;
	m_Cliprect.br.y=m_bmrect.br.y;
	return rfb::Rect(0, 0, m_bmrect.br.x, m_bmrect.br.y/4);

	}
	}
else
	{ 
	m_SWOffsetx=0;
	m_SWOffsety=0;
	m_Cliprect.tl.x=0;
	m_Cliprect.tl.y=0;
	m_Cliprect.br.x=m_bmrect.br.x;
	m_Cliprect.br.y=m_bmrect.br.y;
	vnclog.Print(LL_INTINFO, VNCLOG("GetQuarterSize \n"));
	return rfb::Rect(0, 0, m_bmrect.br.x, m_bmrect.br.y/4);
	}
}

