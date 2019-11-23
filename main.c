//  Copyright (C) 2005 SoftKAM. All Rights Reserved.
//
//  AlphaBlend is free software; you can redistribute it and/or modify
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

#include <windows.h>
#include <stdio.h>
#include "main.h"
#include "alpha.h"

#define MAIN_WND_CLASS "FontAppWndClass32"
#define	ITERATIONS		10000

// Global Variables
HINSTANCE	g_hInst;
HWND		g_hMainWnd;
TGA_FILE	g_tga1, g_tga2;
LONG		g_nAlphaFn = 1;
RECT		g_InfoRct;
LONG		g_nFps = 0;
LONG		g_nTicks = 0;
BYTE		*g_buf;

// Foward declarations
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void				TestAlphaBlit(void);

/******************************************************************************/
void FitWindowToPicture(void)
{
	RECT	wndRect;
	POINT	pt;

	wndRect.left = 0;
	wndRect.top = 0;
	wndRect.right = g_tga1.nWidth;
	wndRect.bottom = g_tga1.nHeight;
	AdjustWindowRect(&wndRect, GetWindowLong(g_hMainWnd, GWL_STYLE), FALSE);
	g_InfoRct.left = 0;
	g_InfoRct.top = wndRect.bottom;
	g_InfoRct.right = g_tga1.nWidth;
	g_InfoRct.bottom = wndRect.bottom + 20;
	pt.x =  0; pt.y = 0;
	ClientToScreen(g_hMainWnd, &pt);
	wndRect.bottom += 20;
	OffsetRect(&wndRect, pt.x, pt.y);
	MoveWindow(g_hMainWnd, wndRect.left, wndRect.top,
				wndRect.right - wndRect.left, wndRect.bottom - wndRect.top, TRUE);
}

/******************************************************************************/
void DrawStats(HDC hDC)
{
	char		tmp[64];

	SetBkMode(hDC, OPAQUE);
	if (g_nTicks >= 1000)
	{
		FillRect(hDC, &g_InfoRct, (HBRUSH)GetStockObject(WHITE_BRUSH));
		sprintf(tmp, "FPS: %d", g_nFps);
		DrawText(hDC, tmp, -1, &g_InfoRct, DT_RIGHT);
		g_nFps = 0;
		g_nTicks = 0;
	}
	if (g_nAlphaFn == 1)
	{		
		SetBkMode(hDC, TRANSPARENT);
		DrawText(hDC, "SSE", -1, &g_InfoRct, DT_LEFT);
	}
	else if (g_nAlphaFn == 2)
	{		
		SetBkMode(hDC, TRANSPARENT);
		DrawText(hDC, "MMX", -1, &g_InfoRct, DT_LEFT);
	}
	else
	{		
		SetBkMode(hDC, TRANSPARENT);
		DrawText(hDC, "BASE", -1, &g_InfoRct, DT_LEFT);
	}
}

/******************************************************************************/
void TestBench(int nAlphaFn)
{
	DWORD startTick, totalTicks, i;
	char tmp[256];

	totalTicks = 0;
	startTick = GetTickCount();
	for (i = 0; i < ITERATIONS; i++)
	{
		if (nAlphaFn == 1)
		{
			AlphaBltSSE(g_buf, g_tga2.pBits, g_tga1.nWidth, g_tga1.nHeight);
		}
		else if (nAlphaFn == 2)
		{
			AlphaBltMMX(g_buf, g_tga2.pBits, g_tga1.nWidth, g_tga1.nHeight);
		}
		else
		{
			AlphaBlt(g_buf, g_tga2.pBits, g_tga1.nWidth, g_tga1.nHeight);
		}
	}
	totalTicks += GetTickCount() - startTick;
	sprintf(tmp, "%s Took %d ms (%d iterations)",
		(nAlphaFn == 1) ? "SSE" : (nAlphaFn == 2) ? "MMX" : "BASIC",
		totalTicks, ITERATIONS);
	MessageBox(HWND_DESKTOP, tmp, "AlphaBlend", MB_OK | MB_ICONINFORMATION);
}

int AddByValue(int x, int y)
{
	return x + y;
}

int AddByReference(int* x, int* y)
{
	return *x + *y;
}

inline int SimpleMath()
{
	return 1 + 1;
}

/******************************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG			msg;
	WNDCLASSEX	wcex;
	HDC			hDC;
	LONG		startTick;
//

	int x = 1, y = 2;
	int result = 0;
	result = AddByValue(x, y);
	result = 0;
	result = AddByReference(&x, &y);
	result = 0;
	result = SimpleMath();

	g_hInst = hInstance;
// Load the test images
	if (!LoadTga("test1.tga", &g_tga1) || !LoadTga("test2.tga", &g_tga2))
	{
		MessageBox(NULL, "Error loading test images", "AlphaBlend", MB_OK|MB_ICONERROR);
		return -1;
	}
	g_buf = (BYTE *)malloc(g_tga1.nWidth * g_tga1.nHeight * 4);
// Register main window class
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= MAIN_WND_CLASS;
	wcex.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wcex);
// Create main window
	g_hMainWnd = CreateWindow(MAIN_WND_CLASS, "AlphaBlend", WS_SYSMENU,
							CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, 
							NULL, hInstance, NULL);
	if (!g_hMainWnd)
	{
		return FALSE;
	}
	FitWindowToPicture();
	ShowWindow(g_hMainWnd, nCmdShow);
	UpdateWindow(g_hMainWnd);
	hDC = GetDC(g_hMainWnd);
// Main message loop
	do
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			memcpy(g_buf, g_tga1.pBits, g_tga1.nWidth * g_tga1.nHeight * 4);
			startTick = GetTickCount();
			if (g_nAlphaFn == 1)
			{
				AlphaBltSSE(g_buf, g_tga2.pBits, g_tga1.nWidth, g_tga1.nHeight);
			}
			else if (g_nAlphaFn == 2)
			{
				AlphaBltMMX(g_buf, g_tga2.pBits, g_tga1.nWidth, g_tga1.nHeight);
			}
			else
			{
				AlphaBlt(g_buf, g_tga2.pBits, g_tga1.nWidth, g_tga1.nHeight);
			}
			Blt32(hDC, 0, 0, g_tga1.nWidth, g_tga1.nHeight, g_buf);
			g_nFps ++;
			g_nTicks += GetTickCount() - startTick;
			DrawStats(hDC);
		}
	}
	while (TRUE);
	FreeTga(&g_tga1);
	FreeTga(&g_tga1);
	free(g_buf);
	return msg.wParam;
}

/******************************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_KEYDOWN:
			if (wParam == '1')
			{
				g_nAlphaFn = 1;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			else if (wParam == '2')
			{
				g_nAlphaFn = 2;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			else if (wParam == '3')
			{
				g_nAlphaFn = 3;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			else if (wParam == '4')
			{
				TestBench(1);
			}
			else if (wParam == '5')
			{
				TestBench(2);
			}
			else if (wParam == '6')
			{
				TestBench(3);
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC			hDC;
				hDC = BeginPaint(hWnd, &ps);
				DrawStats(hDC);
				EndPaint(hWnd, &ps);
			}
			break;
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

/******************************************************************************/
void Blt32(	HDC hdc, int x, int y, int width, int height, void *bits)
{
	BITMAPINFO bi;
	BYTE	*p = (BYTE *)bits;

	ZeroMemory(&bi.bmiHeader, sizeof(bi.bmiHeader));
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	SetDIBitsToDevice(hdc, x, y, width, height, 0, 0, 0, height, bits, &bi, DIB_RGB_COLORS);
}
		
/******************************************************************************/
BOOL LoadTga(LPSTR lpFilename, TGA_FILE *pTga)
{
	TGA_HEADER	header;
	DWORD		picture_size, picture_nb, picture_chunk, picture_rest;
	PBYTE		lpByte;
	HGLOBAL		hHandle;
	BYTE		*HBuf;
	OFSTRUCT	of;
	HFILE		fh;
	DWORD		i;

	fh = OpenFile(lpFilename, &of, OF_READ);
	if (fh == HFILE_ERROR)
	{
		return FALSE;
	}
	if (_llseek(fh,0L,0) == -1)
	{
		_lclose(fh);
		return FALSE;
	}
	_lread(fh, (LPSTR)&header, sizeof(TGA_HEADER));
	if (header.type != 2 || header.bpp  != 32)
	{
		_lclose(fh);
		return FALSE;
	}
	picture_size	= (long)header.width * (long)header.height * (long)(header.bpp  / 8);
	picture_chunk	= 0x8000;
	picture_nb		= picture_size / picture_chunk;
	picture_rest	= picture_size % picture_chunk;
	_llseek(fh, (LONG)0x012, 0);
	hHandle = GlobalAlloc(GMEM_MOVEABLE, picture_size);
	if(!hHandle)
	{
		_lclose(fh);
		return FALSE;
	}
	lpByte = (unsigned char*)GlobalLock(hHandle);
	HBuf = lpByte;
	for (i=0; i < picture_nb; i++, HBuf += picture_chunk)
		_lread(fh, HBuf, picture_chunk);
	if (picture_rest != 0)
		_lread(fh, HBuf, picture_rest );
	_lclose(fh);
	pTga->hHandle = hHandle;
	pTga->nHeight = header.height;
	pTga->nWidth = header.width;
	pTga->pBits = lpByte;

	return TRUE;
}

/******************************************************************************/
void FreeTga(TGA_FILE *pTga)
{
	GlobalUnlock(pTga->hHandle);
	GlobalFree(pTga->hHandle);
}
