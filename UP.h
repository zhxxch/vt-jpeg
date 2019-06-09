#include"RGB.h"
#include<math.h>
float entropy_up_block(
	const int recp[64], const float real[64]){
	float S_recp = 0, S_real = 0,
		power_recp = 0, power_real = 0;
	float power_recps[64], power_reals[64];
	for(int f = 0; f < 64; f++){
		power_recps[f]
			= (float)recp[f] * (float)recp[f];
		power_reals[f]
			= (float)real[f] * (float)real[f];
	}
	for(int f = 0; f < 64; f++){
		power_recp += power_recps[f];
		power_real += power_reals[f];
	}
	for(int f = 0; f < 64; f++){
		const float normpower_recp
			= power_recps[f] / power_recp;
		const float normpower_real
			= power_reals[f] / power_real;
		S_recp -= (normpower_recp > 0
			? (normpower_recp
				* (float)log2(normpower_recp)) : 0);
		S_real -= (normpower_real > 0
			? (normpower_real
				* (float)log2(normpower_real)) : 0);
	}
	return S_recp + S_real;
}
void entropy_up_ycc(
	const int recp[][64], const float real[][64],
	const int num_block, float ineqres[][64]){
	for(int i = 0; i < num_block; i++){
		const float up
			= entropy_up_block(recp[i], real[i]);
		const float px_val
			= ((up - 4.f)*32.f) - 128.f;
		for(int f = 0; f < 64; f++){
			ineqres[i][f] = px_val;
		}
	}
}