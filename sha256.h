#pragma once
const static unsigned int SHA256_K[64] = {
		0x428a2f98, 0x71374491,
		0xb5c0fbcf, 0xe9b5dba5,
		0x3956c25b, 0x59f111f1,
		0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01,
		0x243185be, 0x550c7dc3,
		0x72be5d74, 0x80deb1fe,
		0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786,
		0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa,
		0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d,
		0xb00327c8, 0xbf597fc7,
		0xc6e00bf3, 0xd5a79147,
		0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138,
		0x4d2c6dfc, 0x53380d13,
		0x650a7354, 0x766a0abb,
		0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b,
		0xc24b8b70, 0xc76c51a3,
		0xd192e819, 0xd6990624,
		0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08,
		0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a,
		0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f,
		0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb,
		0xbef9a3f7, 0xc67178f2,
};

typedef union{
	struct{
		unsigned int a, b, c, d, e, f, g, h;
	}comps;
	unsigned int vec8[8];
	unsigned long long vec4[4];
}SHA256_DIGEST_t;

SHA256_DIGEST_t SHA256_init(void){
	const SHA256_DIGEST_t h = {
		0x6a09e667ul, 0xbb67ae85ul,
		0x3c6ef372ul, 0xa54ff53aul,
		0x510e527ful, 0x9b05688cul,
		0x1f83d9abul, 0x5be0cd19ul};
	return h;
}
SHA256_DIGEST_t SHA256_iter(
	const SHA256_DIGEST_t h, const void* input){
	typedef const unsigned int cu32;
	unsigned int w[64];
#define ENDIANXX(x) ((x)>>24)|(((x)&0xff0000)>>8)|(((x)&0xff00)<<8)|(((x)&0xff)<<24);
#define RR(x,r) ((((x)&0xffffffff)>>r)|((x)<<(32-r)))
#define S0(A) (RR((A),2)^RR((A),13)^RR((A),22))
#define S1(E) (RR((E),6)^RR((E),11)^RR((E),25))
#define CH(E,F,G) (((E)&(F))^((~(E))&(G)))
#define MAJ(A,B,C) (((A)&(B))^((A)&(C))^((B)&(C)))
	for(int i = 0; i < 16; i++){
		const unsigned int w_i = ((cu32*)input)[i];
		w[i] = ENDIANXX(w_i);
	}
	for(int i = 16; i < 64; i++){
		cu32 w15 = w[i - 15];
		cu32 w2 = w[i - 2];
		cu32 s0 = RR(w15, 7)
			^ RR(w15, 18) ^ (w15 >> 3);
		cu32 s1 = RR(w2, 17)
			^ RR(w2, 19) ^ (w2 >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}
	SHA256_DIGEST_t t = h;
	for(int i = 0; i < 64; i++){
		cu32 s1 = S1(t.comps.e);
		cu32 ch
			= CH(t.comps.e, t.comps.f, t.comps.g);
		cu32 temp1
			= t.comps.h + s1 + ch + SHA256_K[i] + w[i];
		cu32 s0 = S0(t.comps.a);
		cu32 maj
			= MAJ(t.comps.a, t.comps.b, t.comps.c);
		cu32 temp2 = s0 + maj;
		t.comps.h = t.comps.g;
		t.comps.g = t.comps.f;
		t.comps.f = t.comps.e;
		t.comps.e = t.comps.d + temp1;
		t.comps.d = t.comps.c;
		t.comps.c = t.comps.b;
		t.comps.b = t.comps.a;
		t.comps.a = temp1 + temp2;
	}
#undef ENDIANXX
#undef RR
#undef S0
#undef S1
#undef CH
#undef MAJ
	for(int i = 0; i < 8; i++){
		t.vec8[i] = h.vec8[i] + t.vec8[i];
	}
	return t;
}
SHA256_DIGEST_t SHA256_fin(const SHA256_DIGEST_t h,
	const void* input, const int ttl_sz){
	SHA256_DIGEST_t t = h;
	const unsigned long long bitlen = ttl_sz * 8;
	unsigned long long len_be
		= ((bitlen & 0xff00ff00ff00ff00ull) >> 8)
		| ((bitlen & 0x00ff00ff00ff00ffull) << 8);
	len_be
		= ((len_be & 0xffff0000ffff0000ull) >> 16)
		| ((len_be & 0xffff0000ffffull) << 16);
	len_be
		= ((len_be & 0xffffffff00000000ull) >> 32)
		| ((len_be & 0xffffffffull) << 32);
	const unsigned char* input_bytes
		= (const unsigned char*)input;
	unsigned char m[64] = {0};
	const int last_byte_offset = ttl_sz & 0x3f;
	for(int i = 0; i < last_byte_offset; i++){
		m[i] = input_bytes[i];
	}
	m[last_byte_offset] = 0x80;
	if((64 - last_byte_offset < 9)){
		t = SHA256_iter(h, m);
		for(int i = 0; i < 64; i++){
			m[i] = 0;
		}
	}
	for(int i = 0; i < 8; i++){
		m[63 - i] = (unsigned int)
			((bitlen >> (i * 8)) & 0xff);
	}
	return SHA256_iter(t, m);
}
SHA256_DIGEST_t SHA256(
	const void *input, const int size){
	SHA256_DIGEST_t h = SHA256_init();
	const char *input_bytes = (const char*)input;
	for(int remain_sz = size; remain_sz >= 64;
		remain_sz-=64){
		h = SHA256_iter(h, input_bytes);
		input_bytes += 64;
	}
	return SHA256_fin(h, input_bytes, size);
}
int SHA256DIFF( const void* a, const void* b){
	unsigned int d = 0;
	for(int i = 0; i < 8; i++){
		d |= ((SHA256_DIGEST_t*)a)->vec8[i]
			- ((SHA256_DIGEST_t*)a)->vec8[i];
	}
	return d;
}