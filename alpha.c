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
		mov			edi,dst				// Move the address of index 0 of dst (unsigned char*(Because this is a pointer, it's an int, being compiled as x86, so 32 bits, or 4 bytes)) into EDI destination index register, for string operations
		mov			esi,src				// Move the address of index 0 of src (unsigned char*(Because this is a pointer, it's an int, being compiled as x86, so 32 bits, or 4 bytes)) into ESI source index register, for string operations
		mov			edx,h				// Move the address of h (int, 32 bits(4 bytes)) into 32-bit(4 byte) EDX register. EDX now points to h.
		pxor		mm6,mm6				// Performs a logical XOR operation on mm6 and mm6, then stores the result in mm6, a 64 bit MMX register. Because XORing itself, this initializes to 0.
		pxor		mm7,mm7				// Performs a logical XOR operation on mm7 and mm7, then stores the result in mm7, a 64 bit MMX register. Because XORing itself, this initializes to 0.
		xor			eax,eax				// Performs a logical XOR operation on eax and eax, then stores the result in eax, a 32 bit x86 register. Because XORing itself, this initializes to 0.
scan_loop:								// Label this line to be jumped to.
		mov			ecx,w				// Move address of w into ECX 32 bit(4 byte) register. ECX is our count register, it will decrement while looping. ECX now points to w.
		xor			ebx,ebx				// Performs a logical XOR operation on EBX and EBX, then stores the result in EBX, a 32 bit x86 register. Because XORing itself, this initializes to 0.
pix_loop:								// Label this line to be jumped to.
		movq		mm4,[esi+ebx*8]		// mm0 = src (RG BA RG BA) // Moves a quadword(8 bytes) of data from address of esi+ebx*8 to mm4. ESI is src's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
		movq		mm5,[edi+ebx*8]		// mm1 = dst (RG BA RG BA) // Moves a quadword(8 bytes) of data from address of edi+ebx*8 to mm5. ESD is dst's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
// FIRST PIXEL
		movq		mm0,mm4				// mm0 = 00 00 RG BA // Moves the quadword(8 bytes) contents of mm4 into mm0. This will be RGBA, with each channel being 2 bytes, with a value of 0-255.
		movq		mm1,mm5				// mm1 = 00 00 RG BA // Moves the quadword(8 bytes) contents of mm5 into mm1. This will be RGBA, with each channel being 2 bytes, with a value of 0-255.
		punpcklbw	mm0,mm6				// mm0 = (0R 0G 0B 0A) // Interleaves low order bytes of mm0 and mm6 together, into mm0. http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		punpcklbw	mm1,mm7				// mm0 = (0R 0G 0B 0A) // Interleaves low order bytes of mm1 and mm7 together, into mm1. http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		pshufw		mm2,mm1,0ffh		// mm2 = 0A 0A 0A 0A   // Shuffles the words(2 bytes) from mm0 into mm2 using the third operand to define their placement in mm2. http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc254.htm
		movq		mm3,mm1				// mm3 = mm1		  // Moves the quadword(8 bytes) contents of mm1 to mm3.
		psubw		mm0,mm1				// mm0 = mm0 - mm1	  // Subtract packed word integers in mm1 from packed word integers in mm0. https://www.felixcloutier.com/x86/psubb:psubw:psubd
		psllw		mm3,8				// mm3 = mm1 * 256	  // Shift bits in mm3 left 8 places (Multiply by 256). https://www.felixcloutier.com/x86/psllw:pslld:psllq
		pmullw		mm0,mm2				// mm0 = (src-dst)*alpha // Multiply mm2 and mm0, and store low order bits of result in mm0 https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/eojdc/index.html
		paddw		mm0,mm3				// mm0 = (src-dst)*alpha+dst*256 // Add packed word integers mm3 and mm0, into mm0. https://www.felixcloutier.com/x86/paddb:paddw:paddd:paddq
		psrlw		mm0,8				// mm0 = ((src - dst) * alpha + dst * 256) / 256 // Shift words in mm0 right by 8 (Divide by 256). https://www.felixcloutier.com/x86/psrlw:psrld:psrlq
// SECOND PIXEL
		punpckhbw	mm5,mm7				// mm5 = (0R 0G 0B 0A) // Unpack and interleave high-order bytes from mm5 and mm7 into mm5. https://www.felixcloutier.com/x86/punpckhbw:punpckhwd:punpckhdq:punpckhqdq
		punpckhbw	mm4,mm6				// mm4 = (0R 0G 0B 0A) // Unpack and interleave high-order bytes from mm4 and mm6 into mm4. https://www.felixcloutier.com/x86/punpckhbw:punpckhwd:punpckhdq:punpckhqdq
		movq		mm3,mm5				// mm3 = mm5		  // Moves the quadword(8 bytes) contents of mm5 to mm3.
		pshufw		mm2,mm5,0ffh		// mm2 = 0A 0A 0A 0A  // Shuffles the words(2 bytes) from mm4 into mm2 using the third operand to define their placement in mm2. http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc254.htm
		psllw		mm3,8				// mm3 = mm5 * 256		// Shift bits in mm3 left 8 places (Multiply by 256). https://www.felixcloutier.com/x86/psllw:pslld:psllq
		psubw		mm4,mm5				// mm4 = mm4 - mm5		// Subtract packed word integers in mm5 from packed word integers in mm4. https://www.felixcloutier.com/x86/psubb:psubw:psubd
		pmullw		mm4,mm2				// mm4 = (src-dst)*alpha // Multiply mm2 and mm4, and store low order bits of result in mm4 https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/eojdc/index.html
		paddw		mm4,mm3				// mm4 = (src-dst)*alpha+dst*256 // Add packed word integers mm4 and mm3, into mm4. https://www.felixcloutier.com/x86/paddb:paddw:paddd:paddq
		psrlw		mm4,8				// mm4 = ((src - dst) * alpha + dst * 256) / 256 // Shift words in mm4 right by 8 (Divide by 256). https://www.felixcloutier.com/x86/psrlw:psrld:psrlq
		packuswb	mm0,mm4				// mm0 = RG BA RG BA // Converts 4 signed word integers from mm0 and 4 signed word integers from mm4 into 8 unsigned byte integers in mm0 using unsigned saturation. (Set to min of range if below, set to max of range if above)
		movq		[edi+ebx*8],mm0		// dst = mm0		// Moves the quadword(8 bytes) contents of mm0 to address indicated by edi+ebx*8. EDI is dst's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
		inc			ebx					// increment the value stored at the address of ebx	// Increment EBX
		loop		pix_loop			// loop back to pix_loop label. https://docs.oracle.com/cd/E19455-01/806-3773/instructionset-72/index.html Loop also decrements ECX, and continues without looping if that register is zero.
//
		mov			ebx, wmul4			// Move the address of wmul4 into ebx, creating a pointer to wmul4 in ebx.
		add			esi, ebx			// Add the contents of EBX into ESI
		add			edi, ebx			// Add the contents of EBX into EDI
		dec			edx					// Decrement EDX
		jnz			scan_loop			// Jump to label if zero flag is not set. Not entirely sure how and where the zero flag is being set or cleared here.
	}
}

// Intel will have the actual manual for these instructions, but Oracle seems to have a decent quick reference:
// https://docs.oracle.com/cd/E18752_01/html/817-5477/eojdc.html
/******************************************************************************/
void AlphaBltMMX(unsigned char *dst, unsigned char *src, int w, int h)
{
	int	wmul4 = w << 2;					// Shift bits of w 2 to the left, and assign that to wmul4. This multiplies w by 4 and assigns to wmul4.

	if (w==0) return;					// Return if image width is 0, meaning no image is loaded.
	w >>= 1;							// Shift bits of w 1 to the right. This results in a division of w by 2, assigned back to itself.
	_asm {
// For each pixel: dst = (src_alpha * (src-dst)  + dst * 256) / 256
		mov			edi,dst				// Move the address of index 0 of dst (unsigned char*(Because this is a pointer, it's an int, being compiled as x86, so 32 bits, or 4 bytes)) into EDI destination index register, for string operations
		mov			esi,src				// Move the address of index 0 of src (unsigned char*(Because this is a pointer, it's an int, being compiled as x86, so 32 bits, or 4 bytes)) into ESI source index register, for string operations
		mov			edx,h				// Move the address of h (int, 32 bits(4 bytes)) into 32-bit(4 byte) EDX register. EDX now points to h.
		pxor		mm6,mm6				// Performs a logical XOR operation on mm6 and mm6, then stores the result in mm6, a 64 bit MMX register. Because XORing itself, this initializes to 0.
		pxor		mm7,mm7				// Performs a logical XOR operation on mm7 and mm7, then stores the result in mm7, a 64 bit MMX register. Because XORing itself, this initializes to 0.
		xor			eax,eax				// Performs a logical XOR operation on eax and eax, then stores the result in eax, a 32 bit x86 register. Because XORing itself, this initializes to 0.
scan_loop:
		mov			ecx,w				// Move address of w into ECX 32 bit(4 byte) register. ECX is our count register, it will decrement while looping. ECX now points to w.
		xor			ebx,ebx				// Performs a logical XOR operation on EBX and EBX, then stores the result in EBX, a 32 bit x86 register. Because XORing itself, this initializes to 0.
pix_loop:
		movq		mm4,[esi+ebx*8]		// mm4 = src (RG BA RG BA) // Moves a quadword(8 bytes) of data from address of esi+ebx*8 to mm4. ESI is src's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
		movq		mm5,[edi+ebx*8]		// mm5 = dst (RG BA RG BA) // Moves a quadword(8 bytes) of data from address of edi+ebx*8 to mm5. ESD is dst's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
// FIRST PIXEL
		movq		mm0,mm4				// mm0 = src (-- -- RG BA) // Moves the quadword(8 bytes) contents of mm4 into mm0. This will be RGBA, with each channel being 2 bytes, with a value of 0-255.
		movq		mm1,mm5				// mm1 = dst (-- -- RG BA) // Moves the quadword(8 bytes) contents of mm5 into mm1. This will be RGBA, with each channel being 2 bytes, with a value of 0-255.
		punpcklbw	mm0,mm6				// mm0 = (0R 0G 0B 0A) // Interleaves low order bytes of mm0 and mm6 together, into mm0. http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		mov			al,[esi+ebx*8+3]	// eax = pixel alpha (0 - 255) // Move memory contents of esi+ebx*8+3 into al.
		punpcklbw	mm1,mm7				// mm1 = (0R 0G 0B 0A) // Interleaves low order bytes of mm1 and mm7 together, into mm1. http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		movd		mm2,eax				// 00 00 00 0A  // Moves a doubleword(4 bytes) of data from eax to mm2
		movq		mm3,mm1				// mm3 = mm1: dst (0R 0G 0B 0A) // Moves a quadword(8 bytes) of data from mm1 to mm3
		punpcklwd	mm2,mm2				// 00 00 0A 0A // Interleave words from low doublewords of mm2 into itself? http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		psubw		mm0,mm1				// mm0 = mm0 - mm1 // Subtract packed word integers in mm1 from packed word integers in mm0. https://www.felixcloutier.com/x86/psubb:psubw:psubd
		punpckldq	mm2,mm2				// 0A 0A 0A 0A // Interleave low doublewords of mm2 into itself? http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		psllw		mm3,8				// mm3 = mm1 * 256 // Shift bits in mm3 left 8 places (Multiply by 256). https://www.felixcloutier.com/x86/psllw:pslld:psllq
		pmullw		mm0,mm2				// mm0 = (src - dst) * alpha // Multiply mm2 and mm0, and store low order bits of result in mm0 https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/eojdc/index.html
		paddw		mm0,mm3				// mm0 = (src - dst) * alpha + dst * 256 // Add packed word integers mm3 and mm0, into mm0. https://www.felixcloutier.com/x86/paddb:paddw:paddd:paddq
		psrlw		mm0,8				// mm0 = ((src - dst) * alpha + dst * 256) / 256 // Shift words in mm0 right by 8 (Divide by 256). https://www.felixcloutier.com/x86/psrlw:psrld:psrlq
		packuswb	mm0,mm6				// mm0 = RGBA // Converts 4 signed word integers from mm0 and 4 signed word integers from mm6 into 8 unsigned byte integers in mm0 using unsigned saturation. (Set to min of range if below, set to max of range if above)
// SECOND PIXEL
		punpckhbw	mm4,mm6				// mm4 = (0R 0G 0B 0A) // Unpack and interleave high-order bytes from mm6 and mm4 into mm4. https://www.felixcloutier.com/x86/punpckhbw:punpckhwd:punpckhdq:punpckhqdq
		mov			al,[esi+ebx*8+7]	// eax = pixel alpha (0 - 255) // Move memory contents of esi+ebx*8+3 into al.
		punpckhbw	mm5,mm7				// mm5 = (0R 0G 0B 0A) // Unpack and interleave high-order bytes from mm7 and mm5 into mm5. https://www.felixcloutier.com/x86/punpckhbw:punpckhwd:punpckhdq:punpckhqdq
		movd		mm2,eax				// 00 00 00 0A // Moves a doubleword(4 bytes) of data from eax to mm2
		movq		mm3,mm5				// mm3 = mm5: dst (0R 0G 0B 0A) // Moves a quadword(8 bytes) of data from mm5 to mm3
		punpcklwd	mm2,mm2				// 00 00 0A 0A // Interleave words from low doublewords of mm2 into itself? http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		psubw		mm4,mm5				// mm4 = mm4 - mm5 // Subtract packed word integers in mm5 from packed word integers in mm4. https://www.felixcloutier.com/x86/psubb:psubw:psubd
		punpckldq	mm2,mm2				// 0A 0A 0A 0A // Interleave low doublewords of mm2 into itself? http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		psllw		mm3,8				// mm3 = mm5 * 256 // Shift bits in mm3 left 8 places (Multiply by 256). https://www.felixcloutier.com/x86/psllw:pslld:psllq
		pmullw		mm4,mm2				// mm4 = (src - dst) * alpha // Multiply mm2 and mm4, and store low order bits of result in mm4 https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/eojdc/index.html
		paddw		mm4,mm3				// mm4 = (src - dst) * alpha + dst * 256 // Add packed word integers mm3 and mm4, into mm4. https://www.felixcloutier.com/x86/paddb:paddw:paddd:paddq
		psrlw		mm4,8				// mm4 = ((src - dst) * alpha + dst * 256) / 256 // Shift words in mm4 right by 8 (Divide by 256). https://www.felixcloutier.com/x86/psrlw:psrld:psrlq
		packuswb	mm4,mm6				// mm4 = RGBA // Converts 4 signed word integers from mm6 and 4 signed word integers from mm4 into 8 unsigned byte integers in mm4 using unsigned saturation. (Set to min of range if below, set to max of range if above)
		punpckldq	mm0,mm4				// mm0 = RG BA RG BA // Interleave low doublewords of mm0 and mm4 into mm0 http://qcd.phys.cmu.edu/QCDcluster/intel/vtune/reference/vc265.htm
		movq		[edi+ebx*8],mm0		// dst = mm0 // Moves a quadword(8 bytes) of data from address of mm0 to address indicated by edi+ebx*8. EDI is dst's starting address+EBX which is an offset counter, of 8 bytes. EBX is a counter.
		inc			ebx					// increment the value stored at the address of ebx	// Increment EBX
// REPEAT
		loop		pix_loop			// loop back to pix_loop label. https://docs.oracle.com/cd/E19455-01/806-3773/instructionset-72/index.html Loop also decrements ECX, and continues without looping if that register is zero.
		mov			ebx, wmul4			// Move the address of wmul4 into ebx, creating a pointer to wmul4 in ebx.
		add			esi, ebx			// Add the contents of EBX into ESI
		add			edi, ebx			// Add the contents of EBX into EDI
		dec			edx					// Decrement EDX
		jnz			scan_loop			// Jump to label if zero flag is not set. Not entirely sure how and where the zero flag is being set or cleared here.
	}
}

