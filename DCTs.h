#pragma once
#include<math.h>
typedef float DCT_W_t[64][64];
float dct_w(
	const int x, const int y,
	const int u, const int v){
	const float pi = (float)3.14159265358979323846;
	const float W = (float)(
		cos(((2.0*x + 1.) *u*pi) / 16.)
		*cos(((2.0*y + 1)*v*pi) / 16.));
	const float Cu
		= (float)((u == 0) ? (1. / sqrt(2)) : 1.);
	const float Cv
		= (float)((v == 0) ? (1. / sqrt(2)) : 1.);
	return 0.25f * Cu * Cv * W;
}
void init_dctinv_table(DCT_W_t W){
	for(int x = 0; x < 8; x++){
		for(int y = 0; y < 8; y++){
			for(int u = 0; u < 8; u++){
				for(int v = 0; v < 8; v++){
					W[y * 8 + x][v * 8 + u]
						= dct_w(x, y, u, v);
				}
			}
		}
	}
}
void block_dct_inv(const int recp[64],
	float real[64]){
	for(int x = 0; x < 8; x++){
		for(int y = 0; y < 8; y++){
			for(int u = 0; u < 8; u++){
				for(int v = 0; v < 8; v++){
					real[y * 8 + x]
						+= recp[v * 8 + u]
						* dct_w(x, y, u, v);
				}
			}
		}
	}
}
void block_dct_inv_table(const DCT_W_t Ws,
	const int recp[64], float real[64]){
	for(int t = 0; t < 64; t++){
		for(int w = 0; w < 64; w++){
			real[t] += recp[w] * Ws[t][w];
		}
	}
}
void dct_inv(
	const int recp[][64], float real[][64],
	const DCT_W_t W, const int num_block){
	for(int i = 0; i < num_block; i++){
		block_dct_inv_table(W,
			recp[i], real[i]);
	}
}
