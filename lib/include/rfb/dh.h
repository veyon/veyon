// CRYPTO LIBRARY FOR EXCHANGING KEYS
// USING THE DIFFIE-HELLMAN KEY EXCHANGE PROTOCOL

// The diffie-hellman can be used to securely exchange keys
// between parties, where a third party eavesdropper given
// the values being transmitted cannot determine the key.

// Implemented by Lee Griffiths, Jan 2004.
// This software is freeware, you may use it to your discretion,
// however by doing so you take full responsibility for any damage
// it may cause.

// Hope you find it useful, even if you just use some of the functions
// out of it like the prime number generator and the XtoYmodN function.

// It would be great if you could send me emails to: lee.griffiths@first4internet.co.uk
// with any suggestions, comments, or questions!

// Enjoy.

// Adopted to ms-logon for ultravnc by marscha, 2006.

#ifndef __RFB_DH_H__
#define __RFB_DH_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#define DH_MAX_BITS 31
#define DH_RANGE 100

#define DH_CLEAN_ALL_MEMORY				1
#define DH_CLEAN_ALL_MEMORY_EXCEPT_KEY		2

#define DH_MOD	1
#define DH_GEN	2
#define DH_PRIV	3
#define DH_PUB	4
#define DH_KEY	5

#ifdef ULTRAVNC_ITALC_SUPPORT
#define DH DiffieHellman
#endif

class DiffieHellman
{
public:
	DiffieHellman();
	DiffieHellman(uint64_t generator, uint64_t modulus);
	~DiffieHellman();

	void createKeys();
	uint64_t createInterKey();
	uint64_t createEncryptionKey(uint64_t interKey);
	
	uint64_t getValue(int flags = DH_KEY);

private:
	uint64_t XpowYmodN(uint64_t x, uint64_t y, uint64_t N);
	uint64_t generatePrime();
	uint64_t tryToGeneratePrime(uint64_t start);
	bool millerRabin (uint64_t n, unsigned int trials);
	void cleanMem(int flags=DH_CLEAN_ALL_MEMORY);


	uint64_t gen;
	uint64_t mod;
	uint64_t priv;
	uint64_t pub;
	uint64_t key;
	uint64_t maxNum;

};

int bits(int64_t number);
bool int64ToBytes(const uint64_t integer, char* const bytes);
uint64_t bytesToInt64(const char* const bytes);

#endif // __RFB_DH_H__
