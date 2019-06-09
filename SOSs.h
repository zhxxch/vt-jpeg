#pragma once
#include"assertexw.h"
#include"markers.h"
#include"jpg-fmt.h"
inline int SOS_header(
	const unsigned char marker_content[],
	const JPEG_SOF_t frame, JPEG_SOS_t *header){
	header->Ns_numch = marker_content[0];
	const unsigned char *components
		= marker_content + 1;
	header->Hmax = 0;
	header->Vmax = 0;
	for(int i = 0; i < header->Ns_numch; i++){
		header->channels[i].C_id
			= components[2 * i];
		const int Tid = components[2 * i + 1];
		header->channels[i].H.TableDC = Tid >> 4;
		header->channels[i].H.TableAC = Tid & 0xf;
		const int frame_ch_idx
			= header->channels[i].C_id - 1;
		const union channel_sw frame_ch
			= frame.channels[frame_ch_idx];
		header->channels[i].H.TableQ
			= frame_ch.Q.TableQ;
		header->sample_dims[i].S.H = frame_ch.Q.H;
		header->sample_dims[i].S.V = frame_ch.Q.V;
		header->Hmax
			= (frame_ch.Q.H > header->Hmax) ?
			frame_ch.Q.H : header->Hmax;
		header->Vmax
			= (frame_ch.Q.V > header->Vmax) ?
			frame_ch.Q.V : header->Vmax;
	}
	const int mcu_x = 8 * ((header->Ns_numch == 1)
		? (frame.Hmax / header->Hmax) : (header->Hmax));
	const int mcu_y = 8 * ((header->Ns_numch == 1)
		? (frame.Vmax / header->Vmax) : (header->Vmax));
	header->Xmcus = (frame.X_numpx
		+ mcu_x - 1) / mcu_x;
	header->Ymcus = (frame.Y_numline
		+ mcu_y - 1) / mcu_y;
	for(int i = 0; i < header->Ns_numch; i++){
		header->sample_dims[i].S.x
			= ((header->Ns_numch == 1) ?
				1 : (header->sample_dims[i].S.H))
			* header->Xmcus;
		header->sample_dims[i].S.y
			= ((header->Ns_numch == 1) ?
				1 : (header->sample_dims[i].S.V))
			* header->Ymcus;
	}
	const unsigned char *rests
		= components + header->Ns_numch * 2;
	header->Ss = rests[0];
	header->Se = rests[1];
	const int Abits = rests[2];
	header->Ah = Abits >> 4;
	header->Al = Abits & 0xf;
	return (4 + 2 * (header->Ns_numch));
}
int scan_stat(
	const unsigned char scan_content[],
	const int scan_size, const int Ri,
	const union dht_slots Tables,
	const struct jpeg_scan_header scan,
	huffman_unit_block huffman_stream[]){
	int finbitlen = 0;
	const int num_mcu = scan.Xmcus * scan.Ymcus;
	const int num_block = mcu_ttl_num_block(
		scan.sample_dims, scan.Ns_numch);
	for(int mcu_i = 0, r = Ri,
		eob_counter = 0, mcu_bitlen = 0;
		(finbitlen < 8 * scan_size)
		&& (mcu_i < num_mcu); mcu_i++){
		if((scan.Ss + scan.Se) == 0){
			mcu_bitlen = pro_dc_mcu_stat(
				scan_content, finbitlen,
				8 * scan_size, Tables, scan,
				huffman_stream + mcu_i * num_block);
		} else if(scan.Ss > 0){
			mcu_bitlen = mcu_ac_stat_init(
				scan_content, finbitlen,
				Tables.tables.AC[
					scan.channels[0].H.TableAC],
				scan.Ss, scan.Se, scan.Al,
						&eob_counter,
						huffman_stream[mcu_i]);
		} else{
			mcu_bitlen = seq_mcu_stat(
				scan_content, finbitlen,
				8 * scan_size, Tables, scan,
				huffman_stream + mcu_i * num_block);
		}
		finbitlen += (mcu_bitlen > 0 ?
			mcu_bitlen : 0);
		if(--r == 0){
			r = Ri;
			const int finbytelen
				= (finbitlen + 7) / 8;
			if(scan_content[finbytelen] == 0xff
				&& marker_RSTn(scan_content[
					finbytelen + 1]) >= 0){
				finbitlen = 8 * (finbytelen + 2);
			} else{/* RSTn ERROR */ }
		}
	}
	return finbitlen;
}
int scan_stat_ac_approx(
	const unsigned char scan_content[],
	const int scan_size, const int Ri,
	const union dht_slots Tables,
	const JPEG_SOS_t scan, const JPEG_SOF_t frame,
	const huffman_unit_block ACs_prev[],
	huffman_unit_block ACs_approx[]){
	const int buffer_bitlen = scan_size * 8;
	const int ac_prev_offset = frame
		.ch_block_offset[scan.channels[0].C_id - 1];
	const int x_num_block_seq
		= frame.Xmcus * frame.channels[
			scan.channels[0].C_id - 1].Q.H;
	const int x_num_block_ilv
		= scan.sample_dims[0].S.x;
	const int y_num_block = scan.sample_dims[0].S.y;
	int zero_counter = 0;
	struct huffman_unit new_ac_ampl = {0};
	int finbitlen = 0, r = Ri;
	for(int mcu_y = 0; mcu_y < y_num_block;
		mcu_y++){
		for(int mcu_x = 0; mcu_x < x_num_block_ilv
			&& finbitlen < buffer_bitlen;
			mcu_x++){
			const int mcu_bitlen
				= mcu_ac_stat_approx(
					scan_content, finbitlen,
					Tables.tables.AC[
						scan.channels[0].H.TableAC],
					scan.Ss, scan.Se, scan.Al,
							&zero_counter,
							&new_ac_ampl,
							ACs_prev[ac_prev_offset
							+ mcu_x + mcu_y
							* x_num_block_seq],
							ACs_approx[mcu_x + mcu_y
							* x_num_block_ilv]);
			finbitlen += (mcu_bitlen > 0 ?
				mcu_bitlen : 0);
			if(--r == 0){
				r = Ri;
				finbitlen = (finbitlen + 7) / 8;
				if(scan_content[finbitlen] == 0xff
					&& marker_RSTn(scan_content[
						finbitlen + 1]) >= 0){
					finbitlen = 8 * (finbitlen + 2);
				} else{
							/* RSTn ERROR */
						}
			}
		}
	}
	return finbitlen;
}
void interleav_collect(
	const huffman_unit_block interleaved[],
	const JPEG_SOS_t scan,
	huffman_unit_block output[]){
	const int NumMcu = scan.Xmcus * scan.Ymcus;
	int mcu_num_block = 0;
	int ch_offset[3] = {0};
	int ch_num_block[3] = {0};
	for(int ch_i = 0; ch_i < scan.Ns_numch; ch_i++){
		ch_offset[ch_i] = mcu_num_block;
		ch_num_block[ch_i]
			= scan.sample_dims[ch_i].S.H
			* scan.sample_dims[ch_i].S.V;
		mcu_num_block += ch_num_block[ch_i];
	}
	if(scan.Ns_numch == 1){
		mcu_num_block = 1;
		ch_num_block[0] = 1;
	}
	int output_block_iter = 0;
	for(int ch_i = 0; ch_i < scan.Ns_numch; ch_i++){
		for(int mcu_i = 0; mcu_i < NumMcu; mcu_i++){
			memcpy(output[output_block_iter],
				interleaved[mcu_i*mcu_num_block
				+ ch_offset[ch_i]],
				ch_num_block[ch_i]
				* sizeof output[0]);
			output_block_iter += ch_num_block[ch_i];
		}
	}
}
int interleav_transpose_channel(
	const huffman_unit_block collect[],
	const union channel_sw sample_dims,
	const int x_num_block_seq,
	huffman_unit_block output[]){
	const int y_num_block = sample_dims.S.y;
	const int x_num_block_ilv = sample_dims.S.x;
	const int H = sample_dims.S.H;
	const int V = sample_dims.S.V;
	for(int line_it = 0; line_it < y_num_block / V;
		line_it++){
		for(int v_it = 0; v_it < V; v_it++){
			for(int h_it = 0; h_it < x_num_block_ilv;
				h_it += H){
				memcpy(output[
					line_it * V * x_num_block_seq
						+ v_it * x_num_block_seq
						+ h_it],
					collect[line_it
						* V * x_num_block_ilv
						+ h_it * V + v_it * H],
						H * sizeof output[0]);
			}
		}
	}
	return y_num_block * x_num_block_ilv;
}
void dc_ampl_accumul(
	const int num_blocks, const int Ri,
	huffman_unit_block blocks[]){
	for(int i = 1, r = Ri - 1; i < num_blocks; i++){
		blocks[i][0].ampl = blocks[i][0].ampl
			+ blocks[i - 1][0].ampl;
		if(--r == 0){
			r = Ri - 1;
			i++;
		}
	}
}
void dc_ampl_accumul_collect(
	const JPEG_SOS_t scan, const int Ri,
	huffman_unit_block hstream[]){
	for(int i = 0, block_it = 0;
		i < scan.Ns_numch; i++){
		const int num_block
			= scan.sample_dims[i].S.x
			* scan.sample_dims[i].S.y;
		const int mcu_num_block
			= scan.sample_dims[i].S.H
			* scan.sample_dims[i].S.V;
		dc_ampl_accumul(num_block,
			Ri*mcu_num_block, hstream + block_it);
		block_it += num_block;
	}
}
void coeffs_add_transposed(const int num_block,
	const huffman_unit_block stream_approx[],
	huffman_unit_block stream_init[]){
	for(int i = 0; i < num_block; i++){
		for(int f = 0; f < 64; f++){
			stream_init[i][f].ampl
				+= stream_approx[i][f].ampl;
		}
	}
}
void copy_ampl(
	const struct huffman_unit blocks[],
	const int count, int dst[]){
	for(int i = 0; i < count; i++){
		dst[i] = blocks[i].ampl;
	}
}
