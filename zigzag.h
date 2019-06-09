#pragma once
#include<string.h>
#include"DHTs.h"
inline void zz_to_scanlines(void* scanlines,
	const void* zzbuffer, const int elem_size,
	const int Ss, const int Se){
	/* const int zz[64] = {
		0,1,5,6,14,15,27,28,
		2,4,7,13,16,26,29,42,
		3,8,12,17,25,30,41,43,
		9,11,18,24,31,40,44,53,
		10,19,23,32,39,45,52,54,
		20,22,33,38,46,51,55,60,
		21,34,37,47,50,56,59,61,
		35,36,48,49,57,58,62,63,
	};*/
	const int zz2[64] = {
		0,1,8,16,9,2,3,10,
		17,24,32,25,18,11,4,5,
		12,19,26,33,40,48,41,34,
		27,20,13,6,7,14,21,28,
		35,42,49,56,57,50,43,36,
		29,22,15,23,30,37,44,51,
		58,59,52,45,38,31,39,46,
		53,60,61,54,47,55,62,63,
	};
	for(int f = Ss; f <= Se; f++){
		memcpy(
			(char*)scanlines + zz2[f] * elem_size,
			(const char*)zzbuffer + f * elem_size,
			elem_size);
	}
}
inline void zz_to_scanline_vec(
	const huffman_unit_block zz[], const int count,
	huffman_unit_block scanlines[]){
	const int zz2[64] = {
		0,1,8,16,9,2,3,10,
		17,24,32,25,18,11,4,5,
		12,19,26,33,40,48,41,34,
		27,20,13,6,7,14,21,28,
		35,42,49,56,57,50,43,36,
		29,22,15,23,30,37,44,51,
		58,59,52,45,38,31,39,46,
		53,60,61,54,47,55,62,63,
	};
	for(int i = 0; i < count; i++){
		huffman_unit_block t;
		memcpy(t, zz[i], sizeof t);
		for(int f = 0; f <= 63; f++){
			scanlines[i][zz2[f]] = t[f];
		}
	}
}
