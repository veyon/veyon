#ifdef _USE_DESKTOPDUPLICATION
#include "desktopmanager.h"
#include <stdio.h>
DESKTOPMANAGER *DESKTOPMANAGER_f=NULL;
//Fix crash
//If we call stopW8 almost simultanious with captureW8
//enter crirical section keeps capture waiting, but as soon as delete is called, the critial section release
//and this DESKTOPMANAGER_f =null
// access error and application crash

bool Stop_initiated = false;


unsigned char * StartW8(bool primonly)
{
	Stop_initiated = false;
	DESKTOPMANAGER_f = NULL;
	DESKTOPMANAGER_f = new DESKTOPMANAGER;
	if (DESKTOPMANAGER_f) return DESKTOPMANAGER_f->STARTDESKTOPMANAGER(primonly);
	else return NULL;
}

mystruct * get_plist()
{
	if (DESKTOPMANAGER_f) return &DESKTOPMANAGER_f->list;
	else return NULL;
}

BOOL StopW8()
{
	if (Stop_initiated) return TRUE;
	Stop_initiated = true;
	if (DESKTOPMANAGER_f) DESKTOPMANAGER_f->STOPDESKTOPMANAGER();
	if (DESKTOPMANAGER_f) delete DESKTOPMANAGER_f;
	DESKTOPMANAGER_f = NULL;
	return TRUE;
}

BOOL CaptureW8()
{
	if (Stop_initiated) return false;
	bool returnvalue=false;
	if (DESKTOPMANAGER_f) returnvalue= DESKTOPMANAGER_f->CAPTUREDESKTOPMANAGER();
	return returnvalue;
}

 D3D_DRIVER_TYPE DriverTypes[] =
 {
       D3D_DRIVER_TYPE_HARDWARE,
       D3D_DRIVER_TYPE_WARP,
       D3D_DRIVER_TYPE_REFERENCE,
 };

// Feature levels supported
D3D_FEATURE_LEVEL FeatureLevels[] =
{
       D3D_FEATURE_LEVEL_11_0,
       D3D_FEATURE_LEVEL_10_1,
       D3D_FEATURE_LEVEL_10_0,
       D3D_FEATURE_LEVEL_9_1
};


DESKTOPMANAGER::DESKTOPMANAGER()
{
    m_Device=nullptr;
	m_DeviceContext=nullptr;
	ScreenNr=0;
	ScreenWidth=0;
	ScreenHeight=0;
	ScreenOffSetX=0;
	ScreenOffSetY=0;
	screenrect=nullptr;
	OffesetX=nullptr;
	OffesetY=nullptr;	
	ondesktop=nullptr;
	buffer=nullptr;
	InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x00000400);
	memset(&list, 0, sizeof(list));
}

DESKTOPMANAGER::~DESKTOPMANAGER()
{
	 if (m_DeviceContext)
    {
		m_DeviceContext->ClearState();
        m_DeviceContext->Release();
        m_DeviceContext = nullptr;
    }

    if (m_Device)
    {
        m_Device->Release();
        m_Device = nullptr;
    }

	if (screenrect) delete [] screenrect;
	if (OffesetX) delete [] OffesetX; 
	if (OffesetY) delete [] OffesetY;
	screenrect=nullptr;
	OffesetX=nullptr;
	OffesetY=nullptr;	
	if (buffer) delete [] buffer;
	buffer=nullptr;
	DeleteCriticalSection(&CriticalSection);
	//g_obIPC2.CloseBitmap();
}

unsigned char * DESKTOPMANAGER::STARTDESKTOPMANAGER(bool onlyprim)
{
	//desk geeft de desktop aan
	// 0= monitor 0
	// -1=alle monitors

    int returnvalue=0;
	HRESULT hr=0;
	EnterCriticalSection(&CriticalSection); 
	
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);    
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
    D3D_FEATURE_LEVEL FeatureLevel;


	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
    {
        hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
        D3D11_SDK_VERSION, &m_Device, &FeatureLevel, &m_DeviceContext);
        if (SUCCEEDED(hr))
        {
            // Device creation succeeded, no need to loop anymore
            break;
        }
    }
    if (FAILED(hr)) goto Exit;

	//Number of screens
	if (onlyprim)
	{
		ScreenNr=1;
	}
	else
	{
		ScreenNr=SETOFFSETANDDESKTOPSIZES();
		ScreenWidth=DeskBounds.right-DeskBounds.left;
		ScreenHeight=DeskBounds.bottom-DeskBounds.top;
		ScreenOffSetX=DeskBounds.left;
		ScreenOffSetY=DeskBounds.top;
	}

	screenrect= new RECT [ScreenNr];
	OffesetX=new INT[ScreenNr]; 
	OffesetY=new INT[ScreenNr]; 
	ondesktop=new ONEDESKTOP[ScreenNr]; 
	for (int i=0;i<ScreenNr;i++)
	{
		ondesktop[i].SETONEDESKTOP(i,m_Device, m_DeviceContext);
		SETOFFSETANDDESKTOPSIZES(i);
		OffesetX[i]=screenrect[i].left;
		OffesetY[i]=screenrect[i].top;
		if (!ondesktop[i].INITDESK(&list)) goto Exit;
		if (onlyprim)
			{
				ScreenWidth=screenrect[0].right-screenrect[0].left;
				ScreenHeight=screenrect[0].bottom-screenrect[0].top;
				ScreenOffSetX=screenrect[0].left;
				ScreenOffSetY=screenrect[0].top;
			}
	}
	buffer = new unsigned char[ScreenWidth*ScreenHeight * 4];// g_obIPC2.CreateBitmap(ScreenWidth*ScreenHeight * 4);
	if (buffer==NULL) goto Exit;
	LeaveCriticalSection(&CriticalSection);
	return buffer;
Exit:
	LeaveCriticalSection(&CriticalSection);
	return NULL;
}
int DESKTOPMANAGER::CAPTUREDESKTOPMANAGER()
{
	int returnvalue=0;
	if (!buffer) return 0;
	EnterCriticalSection(&CriticalSection); 	
	for (int i=0;i<ScreenNr;i++)
		{
			if (buffer) returnvalue=ondesktop[i].UPDATEDESK(OffesetX[i],OffesetY[i],screenrect[i].right-screenrect[i].left,screenrect[i].bottom-screenrect[i].top,ScreenOffSetX,ScreenOffSetY,ScreenWidth,ScreenHeight,buffer);
		}
	LeaveCriticalSection(&CriticalSection);
	return returnvalue;
}

int DESKTOPMANAGER::STOPDESKTOPMANAGER()
{
	EnterCriticalSection(&CriticalSection);
	int returnvalue=0;
	if (m_DeviceContext)
    {
		m_DeviceContext->ClearState();
        m_DeviceContext->Release();
        m_DeviceContext = nullptr;
    }

    if (m_Device)
    {
        m_Device->Release();
        m_Device = nullptr;
    }

	if (screenrect) delete [] screenrect;
	if (OffesetX) delete [] OffesetX; 
	if (OffesetY) delete [] OffesetY;
	if (ondesktop) delete [] ondesktop;
	screenrect=nullptr;
	OffesetX=nullptr;
	OffesetY=nullptr;	
	if (buffer) delete []buffer;
	buffer = nullptr;
	//g_obIPC2.CloseBitmap();
	LeaveCriticalSection(&CriticalSection);	
	return returnvalue;
}

int DESKTOPMANAGER::SETOFFSETANDDESKTOPSIZES()
{
	 int returnvalue=0;
	HRESULT hr;
	int SingleOutput=-1;
    // Get DXGI resources
    IDXGIDevice* DxgiDevice = nullptr;
    hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr)) return 0;    
    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr)) return 0;  
    // Set initial values so that we always catch the right coordinates
    DeskBounds.left = INT_MAX;
    DeskBounds.right = INT_MIN;
    DeskBounds.top = INT_MAX;
    DeskBounds.bottom = INT_MIN;

    IDXGIOutput* DxgiOutput = nullptr;

    // Figure out right dimensions for full size desktop texture and # of outputs to duplicate
    UINT OutputCount;
    if (SingleOutput < 0)
    {
        hr = S_OK;
        for (OutputCount = 0; SUCCEEDED(hr); ++OutputCount)
        {
            if (DxgiOutput)
            {
                DxgiOutput->Release();
                DxgiOutput = nullptr;
            }
            hr = DxgiAdapter->EnumOutputs(OutputCount, &DxgiOutput);
            if (DxgiOutput && (hr != DXGI_ERROR_NOT_FOUND))
            {
                DXGI_OUTPUT_DESC DesktopDesc;
                DxgiOutput->GetDesc(&DesktopDesc);

                DeskBounds.left = min(DesktopDesc.DesktopCoordinates.left, DeskBounds.left);
                DeskBounds.top = min(DesktopDesc.DesktopCoordinates.top, DeskBounds.top);
                DeskBounds.right = max(DesktopDesc.DesktopCoordinates.right, DeskBounds.right);
                DeskBounds.bottom = max(DesktopDesc.DesktopCoordinates.bottom, DeskBounds.bottom);
            }
        }

        --OutputCount;
    }
    else
    {
        hr = DxgiAdapter->EnumOutputs(SingleOutput, &DxgiOutput);
        if (FAILED(hr))
        {
            DxgiAdapter->Release();
            DxgiAdapter = nullptr;
            return 0;  
        }
        DXGI_OUTPUT_DESC DesktopDesc;
        DxgiOutput->GetDesc(&DesktopDesc);
        DeskBounds = DesktopDesc.DesktopCoordinates;

        DxgiOutput->Release();
        DxgiOutput = nullptr;

        OutputCount = 1;
    }

    DxgiAdapter->Release();
    DxgiAdapter = nullptr;
    return OutputCount;
}

int DESKTOPMANAGER::SETOFFSETANDDESKTOPSIZES(int i)
{
	 int returnvalue=0;
	HRESULT hr;
    // Get DXGI resources
    IDXGIDevice* DxgiDevice = nullptr;
    hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr)) return 0;    
    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr)) return 0;  
    // Set initial values so that we always catch the right coordinates
    screenrect[i].left = INT_MAX;
    screenrect[i].right = INT_MIN;
    screenrect[i].top = INT_MAX;
    screenrect[i].bottom = INT_MIN;

    IDXGIOutput* DxgiOutput = nullptr;

    // Figure out right dimensions for full size desktop texture and # of outputs to duplicate
    UINT OutputCount;
    if (i < 0)
    {
        hr = S_OK;
        for (OutputCount = 0; SUCCEEDED(hr); ++OutputCount)
        {
            if (DxgiOutput)
            {
                DxgiOutput->Release();
                DxgiOutput = nullptr;
            }
            hr = DxgiAdapter->EnumOutputs(OutputCount, &DxgiOutput);
            if (DxgiOutput && (hr != DXGI_ERROR_NOT_FOUND))
            {
                DXGI_OUTPUT_DESC DesktopDesc;
                DxgiOutput->GetDesc(&DesktopDesc);

                screenrect[i].left = min(DesktopDesc.DesktopCoordinates.left,  screenrect[i].left);
                screenrect[i].top = min(DesktopDesc.DesktopCoordinates.top,  screenrect[i].top);
                screenrect[i].right = max(DesktopDesc.DesktopCoordinates.right,  screenrect[i].right);
                screenrect[i].bottom = max(DesktopDesc.DesktopCoordinates.bottom,  screenrect[i].bottom);
            }
        }

        --OutputCount;
    }
    else
    {
        hr = DxgiAdapter->EnumOutputs(i, &DxgiOutput);
        if (FAILED(hr))
        {
            DxgiAdapter->Release();
            DxgiAdapter = nullptr;
            return 0;  
        }
        DXGI_OUTPUT_DESC DesktopDesc;
        DxgiOutput->GetDesc(&DesktopDesc);
        screenrect[i] = DesktopDesc.DesktopCoordinates;

        DxgiOutput->Release();
        DxgiOutput = nullptr;

        OutputCount = 1;
    }

    DxgiAdapter->Release();
    DxgiAdapter = nullptr;
    return OutputCount;
}










ONEDESKTOP::ONEDESKTOP()
{
	DesktopResource = nullptr;
	m_DeskDupl=nullptr;
	m_AcquiredDesktopImage=nullptr;
	m_MetaDataBuffer=nullptr;
	frameaquired = false;
	m_MetaDataSize = 0;
}

void ONEDESKTOP::SETONEDESKTOP(int desknr_IN,ID3D11Device* m_Device_IN,ID3D11DeviceContext* m_DeviceContext_IN)
{	
	m_Device=m_Device_IN;
	m_DeviceContext=m_DeviceContext_IN;
	Desknr=desknr_IN;
}

ONEDESKTOP::~ONEDESKTOP()
{
	if (DesktopResource)
    {
	DesktopResource->Release();
    DesktopResource = nullptr;
	}

	if (m_DeskDupl)
    {
		m_DeskDupl->ReleaseFrame();
        m_DeskDupl->Release();
        m_DeskDupl = nullptr;
    }

    if (m_AcquiredDesktopImage)
    {
        m_AcquiredDesktopImage->Release();
        m_AcquiredDesktopImage = nullptr;
    }

	if (m_MetaDataBuffer)
    {
        delete [] m_MetaDataBuffer;
        m_MetaDataBuffer = nullptr;
    }
}

int ONEDESKTOP::UPDATEDESK(int offsetx,int offsety, int width,int height,int screenoffsetx,int screenoffsety,int screenwidth,int screenheight,unsigned char *buffer)
{
	HRESULT hr=0;
	UINT DirtyCount=0;
	UINT MoveCount=0;	
	IDXGIResource* DesktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	BYTE* DirtyRects=nullptr;
	BYTE* MoveRects=nullptr;
	if (frameaquired)
	{
		hr = m_DeskDupl->ReleaseFrame();
		if (FAILED(hr))
		{
			goto Exit1;
		}
	}

	if (m_AcquiredDesktopImage)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}

    // Get new frame
	frameaquired = false;
    hr = m_DeskDupl->AcquireNextFrame(50, &FrameInfo, &DesktopResource);
    if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		{		
			goto Exit1;
		}

		if (hr == DXGI_ERROR_ACCESS_LOST)
			{
				//◦Desktop switch
				//◦Mode change
				//◦Switch from DWM on, DWM off, or other full-screen application
				if (m_DeskDupl)
				{
					m_DeskDupl->ReleaseFrame();
					m_DeskDupl->Release();
					m_DeskDupl = nullptr;
				}				
			}
		if (m_AcquiredDesktopImage)
			{
				m_AcquiredDesktopImage->Release();
				m_AcquiredDesktopImage = nullptr;
			}
		goto Exit;
	}
	frameaquired = true;

	if (m_AcquiredDesktopImage)
    {
        m_AcquiredDesktopImage->Release();
        m_AcquiredDesktopImage = nullptr;
    }

    // QI for IDXGIResource
	if (DesktopResource==nullptr) goto Exit;
    hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&m_AcquiredDesktopImage));

    if (DesktopResource)
    {
	DesktopResource->Release();
    DesktopResource = nullptr;
	}

    if (FAILED(hr))goto Exit;
	
/// frame content

	if (FrameInfo.TotalMetadataBufferSize)
    {
        // Old buffer too small
        if (FrameInfo.TotalMetadataBufferSize > m_MetaDataSize)
        {
            if (m_MetaDataBuffer)
            {
                delete [] m_MetaDataBuffer;
                m_MetaDataBuffer = nullptr;
            }
            m_MetaDataBuffer = new (std::nothrow) BYTE[FrameInfo.TotalMetadataBufferSize];
            if (!m_MetaDataBuffer)
            {
                m_MetaDataSize = 0;
                MoveCount = 0;
                DirtyCount = 0;
                goto Exit;
            }
            m_MetaDataSize = FrameInfo.TotalMetadataBufferSize;
        }

        UINT BufSize = FrameInfo.TotalMetadataBufferSize;

        // Get move rectangles
        hr = m_DeskDupl->GetFrameMoveRects(BufSize, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(m_MetaDataBuffer), &BufSize);
        if (FAILED(hr))
        {
            MoveCount = 0;
            DirtyCount = 0;
            goto Exit;
        }
        MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
		MoveRects = m_MetaDataBuffer;
        DirtyRects = m_MetaDataBuffer + BufSize;
        BufSize = FrameInfo.TotalMetadataBufferSize - BufSize;

        // Get dirty rectangles
        hr = m_DeskDupl->GetFrameDirtyRects(BufSize, reinterpret_cast<RECT*>(DirtyRects), &BufSize);
        if (FAILED(hr))
        {
            MoveCount = 0;
            DirtyCount = 0;
            goto Exit;
        }
        DirtyCount = BufSize / sizeof(RECT);
    }


	D3D11_TEXTURE2D_DESC Desc;
	m_AcquiredDesktopImage->GetDesc(&Desc);
	D3D11_TEXTURE2D_DESC BitmapBufferDesc;
	ID3D11Texture2D* BitmapBuffer=nullptr;
	BitmapBufferDesc = Desc;
	BitmapBufferDesc.Usage = D3D11_USAGE_STAGING;
	BitmapBufferDesc.BindFlags = 0;
	BitmapBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	BitmapBufferDesc.MiscFlags = 0;
	hr = m_Device->CreateTexture2D(&BitmapBufferDesc, NULL, &BitmapBuffer);
	m_DeviceContext->CopyResource(BitmapBuffer, m_AcquiredDesktopImage);

	IDXGISurface *BitmapSurface=nullptr;
	hr = BitmapBuffer->QueryInterface(__uuidof(IDXGISurface), (void **)&BitmapSurface);
	BitmapBuffer->Release();
    BitmapBuffer = NULL;	 
	if (FAILED(hr)) goto Exit;

	DXGI_MAPPED_RECT MappedSurface;
	hr = BitmapSurface->Map(&MappedSurface, DXGI_MAP_READ);

	if (FrameInfo.TotalMetadataBufferSize)
    {

        if (MoveCount)
        {
              for (UINT i = 0; i < MoveCount; ++i)
				{
					RECT *myrect= (RECT*)MoveRects;
				}
        }

        if (DirtyCount)
        {		
            for (UINT i = 0; i < DirtyCount; ++i)
				{
					RECT *myrect= (RECT*)DirtyRects;
					DirtyRects+=sizeof(RECT);
					//g_obIPC2.Addrect(0, myrect->left+offsetx, myrect->top+offsety,myrect->right+offsetx,myrect->bottom+offsety,0,0,0,0);

					
					int xsource=myrect->left;
					int ysource=myrect->top;

					int xdest=myrect->left+offsetx-screenoffsetx;
					int ydest=myrect->top+offsety-screenoffsety;

					int pitchsource=MappedSurface.Pitch;
					int pitchdest=screenwidth*4;

					int sourceline=MappedSurface.Pitch;
					int destline=screenwidth*4;

					__try
					{


						if (screenwidth > screenheight)
						{
							unsigned char *source = MappedSurface.pBits +
								myrect->top*sourceline + myrect->left * 4;
							unsigned char *dest = buffer + ydest*destline +
								xdest * 4;

							unsigned int offset = 0;
							unsigned int outoffset = 0;

							int rectwidth = myrect->right - myrect->left;
							int rectheight = myrect->bottom - myrect->top;

							//x' = y
							//y' = - x

							for (int i = 0; i < rectheight; i++)
							{
								memcpy(dest + outoffset, source + offset,
									rectwidth * 4);
								offset += pitchsource;
								outoffset += pitchdest;
							}
							Addrect(0, myrect->left + offsetx, myrect->top + offsety, myrect->right + offsetx, myrect->bottom + offsety, 0, 0, 0, 0);
						}
						else
						{
							int rectwidth = myrect->right - myrect->left;
							int rectheight = myrect->bottom - myrect->top;

							for (int j = 0; j < rectheight; j++)
							{
								for (int k = 0; k < rectwidth; k++)
								{

									unsigned char *source =
										MappedSurface.pBits + (myrect->top + j)*sourceline + (myrect->left + k) * 4;
									unsigned char *dest = buffer +
										((myrect->left + k) + offsety - screenoffsety)*destline + (screenwidth
										- myrect->top - j + offsetx - screenoffsetx) * 4;
									memcpy(dest, source, 4);
								}
							}
							Addrect(0, screenwidth - myrect->bottom + offsetx, myrect->left + offsety, screenwidth - myrect->top + offsetx, myrect->right + offsety, 0, 0, 0, 0);
						}
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						BitmapSurface->Unmap();
						BitmapSurface->Release();
						BitmapSurface = NULL;
						goto Exit;
					}
					



					

				}
        }
    }
	BitmapSurface->Unmap();
	BitmapSurface->Release();
    BitmapSurface = NULL;
	if (FAILED(hr)) goto Exit;
   
Exit1:
	//ok or wait
	return 1;
Exit:
	//error
	return 0;
}



int ONEDESKTOP::CLOSEDESK()
{
	if (DesktopResource)
    {
	DesktopResource->Release();
    DesktopResource = nullptr;
	}

	if (m_DeskDupl)
    {
		m_DeskDupl->ReleaseFrame();
        m_DeskDupl->Release();
        m_DeskDupl = nullptr;
    }

    if (m_AcquiredDesktopImage)
    {
        m_AcquiredDesktopImage->Release();
        m_AcquiredDesktopImage = nullptr;
    }

	if (m_MetaDataBuffer)
    {
        delete [] m_MetaDataBuffer;
        m_MetaDataBuffer = nullptr;
    }
	return 0;
}

int ONEDESKTOP::INITDESK(mystruct *plist_IN)
{
	plist = plist_IN;
	HRESULT hr=0;
	DXGI_OUTPUT_DESC m_OutputDesc;

	IDXGIDevice* DxgiDevice = nullptr;
    hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr))  goto Exit;
   

    // Get DXGI adapter
    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr))  goto Exit;
   

    // Get output
    IDXGIOutput* DxgiOutput = nullptr;
    hr = DxgiAdapter->EnumOutputs(Desknr, &DxgiOutput);
    DxgiAdapter->Release();
    DxgiAdapter = nullptr;
    if (FAILED(hr))  goto Exit;
   

    DxgiOutput->GetDesc(&m_OutputDesc);

    // QI for Output 1
    IDXGIOutput1* DxgiOutput1 = nullptr;
    hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
    DxgiOutput->Release();
    DxgiOutput = nullptr;
    if (FAILED(hr))  goto Exit;
   

    // Create desktop duplication
    hr = DxgiOutput1->DuplicateOutput(m_Device, &m_DeskDupl);
    DxgiOutput1->Release();
    DxgiOutput1 = nullptr;
    if (FAILED(hr))  goto Exit;	
	return 1;



Exit:
	CLOSEDESK();
	return 0;


}

void ONEDESKTOP::Addrect(int type, int x1, int y1, int x2, int y2, int x11, int y11, int x22, int y22)
{
	if (plist == NULL) return;
	if (plist->locked == 1)
	{
		return;
	}
	int counter = plist->counter;
	plist->locked = 1;
	counter++;
	if (counter>1999) counter = 1;
	plist->rect1[counter].left = x1;
	plist->rect1[counter].right = x2;
	plist->rect1[counter].top = y1;
	plist->rect1[counter].bottom = y2;
	plist->rect2[counter].left = x11;
	plist->rect2[counter].right = x22;
	plist->rect2[counter].top = y11;
	plist->rect2[counter].bottom = y22;
	plist->type[counter] = type;
	plist->locked = 0;
	plist->counter = counter;
}
#endif