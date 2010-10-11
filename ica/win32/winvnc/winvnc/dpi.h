#include <windows.h>
#ifndef DPI_H__
#define DPI_H__

class CDPI
{
public:
    CDPI() : _fInitialized(false), _dpiX(96), _dpiY(96) { }
    
    // Get screen DPI.
    int GetDPIX() { _Init(); return _dpiX; }
    int GetDPIY() { _Init(); return _dpiY; }

    // Convert between raw pixels and relative pixels.
    int ScaleX(int x) { _Init(); return MulDiv(x, _dpiX, 96); }
    int ScaleY(int y) { _Init(); return MulDiv(y, _dpiY, 96); }
    int UnscaleX(int x) { _Init(); return MulDiv(x, 96, _dpiX); }
    int UnscaleY(int y) { _Init(); return MulDiv(y, 96, _dpiY); }

    // Determine the screen dimensions in relative pixels.
    int ScaledScreenWidth() { return _ScaledSystemMetricX(SM_CXSCREEN); }
    int ScaledScreenHeight() { return _ScaledSystemMetricY(SM_CYSCREEN); }
	int ScaledScreenVirtualWidth() { return _ScaledSystemMetricX(SM_CXVIRTUALSCREEN); }
    int ScaledScreenVirtualHeight() { return _ScaledSystemMetricY(SM_CYVIRTUALSCREEN); }
	int ScaledScreenVirtualX() { return _ScaledSystemMetricX(SM_XVIRTUALSCREEN); }
    int ScaledScreenVirtualY() { return _ScaledSystemMetricY(SM_YVIRTUALSCREEN); }

    // Scale rectangle from raw pixels to relative pixels.
    void ScaleRect(__inout RECT *pRect)
    {
        pRect->left = ScaleX(pRect->left);
        pRect->right = ScaleX(pRect->right);
        pRect->top = ScaleY(pRect->top);
        pRect->bottom = ScaleY(pRect->bottom);
    }
    // Determine if screen resolution meets minimum requirements in relative
    // pixels.
    bool IsResolutionAtLeast(int cxMin, int cyMin) 
    { 
        return (ScaledScreenWidth() >= cxMin) && (ScaledScreenHeight() >= cyMin); 
    }

    // Convert a point size (1/72 of an inch) to raw pixels.
    int PointsToPixels(int pt) { return MulDiv(pt, _dpiY, 72); }

    // Invalidate any cached metrics.
    void Invalidate() { _fInitialized = false; }

private:
    void _Init()
    {
        if (!_fInitialized)
        {
            HDC hdc = GetDC(NULL);
            if (hdc)
            {
				RECT rect;
				GetClipBox(hdc,&rect);
				int dpiwidth=GetSystemMetrics(SM_CXVIRTUALSCREEN);
				int dpiheight=GetSystemMetrics(SM_CYVIRTUALSCREEN);
				int fullwidth=rect.right-rect.left;
				int fullheight=rect.bottom-rect.top;
				// Seeme GetDC can return NULL, in that case fullwidth=0 and devide by 0
				if (fullwidth==0) fullwidth=dpiwidth;
				if (fullheight==0) fullheight=dpiheight;

                _dpiX = dpiwidth*96/fullwidth;//GetDeviceCaps(hdc, LOGPIXELSX);
                _dpiY = dpiheight*96/fullheight;//GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(NULL, hdc);
            }
            _fInitialized = true;
        }
    }

    int _ScaledSystemMetricX(int nIndex) 
    { 
        _Init(); 
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiX); 
    }

    int _ScaledSystemMetricY(int nIndex) 
    { 
        _Init(); 
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiY); 
    }
private:
    bool _fInitialized;

    int _dpiX;
    int _dpiY;
};
#endif