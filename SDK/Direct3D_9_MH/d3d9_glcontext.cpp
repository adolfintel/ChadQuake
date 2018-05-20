/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/


#ifdef DIRECT3D9_WRAPPER
#include "d3d9_internal.h"

// link to d3d libraries this way so that we don't need to modify the project
#pragma comment (lib, "d3d9.lib")

globals_t d3d_Globals;

#pragma warning(disable:4996) // Baker: strcat

// d3dx crap - because Microsoft played musical chairs with d3dx versioning we can't just statically link and expect everything to work on different PCs,
// so instead we check d3dx dll versions and dynamically load from the best one we can find
HINSTANCE hInstD3DX = NULL;

D3DXLoadSurfaceFromMemoryProc QD3DXLoadSurfaceFromMemory = NULL;
D3DXSaveSurfaceToFileProc QD3DXSaveSurfaceToFile = NULL;
D3DXLoadSurfaceFromSurfaceProc QD3DXLoadSurfaceFromSurface = NULL;
D3DXFilterTextureProc QD3DXFilterTexture = NULL;

void R_UnloadD3DX (void)
{
	// clear the procs
	QD3DXLoadSurfaceFromMemory = NULL;
	QD3DXSaveSurfaceToFile = NULL;
	QD3DXLoadSurfaceFromSurface = NULL;
	QD3DXFilterTexture = NULL;

	// and unload the library
	if (hInstD3DX)
	{
		FreeLibrary (hInstD3DX);
		hInstD3DX = NULL;
	}
}


BOOL R_TryLoadD3DX (char *libname)
{
	// because we don't have d3dx any more we load it this way - yuck!!!
	if ((hInstD3DX = LoadLibrary (libname)) != NULL)
	{
		// now try to load them load them
		if ((QD3DXLoadSurfaceFromMemory = (D3DXLoadSurfaceFromMemoryProc) GetProcAddress (hInstD3DX, "D3DXLoadSurfaceFromMemory")) == NULL) return FALSE;
		if ((QD3DXSaveSurfaceToFile = (D3DXSaveSurfaceToFileProc) GetProcAddress (hInstD3DX, "D3DXSaveSurfaceToFileA")) == NULL) return FALSE;
		if ((QD3DXLoadSurfaceFromSurface = (D3DXLoadSurfaceFromSurfaceProc) GetProcAddress (hInstD3DX, "D3DXLoadSurfaceFromSurface")) == NULL) return FALSE;
		if ((QD3DXFilterTexture = (D3DXFilterTextureProc) GetProcAddress (hInstD3DX, "D3DXFilterTexture")) == NULL) return FALSE;

		// loaded OK
		return TRUE;
	}
	else
	{
		// didn't load at all
		return FALSE;
	}
}


void R_LoadD3DX (void)
{
	int i;

	// ensure that D3DX is unloaded before we begin
	R_UnloadD3DX ();

	// starting at 99 to future-proof things a little, 23 and previous were in static libs and
	// there was no plain old "d3dx9.dll"
	for (i = 99; i > 23; i--)
	{
		// try to load this version
		if (R_TryLoadD3DX (va ("d3dx9_%d.dll", i))) return;

		// unload if it didn't
		R_UnloadD3DX ();
	}

	// not sure if this even exists with some versions....
	if (R_TryLoadD3DX ("d3dx9.dll")) return;

	// the HINSTANCE for the library should be valid if it loaded or NULL if it didn't
	System_Error ("R_LoadD3DX : failed to load D3DX\nPlease update your installation of DirectX...");
}


globals_t::globals_t (void)
{
	this->NumModeList = 0;
	this->RequestStencil = FALSE;
	this->Object = NULL;
}


void globals_t::CreateDirect3D (void)
{
	if (!this->Object)
	{
		if (!(this->Object = Direct3DCreate9 (D3D_SDK_VERSION)))
			System_Error ("Failed to create Direct3D Object");

		if (FAILED (this->Object->GetDeviceCaps (D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &this->DeviceCaps)))
			System_Error ("failed to get object caps");

		// get the format for the desktop mode
		if (FAILED (this->Object->GetAdapterDisplayMode (D3DADAPTER_DEFAULT, &this->DesktopMode)))
			System_Error ("Failed to get desktop mode");

		// now attempt to load d3dx; these pointers will hold good across mode changes so once only is sufficient
		R_LoadD3DX ();
	}
}


void globals_t::D3DModeToDEVMODE (LPDEVMODE lpDevMode, D3DDISPLAYMODE *mode)
{
	lpDevMode->dmPelsWidth = mode->Width;
	lpDevMode->dmPelsHeight = mode->Height;
	lpDevMode->dmDisplayFrequency = mode->RefreshRate;
	lpDevMode->dmDisplayFlags = 0;

	switch (mode->Format)
	{
	case D3DFMT_R8G8B8:   lpDevMode->dmBitsPerPel = 32; break;
	case D3DFMT_X8R8G8B8: lpDevMode->dmBitsPerPel = 32; break;
	case D3DFMT_A8R8G8B8: lpDevMode->dmBitsPerPel = 32; break;
	case D3DFMT_A8B8G8R8: lpDevMode->dmBitsPerPel = 32; break;
	case D3DFMT_X8B8G8R8: lpDevMode->dmBitsPerPel = 32; break;
	case D3DFMT_R5G6B5:   lpDevMode->dmBitsPerPel = 16; break;
	case D3DFMT_X1R5G5B5: lpDevMode->dmBitsPerPel = 16; break;
	case D3DFMT_A1R5G5B5: lpDevMode->dmBitsPerPel = 16; break;
	case D3DFMT_A4R4G4B4: lpDevMode->dmBitsPerPel = 16; break;
	case D3DFMT_A8R3G3B2: lpDevMode->dmBitsPerPel = 16; break;
	case D3DFMT_X4R4G4B4: lpDevMode->dmBitsPerPel = 16; break;
	default: lpDevMode->dmBitsPerPel = 0; break;
	}
}


void globals_t::GetModeList (void)
{
	this->NumModeList = 0;

	// get and validate the number of modes for this format; we expect that this will succeed first time
	UINT modecount = this->Object->GetAdapterModeCount (D3DADAPTER_DEFAULT, this->DesktopMode.Format);

	if (!modecount) return;

	// check each mode in turn to find a match
	for (UINT m = 0; m < modecount; m++)
	{
		// get this mode
		if (FAILED (this->Object->EnumAdapterModes (D3DADAPTER_DEFAULT, this->DesktopMode.Format, m, &this->ModeList[this->NumModeList]))) continue;

		// ensure that the texture formats we want to create exist
		if (!D3D_CheckTextureFormat (D3DFMT_L8, this->DesktopMode.Format)) continue;
		if (!D3D_CheckTextureFormat (D3DFMT_X8R8G8B8, this->DesktopMode.Format)) continue;
		if (!D3D_CheckTextureFormat (D3DFMT_A8R8G8B8, this->DesktopMode.Format)) continue;

		// don't take lower
		if (this->ModeList[this->NumModeList].Width < MIN_MODE_WIDTH_640) continue;
		if (this->ModeList[this->NumModeList].Height < MIN_MODE_HEIGHT_400) continue;

		// check for weird rotated screen modes on laptops
		if (this->ModeList[this->NumModeList].Height > this->ModeList[this->NumModeList].Width) continue;

		this->NumModeList++;
	}
}


void context_t::InitializeStates (void)
{
	// clear
	this->State.Clear.Color = 0x00000000;
	this->State.Clear.Depth = 1.0f;
	this->State.Clear.Stencil = 0;
}


D3DPRESENT_PARAMETERS *context_t::SetupPresentParams (int width, int height, BOOL windowed, BOOL vsync)
{
	static D3DPRESENT_PARAMETERS PresentParams;

	// clear present params to NULL
	memset (&PresentParams, 0, sizeof (D3DPRESENT_PARAMETERS));

	// fill in what's different between fullscreen and windowed modes
	if (windowed)
	{
		PresentParams.BackBufferFormat = D3DFMT_UNKNOWN;
		PresentParams.FullScreen_RefreshRateInHz = 0;
		PresentParams.Windowed = TRUE;
	}
	else
	{
		PresentParams.BackBufferFormat = d3d_Globals.DesktopMode.Format;
		PresentParams.FullScreen_RefreshRateInHz = d3d_Globals.DesktopMode.RefreshRate;
		PresentParams.Windowed = FALSE;
	}

	// request 1 backbuffer
	PresentParams.BackBufferCount = 1;
	PresentParams.BackBufferWidth = this->DisplayMode.Width = width;
	PresentParams.BackBufferHeight = this->DisplayMode.Height = height;

	PresentParams.hDeviceWindow = this->Window;

	PresentParams.EnableAutoDepthStencil = TRUE;

	if (d3d_Globals.RequestStencil)
		PresentParams.AutoDepthStencilFormat = D3DFMT_D24S8;
	else PresentParams.AutoDepthStencilFormat = D3DFMT_D24X8;

	this->DisplayMode.dsFmt = PresentParams.AutoDepthStencilFormat;

	if (vsync)
		PresentParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	else PresentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	PresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;

	// store these out so that we don't need to touch the D3DPRESENT_PARAMETERS struct
	this->DisplayMode.Windowed = windowed;
	this->DisplayMode.VSync = vsync;

	return &PresentParams;
}


void context_t::UpdateDisplayMode (void)
{
	// adds a bunch of checking for various conditions relating to display mode and backbuffer clamping
	IDirect3DSwapChain9 *sc = NULL;
	D3DPRESENT_PARAMETERS scpp;

	if (SUCCEEDED (this->Device->GetSwapChain (0, &sc)))
	{
		sc->GetPresentParameters (&scpp);

		this->DisplayMode.Width = scpp.BackBufferWidth;
		this->DisplayMode.Height = scpp.BackBufferHeight;
		this->DisplayMode.dsFmt = scpp.AutoDepthStencilFormat;
		this->DisplayMode.VSync = (scpp.PresentationInterval == D3DPRESENT_INTERVAL_ONE);
		this->DisplayMode.Windowed = scpp.Windowed;

		sc->Release ();
	}
}


context_t::context_t (HDC hdc)
{
	// Baker didn't like this work being done in wglMakeCurrent so i moved it to wglCreateContext, which i guess it
	// was always appropriate for anyway.  in *theory* this would mean that the wrapper could be extended to support
	// multiple contexts if all the global state stuff/etc was pulled out into it's own context struct.

	// the object must be initialized one-time-only
	d3d_Globals.CreateDirect3D ();

	RECT clientrect;
	LONG winstyle;
	RECT workarea;

	// we can't extern mainwindow as it may be called something else depending on the codebase used
	if ((this->Window = WindowFromDC (hdc)) == NULL) System_Error ("Direct3D9_wglCreateContext: could not determine application window");

	// get the dimensions of the window
	GetClientRect (this->Window, &clientrect);

	// see are we fullscreen
	winstyle = GetWindowLong (this->Window, GWL_STYLE);

	// still unsure of the best way to handle this, or even if it should be handled in the wrapper
#if 0
	// validate window dimensions for windowed modes to prevent setting modes >= the desktop work area;
	// http://www.celephais.net/board/view_thread.php?id=61375&start=860
	if (!(winstyle & WS_POPUP))
	{
		if (SystemParametersInfo (SPI_GETWORKAREA, 0, &workarea, 0))
		{
			RECT windowrect;
			CopyRect (&windowrect, &clientrect);
			AdjustWindowRect (&windowrect, winstyle, FALSE);

			// width
			if (windowrect.right - windowrect.left >= workarea.right - workarea.left)
				System_Error ("context_t::context_t : window width is too large");

			// height
			if (windowrect.bottom - windowrect.top >= workarea.bottom - workarea.top)
				System_Error ("context_t::context_t : window height is too large");
		}
	}
#endif

	// the window is always created in a windowed mode and may then be switched to fullscreen after
	// this is conformant with DXGI requirements on Vista+ and a lot of other annoying crap just goes away
	// create with no vsync by default and we'll switch the mode when the cvars come online
	D3DPRESENT_PARAMETERS *PresentParams = this->SetupPresentParams (clientrect.right, clientrect.bottom, TRUE, FALSE);

	// --------------------------------------------------------------------------------------------------------
	// here we use D3DCREATE_FPU_PRESERVE to maintain the resolution of Quake's timers (this is a serious problem)
	// and D3DCREATE_DISABLE_DRIVER_MANAGEMENT to protect us from rogue drivers (call it honest paranoia).  first
	// we attempt to create a hardware vp device.
	// --------------------------------------------------------------------------------------------------------
	// NOTE re pure devices: we intentionally DON'T request a pure device, EVEN if one is available, as we need
	// to support certain glGet functions that would be broken if we used one.
	// --------------------------------------------------------------------------------------------------------
	// NOTE re D3DCREATE_FPU_PRESERVE - this can be avoided if we use a timer that's not subject to FPU drift,
	// such as timeGetTime (with timeBeginTime (1)); by default Quake's times *ARE* prone to FPU drift as they
	// use doubles for storing the last time, which gradually creeps up to be nearer to the current time each
	// frame.  Not using doubles for the stored times (i.e. switching them all to floats) would also help here.
	if (SUCCEEDED (d3d_Globals.Object->CreateDevice (
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		this->Window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
		PresentParams,
		&this->Device
	)))
	{
		// attempt to create our vertex shader
		if (!this->CreateCommonVertexShader ())
		{
			// couldn't create it with a hardware VP device so destroy everything and try to create with software VP
			this->DestroyVertexShader ();
			SAFE_RELEASE (this->Device);
		}
	}

	if (!this->Device)
	{
		// it's OK, we may not have hardware vp available, so create a software vp device
		if (FAILED (d3d_Globals.Object->CreateDevice (
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			this->Window,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
			PresentParams,
			&this->Device
		))) System_Error ("failed to create Direct3D device");

		if (!this->CreateCommonVertexShader ()) System_Error ("failed to create Direct3D vertex shader with software VP device");
	}

	if (this->Device == NULL) System_Error ("created NULL Direct3D device");

	// check if a FS mode and switch to the mode as soon as possible
	if (winstyle & WS_POPUP)
	{
		// and reset; there's no need for a full release/recreate because no objects have been created yet
		this->Device->Reset (this->SetupPresentParams (this->DisplayMode.Width, this->DisplayMode.Height, FALSE, this->DisplayMode.VSync));
		this->UpdateDisplayMode ();
	}

	// let's always have a valid initial viewport
	this->ResetViewport ();
	this->UpdateViewport ();

	// build extensions string
	this->GLExtensions[0] = 0;

	// D3D always has these
	strcat (this->GLExtensions, "GL_EXT_swap_control ");
	strcat (this->GLExtensions, "GL_ARB_texture_storage ");

	// conditional NPO2 support is equivalent to GL_ARB_texture_rectangle which we don't want to support because in GL it uses unnormalized texcoords.
	if (!(d3d_Globals.DeviceCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) && !(d3d_Globals.DeviceCaps.TextureCaps & D3DPTEXTURECAPS_POW2))
	{
		// full unconditional non-power-of-two support
		strcat (this->GLExtensions, "GL_ARB_texture_non_power_of_two ");
	}

	// multitexture
	if (d3d_Globals.DeviceCaps.MaxSimultaneousTextures > 1 && d3d_Globals.DeviceCaps.MaxTextureBlendStages > 1)
		strcat (this->GLExtensions, "GL_ARB_multitexture ");

	// anisotropic filtering
	if (d3d_Globals.DeviceCaps.MaxAnisotropy > 1)
		strcat (this->GLExtensions, "GL_EXT_texture_filter_anisotropic ");

	// combine and add - because d3d caps are MUCH more granular than a single monolithic GL extension, we just
	// expose these 2 extensions on Ps 2.0 or better hardware
	if (D3DSHADER_VERSION_MAJOR (d3d_Globals.DeviceCaps.PixelShaderVersion) >= 2)
	{
		strcat (this->GLExtensions, "GL_ARB_texture_env_add ");
		strcat (this->GLExtensions, "GL_ARB_texture_env_combine ");
	}

	// get adapter ID for glGetString (renderer/vendor) stuff
	d3d_Globals.Object->GetAdapterIdentifier (0, 0, &this->AdapterID);

	this->DeviceLost = FALSE;
	this->ClientActiveTexture = 0;

	// initialize textures
	this->InitTextures ();

	// initialize state
	this->InitStates ();

	// disable lighting
	this->SetRenderState (D3DRS_LIGHTING, FALSE);

	// set projection and world to dirty, beginning of stack and identity
	this->InitializeTransforms ();

	this->InitGeometry ();

	this->InitGammaAndContrast ();

	// states
	this->InitializeStates ();

	// clear the color buffer on creation
	this->Clear (D3DCLEAR_TARGET);
}


void context_t::PreReset (void)
{
	this->DestroyGammaAndContrast ();
	this->ReleaseDefaultPoolTextures ();
}


void context_t::PostReset (void)
{
	this->InitGammaAndContrast ();

	this->InitializeStates ();

	this->RecreateDefaultPoolTextures ();

	// set projection and world to dirty, beginning of stack and identity
	this->InitializeTransforms ();

	// force all states back to the way they were
	this->SetRenderStates ();
	this->SetTextureStates ();

	this->InitGeometry ();
}

// Baker: Added this functon to center the window on the desktop.
// The desktop size is probably in the wrapper somewhere, but I wasn't sure
// which one was persistent.

static void WIN_AdjustRectToCenterScreen (RECT *in_windowrect, int desktop_width, int desktop_height)
{
	int nwidth  = in_windowrect->right - in_windowrect->left;	// But the window positioning is apparently affected by DPI
	int nheight = in_windowrect->bottom - in_windowrect->top;

	in_windowrect->left = 0 + (desktop_width - nwidth) / 2;
	in_windowrect->top =  0 + (desktop_height - nheight) / 2;
	in_windowrect->right = in_windowrect->left + nwidth;
	in_windowrect->bottom = in_windowrect->top + nheight;
}

// Baker: Ok here are scenarios ...
// 1) New windowed mode -- center window on the desktop.  We receive desktop width/height.  We respond back with the border size we calculated so WM_GETMINMAXINFO can accurately restrict resizing.
// 2) Resize in-progress -- we only receive a client window area x, y -- which is the top left of the canvas, not the window.  We need to subtract out the border size to calculate the corect Window X, Y position
//    It could be improved or streamlined more, but due to time constraints need to roll with this for now ...
// 3) Full window.  No changes.

void context_t::ResetMode (int width, int height, BOOL windowed, int client_left, int client_top, int desktop_width, int desktop_height, int is_resize, int *pborder_width, int *pborder_height)
{
	BOOL wasWindowed = this->DisplayMode.Windowed;

	// reset window styles
	if (!windowed && wasWindowed)
	{
		SetWindowLong (this->Window, GWL_STYLE, WS_POPUP); // Fullscreen goes to 0,0
		SetWindowPos (this->Window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	// reset present params and reset the device
	this->ResetDevice (this->SetupPresentParams (width, height, windowed, this->DisplayMode.VSync));

	if (windowed)
	{
		RECT winrect;

		// MH:  NOTE - DON'T send the styles from the engine, this isn't OpenGL.
		// Baker: I made WinQuake have a resizable window and copied those window
		//        borders styles to the engine ... so I bent the engine towards this to solve ;-)

#if 1 // D3D_RESIZABLE_WINDOW
		LONG winstyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_VISIBLE;
#else
		LONG winstyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX;
#endif
		// Baker: I made this always happen.
		if (1) // (!wasWindowed)
		{
			SetWindowLong (this->Window, GWL_STYLE, winstyle);
			SetWindowPos (this->Window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);			
		}
		// Remove window topmost.  Seems to be required to be separate call.
		SetWindowPos (this->Window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

		SetRect (&winrect, 0, 0, width, height);
		RECT client_rect = winrect; // Copy it off so we know border left, top, right, bottom

		AdjustWindowRectEx (&winrect, winstyle, FALSE, 0); // Add the borders to the rect

		int border_left = -winrect.left;	// Left border amount calculation
		int border_top = -winrect.top;		// Top border amount calculation

		int border_width = (winrect.right - winrect.left) - client_rect.right;
		int border_height = (winrect.bottom - winrect.top) - client_rect.bottom;
		
		// Baker: positioning
		// A) Mode change window = center on desktop
		// B) Resize = Use top left.  We only receive client canvas top left ...
		//		so we must subtract out border left and border top
		
		#define RECT_WIDTH(_rect)	(_rect.right - _rect.left)
		#define RECT_HEIGHT(_rect)  (_rect.bottom - _rect.top)
		switch (is_resize) {
		
		default:		// Resize.  We want the left and the top.
						// We are provided the client left and the client top
						winrect.left += client_left, winrect.right += client_left;
						winrect.top += client_top, winrect.bottom += client_top;

						// If resize, we need to tell the engine the border width and height so that 
						// WM_GETMINMAXINFO can add the border size in.

						*pborder_width  = (winrect.right - winrect.left) - client_rect.right;
						*pborder_height = (winrect.bottom - winrect.top) - client_rect.bottom;
						
						break;

		// Not resize --- that means setmode
		case false:		WIN_AdjustRectToCenterScreen (&winrect, desktop_width, desktop_height);
		
		}
		
		// Baker: Just for clarity
		int x = winrect.left, y = winrect.top;
		int w = RECT_WIDTH(winrect), h = RECT_HEIGHT(winrect);

		
		#if 0 // Baker: Debug purposes if needed
			Con_SafePrintLinef ("Winrect RB %d, %d --> %d x %d --> %d, %d", winrect.left, winrect.top, winrect.right, winrect.bottom, w, h);
			Con_SafePrintLinef ("CL Rect  %d %d %d %d %d %d", client_rect.left, client_rect.top, client_rect.right, client_rect.bottom, RECT_WIDTH(client_rect), RECT_HEIGHT(client_rect) );
			Con_SafePrintLinef ("Width %d Height %d --> w %d, h %d", width, height, w, h);					
			Con_SafePrintLinef ("Border width %d x Height %d --> w %d, h %d", border_width, border_height, w - border_width, border_height - h);
			Con_SafePrintLinef ("Border left size %d x Border top size %d", border_left, border_top);//, w - border_width, border_height - h);
		#endif // 0

		// Baker:  Now use the calculations for positioning.
		MoveWindow (this->Window, x, y, w, h, TRUE /* please paint the borders */);
		
		// Originally ... MoveWindow (this->Window, 0, 0, winrect.right - winrect.left, winrect.bottom - winrect.top, FALSE);
	}
}


void context_t::BeginScene (void)
{
	// check for a beginscene
	if (!this->State.SceneBegun)
	{
		// issue a beginscene (geometry needs this
		this->Device->BeginScene ();

		// bind our main d3d objects
		this->Geometry.Activate ();
		this->ActivateVertexShader ();

		// clear down bound textures
		for (int i = 0; i < D3D_MAX_TMUS; i++)
			this->State.BoundTextures[i] = NULL;

		this->State.DrawCount = 0;

		// we're in a scene now
		this->State.SceneBegun = TRUE;
	}
}


void context_t::EndScene (void)
{
	// if we had lost the device (e.g. on a mode switch, alt-tab, etc) we must try to recover it
	if (this->DeviceLost)
	{
		// here we get the current status of the device
		HRESULT hr = this->Device->TestCooperativeLevel ();

		switch (hr)
		{
		case D3D_OK:
			// device is recovered
			this->DeviceLost = FALSE;
			this->UpdateDisplayMode ();
			this->PostReset ();
			break;

		case D3DERR_DEVICELOST:
			// device is still lost
			Sleep (1);
			break;

		case D3DERR_DEVICENOTRESET:
			// device is ready to be reset
			this->PreReset ();
			if (FAILED (this->Device->Reset (this->SetupPresentParams (this->DisplayMode.Width, this->DisplayMode.Height, this->DisplayMode.Windowed, this->DisplayMode.VSync))))
				System_Error ("device reset failed");
			break;

		default:
			break;
		}

		// yield the CPU a little; i do this because a lost device typically happens when the window is minimized or alt-tabbed
		// away from, so there is no work for d3d to be doing anyway
		Sleep (10);

		// don't bother this frame
		return;
	}

	if (this->State.SceneBegun)
	{
		HRESULT hr;

		this->FlushGeometry ();

		/*
		if (this->State.DrawCount)
		{
			Con_PrintLinef ("%d draw call\n", this->State.DrawCount);
			this->State.DrawCount = 0;
		}
		*/

		// finalize gamma and contrast adjustment
		this->FinishGammaAndContrast ();

		// endscene and present are only required if a scene was begun (i.e. if something was actually drawn)
		this->Device->EndScene ();
		this->State.SceneBegun = FALSE;

		// present the display
		hr = this->Device->Present (NULL, NULL, NULL, NULL);

		if (hr == D3DERR_DEVICELOST)
		{
			// flag a lost device
			this->DeviceLost = TRUE;
		}
		else if (FAILED (hr))
		{
			// something else bad happened
			System_Error ("FAILED (hr) on this->Device->Present");
		}
	}
}


void context_t::ResetDevice (D3DPRESENT_PARAMETERS *PresentParams)
{
	this->FlushGeometry ();
	this->PreReset ();

	while (this->Device->TestCooperativeLevel () != D3D_OK)
		Sleep (1);

	// reset device
	if (FAILED (this->Device->Reset (PresentParams)))
		System_Error ("context_t::ResetDevice : failed");

	while (this->Device->TestCooperativeLevel () != D3D_OK)
		Sleep (1);

	// clean up states/etc
	this->UpdateDisplayMode ();
	this->PostReset ();
}


void context_t::SetVSync (int interval)
{
	// doing it this way because interval can be values other than 0 or 1
	this->ResetDevice (this->SetupPresentParams (this->DisplayMode.Width, this->DisplayMode.Height, this->DisplayMode.Windowed, interval ? TRUE : FALSE));
}


void context_t::Sync (void)
{
	IDirect3DQuery9 *FinishEvent = NULL;
	
	if (SUCCEEDED (this->Device->CreateQuery (D3DQUERYTYPE_EVENT, &FinishEvent)))
	{
		this->FlushGeometry ();

		if (SUCCEEDED (FinishEvent->Issue (D3DISSUE_END)))
			while (FinishEvent->GetData (NULL, 0, D3DGETDATA_FLUSH) == S_FALSE);

		SAFE_RELEASE (FinishEvent);
	}
}


void context_t::Release (void)
{
	// release our d3d objects
	this->DestroyGammaAndContrast ();
	this->DestroyVertexShader ();
	this->ReleaseTextures ();

	if (this->Device)
	{
		// switch to windowed before destroying the device; this is for conformance with DXGI requirements on Vista+
		this->Device->Reset (this->SetupPresentParams (this->DisplayMode.Width, this->DisplayMode.Height, TRUE, this->DisplayMode.VSync));
		this->UpdateDisplayMode ();
	}

	// now destroy the device
	SAFE_RELEASE (this->Device);
}


#endif // DIRECT3D9_WRAPPER

