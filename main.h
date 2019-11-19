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

#ifndef _MAIN_H_INCLUDED
#define _MAIN_H_INCLUDED

#pragma pack(push, 1)

typedef struct
{
	BYTE idlen;
	BYTE map;
	BYTE type;
	BYTE b0, b1, b2, b3, b4;
	WORD xorg;
	WORD yorg;
	WORD width;
	WORD height;
	BYTE bpp;
	BYTE b5;
} TGA_HEADER;

typedef struct
{
	int		nWidth;
	int		nHeight;
	BYTE 	*pBits;
	HANDLE	hHandle;
} TGA_FILE;


#pragma pack(pop)

BOOL LoadTga(LPSTR lpFilename, TGA_FILE *pTga);
void FreeTga(TGA_FILE *pTga);
void Blt32(	HDC hdc, int x, int y, int width, int height, void *bits);

#endif // _MAIN_H_INCLUDED
