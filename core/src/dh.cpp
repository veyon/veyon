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

#include "rfb/dh.h"
#include "Logger.h"

#define throw
#define Except(x) qCritical(x)

DiffieHellman::DiffieHellman() : maxNum(((uint64_t) 1) << DH_MAX_BITS) {
	srand((unsigned) time(NULL));
}

DiffieHellman::DiffieHellman(uint64_t generator, uint64_t modulus)
	: gen(generator), mod(modulus),
	  maxNum(((uint64_t) 1) << DH_MAX_BITS) {
	if (gen > maxNum || mod > maxNum)
		throw Except("Input exceeds maxNum");
	if (gen > mod)
		throw Except("Generator is larger than modulus");
	srand((unsigned) time(NULL));
}

DiffieHellman::~DiffieHellman() { cleanMem(); }

uint64_t rng(uint64_t limit) {
	return ((((uint64_t) rand()) * rand() * rand ()) % limit);
}

//Performs the miller-rabin primality test on a guessed prime n.
//trials is the number of attempts to verify this, because the function
//is not 100% accurate it may be a composite.  However setting the trial
//value to around 5 should guarantee success even with very large primes
bool DiffieHellman::millerRabin (uint64_t n, unsigned int trials) { 
	uint64_t a = 0; 

	for (unsigned int i = 0; i < trials; i++) { 
		a = rng(n - 3) + 2;// gets random value in [2..n-1] 
		if (XpowYmodN(a, n - 1, n) != 1) return false; //n composite, return false 
	}
	return true; // n probably prime 
} 

//Generates a large prime number by
//choosing a randomly large integer, and ensuring the value is odd
//then uses the miller-rabin primality test on it to see if it is prime
//if not the value gets increased until it is prime
uint64_t DiffieHellman::generatePrime() {
	uint64_t prime = 0;

	do {
		uint64_t start = rng(maxNum);
		prime = tryToGeneratePrime(start);
	} while (!prime);
	return prime;
}
 
uint64_t DiffieHellman::tryToGeneratePrime(uint64_t prime) {
	//ensure it is an odd number
	if ((prime & 1) == 0)
		prime += 1;

	uint64_t cnt = 0;
	while (!millerRabin(prime, 25) && (cnt++ < DH_RANGE) && prime < maxNum) {
		prime += 2;
		if ((prime % 3) == 0) prime += 2;
	}
	return (cnt >= DH_RANGE || prime >= maxNum) ? 0 : prime;
}
 
//Raises X to the power Y in modulus N
//the values of X, Y, and N can be massive, and this can be 
//achieved by first calculating X to the power of 2 then 
//using power chaining over modulus N
uint64_t DiffieHellman::XpowYmodN(uint64_t x, uint64_t y, uint64_t N) {
	uint64_t result = 1;
	const uint64_t oneShift63 = ((uint64_t) 1) << 63;
	
	for (int i = 0; i < 64; y <<= 1, i++){
		result = result * result % N;
		if (y & oneShift63)
			result = result * x % N;
	}
	return result;
}

void DiffieHellman::createKeys() {
	gen = generatePrime();
	mod = generatePrime();

	if (gen > mod) {
		uint64_t swap = gen;
		gen  = mod;
		mod  = swap;
	}
}

uint64_t DiffieHellman::createInterKey() {
	priv = rng(maxNum);
	return pub = XpowYmodN(gen,priv,mod);
}

uint64_t DiffieHellman::createEncryptionKey(uint64_t interKey) {
	if (interKey >= maxNum)
		throw Except("interKey larger than maxNum");
	return key = XpowYmodN(interKey,priv,mod);
}

void DiffieHellman::cleanMem(int flags) { // marscha (TODO): SecureZeroMemory?
	gen  = 0;
	mod  = 0;
	priv = 0;
	pub  = 0;
	
	if (flags != DH_CLEAN_ALL_MEMORY_EXCEPT_KEY)
		key = 0;
}

uint64_t DiffieHellman::getValue(int flags) {
	switch (flags) {
		case DH_MOD:
			return mod;
		case DH_GEN:
			return gen;
		case DH_PRIV:
			return priv;
		case DH_PUB:
			return pub;
		case DH_KEY:
			return key;
		default:
			return (uint64_t) 0;
	}
}

int bits(int64_t number){
	for (unsigned int i = 0; i < 64; i++){
		number /= 2;
		if (number < 2) return i;
	}
	return 0;
}

bool int64ToBytes(const uint64_t integer, char* const bytes) {
	for (int i = 0; i < 8; i++) {
		bytes[i] = (unsigned char) (integer >> (8 * (7 - i)));
	}
	return true;
}

uint64_t bytesToInt64(const char* const bytes) {
	uint64_t result = 0;
	for (int i = 0; i < 8; i++) {
		result <<= 8;
		result += (unsigned char) bytes[i];
	}
	return result;
}

