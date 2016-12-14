#ifdef _USE_DESKTOPDUPLICATION
#ifndef __DESK
#define __DESK
#include <winsock2.h>
#include <windows.h>
#include <new>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <sal.h>
#include <warning.h>
#include <DirectXMath.h>
#include "IPC.h"

class ONEDESKTOP
{
    public:
		ONEDESKTOP();        
        ~ONEDESKTOP();

		void SETONEDESKTOP(int desknr,ID3D11Device* m_Device,ID3D11DeviceContext* m_DeviceContext);
		int INITDESK(mystruct *plist_IN);
		int UPDATEDESK(int offsetx,int offsety,int width,int height,int screenofsetx,int screenofsety, int ScreenWidth,int ScreenHeight ,unsigned char *buffer);
		int CLOSEDESK();
    private:
		ID3D11Device* m_Device;
		ID3D11DeviceContext* m_DeviceContext;
		IDXGIResource* DesktopResource;
		IDXGIOutputDuplication* m_DeskDupl;
		ID3D11Texture2D* m_AcquiredDesktopImage;
		UINT m_MetaDataSize;
		_Field_size_bytes_(m_MetaDataSize) BYTE* m_MetaDataBuffer;
		int Desknr;
		void ONEDESKTOP::Addrect(int type, int x1, int y1, int x2, int y2, int x11, int y11, int x22, int y22);
		mystruct *plist;
		bool frameaquired;
};

class DESKTOPMANAGER
{
    public:
        DESKTOPMANAGER();
        ~DESKTOPMANAGER();
		unsigned char *STARTDESKTOPMANAGER(bool onlyprim);
		int STOPDESKTOPMANAGER();		
		int CAPTUREDESKTOPMANAGER();
		mystruct list;

    private:
		int SETOFFSETANDDESKTOPSIZES();
		int SETOFFSETANDDESKTOPSIZES(int i);
		ID3D11Device* m_Device;
		ID3D11DeviceContext* m_DeviceContext;
		RECT DeskBounds;
		INT ScreenNr;
		INT ScreenWidth;
		INT ScreenHeight;
		INT ScreenOffSetX;
		INT ScreenOffSetY;
		RECT *screenrect;
		INT *OffesetX;
		bool portrait[10];
		INT *OffesetY;	
		ONEDESKTOP *ondesktop;
		unsigned char *buffer;
		CRITICAL_SECTION CriticalSection;		
};

#endif
#endif