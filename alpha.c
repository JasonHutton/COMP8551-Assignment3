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

#include "alpha.h"

/******************************************************************************/
void AlphaBlt(unsigned char *dst, unsigned char *src, int w, int h)
{
	int i;
	int srcb, srcg, srcr, srca;
	int dstb, dstg, dstr, dsta;

	for (i=0; i < w * h * 4; i += 4)
	{
		srcb = src[i + 0];
		srcg = src[i + 1];
		srcr = src[i + 2];
		srca = src[i + 3];
		dstb = dst[i + 0];
		dstg = dst[i + 1];
		dstr = dst[i + 2];
		dsta = dst[i + 3];
		dstb = (srca * (srcb - dstb) + dstb * 256) / 256;
		dstg = (srca * (srcg - dstg) + dstg * 256) / 256;
		dstr = (srca * (srcr - dstr) + dstr * 256) / 256;
		dsta = srca;
		dst[i + 0] = dstb;
		dst[i + 1] = dstg;
		dst[i + 2] = dstr;
		dst[i + 3] = dsta;
	}
}

// Intel will have the actual manual for these instructions, but Oracle seems to have a decent quick reference:
// https://docs.oracle.com/cd/E26502_01/html/E28388/eojde.html
/******************************************************************************/
void AlphaBltSSE(unsigned char *dst, unsigned char *src, int w, int h)
{
	int	wmul4 = w << 2;					// Shift bits of w 2 to the left, and assign that to wmul4. This multiplies w by 4 and assigns to wmul4.

	if (w==0) return;                   // Return if image width is 0, meaning no image is loaded.
	w >>= 1;							// Shift bits of w 1 to the right. This results in a division of w by 2, assigned back to itself.
	_asm {
// For each pixel: dst = (src_alpha * (src-dst)  + dst * 256) / 256
		mov			edi,dst				// Move the address of index 0 of dst (unsigned char*) into EDI destination index register, for string operations
		mov			esi,src				// Move the address of index 0 of src (unsigned char*) into ESI source index register, for string operations
		mov			edx,h				// Move the address of h (int, 35 bits(4 bytes)) into 32-bit(4 byte) EDX register.
		pxor		mm6,mm6				// Performs a logical XOR operation on mm6 and mm6, then stores the result in mm6, a 64 bit MMX register. Because XORing itself, this initializes to 0.
		pxor		mm7,mm7				// Performs a logical XOR operation on mm7 and mm7, then stores the result in mm7, a 64 bit MMX register. Because XORing itself, this initializes to 0.
		xor			eax,eax				// Performs a logical XOR operation on eax and eax registers, then stores the result in eax, a 32 bit x86 register. Because XORing itself, this initializes to 0.
scan_loop:								// Label this line to be jumped to.
		mov			ecx,w				// Move address of w into ECX 32 bit(4 byte) register.
		xor			ebx,ebx				// Perform a logical XOR operation on EBX and EBX registers, then stores the result in EBX, a 32 bit x86 register. Because XORing itself, this initializes to 0.
pix_loop:								// Label this line to be jumped to.
		movq		mm4,[esi+ebx*8]		// mm0 = src (RG BA RG BA) // Moves a quadword(8 bytes) of data from address of esi+ebx*8 to mm4. ESI is src's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
		movq		mm5,[edi+ebx*8]		// mm1 = dst (RG BA RG BA) // Moves a quadword(8 bytes) of data from address of edi+ebx*8 to mm5. ESD is dst's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
// FIRST PIXEL
		movq		mm0,mm4				// mm0 = 00 00 RG BA
		movq		mm1,mm5				// mm1 = 00 00 RG BA
		punpcklbw	mm0,mm6				// mm0 = (0R 0G 0B 0A)
		punpcklbw	mm1,mm7				// mm0 = (0R 0G 0B 0A)
		pshufw		mm2,mm0,0ffh		// mm2 = 0A 0A 0A 0A
		movq		mm3,mm1				// mm3 = mm1
		psubw		mm0,mm1				// mm0 = mm0 - mm1
		psllw		mm3,8				// mm3 = mm1 * 256
		pmullw		mm0,mm2				// mm0 = (src-dst)*alpha
		paddw		mm0,mm3				// mm0 = (src-dst)*alpha+dst*256
		psrlw		mm0,8				// mm0 = ((src - dst) * alpha + dst * 256) / 256
// SECOND PIXEL
		punpckhbw	mm5,mm7				// mm5 = (0R 0G 0B 0A)
		punpckhbw	mm4,mm6				// mm4 = (0R 0G 0B 0A)
		movq		mm3,mm5				// mm3 = mm5
		pshufw		mm2,mm4,0ffh		// mm2 = 0A 0A 0A 0A
		psllw		mm3,8				// mm3 = mm5 * 256
		psubw		mm4,mm5				// mm4 = mm4 - mm5
		pmullw		mm4,mm2				// mm4 = (src-dst)*alpha
		paddw		mm4,mm3				// mm4 = (src-dst)*alpha+dst*256
		psrlw		mm4,8				// mm4 = ((src - dst) * alpha + dst * 256) / 256
		packuswb	mm0,mm4				// mm0 = RG BA RG BA
		movq		[edi+ebx*8],mm0		// dst = mm0
		inc			ebx
		loop		pix_loop
//
		mov			ebx, wmul4
		add			esi, ebx
		add			edi, ebx
		dec			edx
		jnz			scan_loop
	}
}

// Intel will have the actual manual for these instructions, but Oracle seems to have a decent quick reference:
// https://docs.oracle.com/cd/E18752_01/html/817-5477/eojdc.html
/******************************************************************************/
void AlphaBltMMX(unsigned char *dst, unsigned char *src, int w, int h)
{
	int	wmul4 = w << 2;

	if (w==0) return;
	w >>= 1;
	_asm {
// For each pixel: dst = (src_alpha * (src-dst)  + dst * 256) / 256
		mov			edi,dst
		mov			esi,src
		mov			edx,h
		pxor		mm6,mm6
		pxor		mm7,mm7
		xor			eax,eax
scan_loop:
		mov			ecx,w
		xor			ebx,ebx
pix_loop:
		movq		mm4,[esi+ebx*8]		// mm4 = src (RG BA RG BA)
		movq		mm5,[edi+ebx*8]		// mm5 = dst (RG BA RG BA)
// FIRST PIXEL
		movq		mm0,mm4				// mm0 = src (-- -- RG BA)
		movq		mm1,mm5				// mm1 = dst (-- -- RG BA)
		punpcklbw	mm0,mm6				// mm0 = (0R 0G 0B 0A)
		mov			al,[esi+ebx*8+3]	// eax = pixel alpha (0 - 255)
		punpcklbw	mm1,mm7				// mm1 = (0R 0G 0B 0A)
		movd		mm2,eax				// 00 00 00 0A
		movq		mm3,mm1				// mm3 = mm1: dst (0R 0G 0B 0A)
		punpcklwd	mm2,mm2				// 00 00 0A 0A
		psubw		mm0,mm1				// mm0 = mm0 - mm1
		punpckldq	mm2,mm2				// 0A 0A 0A 0A
		psllw		mm3,8				// mm3 = mm1 * 256
		pmullw		mm0,mm2				// mm0 = (src - dst) * alpha
		paddw		mm0,mm3				// mm0 = (src - dst) * alpha + dst * 256
		psrlw		mm0,8				// mm0 = ((src - dst) * alpha + dst * 256) / 256
		packuswb	mm0,mm6				// mm0 = RGBA
// SECOND PIXEL
		punpckhbw	mm4,mm6				// mm4 = (0R 0G 0B 0A)
		mov			al,[esi+ebx*8+7]	// eax = pixel alpha (0 - 255)
		punpckhbw	mm5,mm7				// mm5 = (0R 0G 0B 0A)
		movd		mm2,eax				// 00 00 00 0A
		movq		mm3,mm5				// mm3 = mm5: dst (0R 0G 0B 0A)
		punpcklwd	mm2,mm2				// 00 00 0A 0A
		psubw		mm4,mm5				// mm4 = mm4 - mm5
		punpckldq	mm2,mm2				// 0A 0A 0A 0A
		psllw		mm3,8				// mm3 = mm5 * 256
		pmullw		mm4,mm2				// mm4 = (src - dst) * alpha
		paddw		mm4,mm3				// mm4 = (src - dst) * alpha + dst * 256
		psrlw		mm4,8				// mm4 = ((src - dst) * alpha + dst * 256) / 256
		packuswb	mm4,mm6				// mm4 = RGBA
		punpckldq	mm0,mm4				// mm0 = RG BA RG BA
		movq		[edi+ebx*8],mm0		// dst = mm0
		inc			ebx
// REPEAT
		loop		pix_loop
		mov			ebx, wmul4
		add			esi, ebx
		add			edi, ebx
		dec			edx
		jnz			scan_loop
	}
}

