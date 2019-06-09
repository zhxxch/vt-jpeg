#pragma once
#include"zigzag.h"
#include"markers.h"
struct q_table{
	int zz[64];
	int scan[64];
};
union dqt_slots{
	struct q_table vec[4];
};
union dqt_slots update_dqt(
	const unsigned char dqt_data[],
	const int dqt_len,
	union dqt_slots prev_dqts){
	typedef unsigned char u8;
	for(int fin_len = 0; fin_len < dqt_len;){
		const u8 *dqt_part = dqt_data + fin_len;
		if((dqt_part[0] & 0xf0) > 0){
			fin_len += 1 + 2 * 64;
		} else{
			struct q_table *dst = &prev_dqts.vec[
				dqt_part[0] & 0xf];
			for(int i = 0; i < 64; i++){
				dst->zz[i] = dqt_part[1 + i];
			}
			zz_to_scanlines(dst->scan, dst->zz,
				sizeof dst->zz[0],0,63);
			fin_len += 1 + 64;
		}
	}
	return prev_dqts;
}
void quantization_inv(const int qtable[64],
	const int coeffs_input[64],
	int coeffs_output[64]){
	for(int i = 0; i < 64; i++){
		coeffs_output[i]
			= qtable[i] * coeffs_input[i];
	}
}
void quant_inv_frame(const int coeffs_input[][64],
	const union dqt_slots QTables,
	const JPEG_SOF_t frame,
	int coeffs_output[][64]){
	for(int ch_i = 0, block_it = 0;
		ch_i < frame.Nf_numch; ch_i++){
		const int num_block
			= frame.Xmcus*frame.channels[ch_i].Q.H
			* frame.Ymcus*frame.channels[ch_i].Q.V;
		const int *q_table = QTables.vec[frame.channels[ch_i].Q.TableQ].scan;
		for(int i = 0; i < num_block; i++){
			quantization_inv(q_table,
				coeffs_input[block_it + i],
				coeffs_output[block_it + i]);
		}
		block_it += num_block;
	}
}
#include<math.h>
float quant_block_cutoff_bits(const int q[64]){
	float A = 0;
	for(int i = 0; i < 64; i++){
		A += (float)log2(q[i]);
	}
	return A;
}