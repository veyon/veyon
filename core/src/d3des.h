#ifndef D3DES_H
#define D3DES_H

/*
 * This is D3DES (V5.09) by Richard Outerbridge with the double and
 * triple-length support removed for use in VNC.
 *
 * These changes are:
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * ============================================================================
 * SECURITY WARNING: DES (Data Encryption Standard) is CRYPTOGRAPHICALLY BROKEN
 * ============================================================================
 * 
 * DES should not be used for any security-sensitive applications:
 * - 56-bit effective key length can be brute-forced with modern hardware
 * - Known cryptanalytic attacks exist
 * - Deprecated by NIST and industry standards
 * 
 * This implementation is maintained ONLY for backward compatibility with 
 * legacy VNC protocol implementations. For new code, use modern algorithms
 * such as AES-256, ChaCha20, or other NIST-approved ciphers.
 * 
 * See SECURITY-NOTES.md for detailed security considerations.
 * ============================================================================
 */

/* d3des.h -
 *
 *	Headers and defines for d3des.c
 *	Graven Imagery, 1992.
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge
 *	(GEnie : OUTER; CIS : [71755,204])
 */

#define EN0	0	/* MODE == encrypt */
#define DE1	1	/* MODE == decrypt */

extern void rfbDesKey(unsigned char *, int);
/*		      hexkey[8]     MODE
 * Sets the internal key register according to the hexadecimal
 * key contained in the 8 bytes of hexkey, according to the DES,
 * for encryption or decryption according to MODE.
 */


extern void rfbDes(unsigned char *, unsigned char *);
/*		    from[8]	      to[8]
 * Encrypts/Decrypts (according to the key currently loaded in the
 * internal key register) one block of eight bytes at address 'from'
 * into the block at address 'to'.  They can be the same.
 */

/* d3des.h V5.09 rwo 9208.04 15:06 Graven Imagery
 ********************************************************************/

#endif
