#pragma once
#include"markers.h"
#include"jpg-fmt.h"
inline int SOF_header(
	const unsigned char marker_content[],
	struct jpeg_frame_header *header){
	header->P_depth = marker_content[0];
	header->Y_numline = endianxxp(
		marker_content + 1);
	header->X_numpx = endianxxp(marker_content + 3);
	header->Nf_numch = marker_content[5];
	header->Hmax = 0;
	header->Vmax = 0;
	const unsigned char *components
		= marker_content + 6;
	for(int i = 0; i < header->Nf_numch; i++){
		const int C_id = components[3 * i];
		const int C_i = C_id - 1;
		header->channels[C_i].C_id
			= components[3 * i];
		header->channels[C_i].Q.TableQ
			= components[3 * i + 2];
		const int sample_factor
			= components[3 * i + 1];
		header->channels[C_i].Q.H
			= sample_factor >> 4;
		header->channels[C_i].Q.V
			= sample_factor & 0xf;
		header->Hmax = (header->channels[C_i].Q.H
	> header->Hmax) ? header->channels[C_i].Q.H
			: header->Hmax;
		header->Vmax = (header->channels[C_i].Q.V
	> header->Vmax) ? header->channels[C_i].Q.V
			: header->Vmax;
	}
	const int mcu_x = 8 * ((header->Nf_numch == 1)
		? 1 : (header->Hmax));
	const int mcu_y = 8 * ((header->Nf_numch == 1)
		? 1 : (header->Vmax));
	header->Xmcus = (header->X_numpx
		+ mcu_x - 1) / mcu_x;
	header->Ymcus = (header->Y_numline
		+ mcu_y - 1) / mcu_y;
	for(int i = 0, block_it = 0;
		i < header->Nf_numch; i++){
		header->ch_num_block[i]
			= header->Xmcus*header->channels[i].Q.H
			*header->Ymcus*header->channels[i].Q.V;
		header->ch_block_offset[i] = block_it;
		block_it += header->ch_num_block[i];
	}
	return 15;
}
void interleav_transpose_frame(
	const huffman_unit_block collect[],
	const JPEG_SOS_t scan, const JPEG_SOF_t frame,
	huffman_unit_block output[]){
	if(scan.Ns_numch == 1){
		union channel_sw scan_dims
			= scan.sample_dims[0];
		scan_dims.S.H = 1;
		scan_dims.S.V = 1;
		const int x_num_block_seq
			= frame.Xmcus*frame.channels[
				scan.channels[0].C_id - 1].Q.H;
		const int transposed_offset
			= frame.ch_block_offset[
				scan.channels[0].C_id - 1];
		interleav_transpose_channel(
			collect, scan_dims, x_num_block_seq,
			output + transposed_offset);
		return;
	}
	for(int i = 0, block_ilv_it = 0;
		i < scan.Ns_numch; i++){
		block_ilv_it
			+= interleav_transpose_channel(
				collect + block_ilv_it,
				scan.sample_dims[i],
				frame.Xmcus*frame.channels[
					scan.channels[i].C_id - 1].Q.H,
				output + frame.ch_block_offset[
					scan.channels[i].C_id - 1]);
	}
}
inline int update_tables(
	const unsigned char byte[],
	union dht_slots *htables,
	union dqt_slots *qtables,
	JPEG_DRI_t *Ri){
	int fin_size = 0;
	for(int content_size = 0;
		byte[fin_size + 1] != JpegMarkers.SOS
		&& marker_SOFn(byte[fin_size + 1]) < 0;
		fin_size += content_size + 2){
		content_size
			= marker_length(byte + fin_size);
		if(byte[fin_size + 1] == JpegMarkers.DHT
			&& htables != NULL){
			*htables = update_dht(
				byte + fin_size + 4,
				content_size - 2, *htables);
		} else if(byte[fin_size + 1]
			== JpegMarkers.DQT && qtables != NULL){
			*qtables = update_dqt(
				byte + fin_size + 4,
				content_size - 2, *qtables);
		} else if(byte[fin_size + 1]
			== JpegMarkers.DRI&&Ri != NULL){
			*Ri = DRI_value(byte + fin_size + 4);
		}
	}
	return fin_size;
}
int frame_stat(
	const unsigned char FrameBytes[],
	const int FrameSize, const JPEG_SOF_t Frame,
	JPEG_DRI_t *Ri, union dht_slots* HTables,
	JPEG_SOS_t scans[],
	huffman_unit_block interleaved_stream[],
	huffman_unit_block collected_stream[],
	huffman_unit_block *approx_streams[],
	huffman_unit_block transposed_stream[]){
	int pre_scan_offset = 0;
	for(int scan_i = 0, scan_bitlen = 0, EOI = 0;
		!EOI; scan_i++){
		const int scan_header_offset
			= pre_scan_offset + update_tables(
				FrameBytes + pre_scan_offset,
				HTables, NULL, Ri);
		const int mcus_offset = scan_header_offset
			+ 4 + SOS_header(FrameBytes
				+ scan_header_offset + 4,
				Frame, scans + scan_i);
		print_scan_header(scans[scan_i]);
		const int FrameNumBlock
			= Frame.ch_block_offset[2]
			+ Frame.ch_num_block[2];
		memset(interleaved_stream, 0,
			sizeof(huffman_unit_block)
			* FrameNumBlock);
		memset(collected_stream, 0,
			sizeof(huffman_unit_block)
			* FrameNumBlock);
		if(scans[scan_i].Ah > 0
			&& scans[scan_i].Ns_numch == 1
			&& scans[scan_i].Ss > 0){
			scan_bitlen = scan_stat_ac_approx(
				FrameBytes + mcus_offset,
				FrameSize - mcus_offset, *Ri,
				*HTables, scans[scan_i],
				Frame, transposed_stream,
				interleaved_stream);
			interleav_transpose_frame(
				interleaved_stream,
				scans[scan_i], Frame,
				approx_streams[scan_i]);
			coeffs_add_transposed(FrameNumBlock,
				approx_streams[scan_i],
				transposed_stream);
		} else{
			scan_bitlen = scan_stat(
				FrameBytes + mcus_offset,
				FrameSize - mcus_offset, *Ri,
				*HTables, scans[scan_i],
				interleaved_stream);
			interleav_collect(
				interleaved_stream, scans[scan_i],
				collected_stream);
			if(scans[scan_i].Ah > 0){
				interleav_transpose_frame(
					collected_stream,
					scans[scan_i], Frame,
					approx_streams[scan_i]);
				coeffs_add_transposed(
					FrameNumBlock,
					approx_streams[scan_i],
					transposed_stream);
			} else{
				dc_ampl_accumul_collect(
					scans[scan_i], *Ri,
					collected_stream);
				interleav_transpose_frame(
					collected_stream, scans[scan_i],
					Frame, approx_streams[scan_i]);
				coeffs_add_transposed(
					FrameNumBlock,
					approx_streams[scan_i],
					transposed_stream);
			}
		}
		const int post_scan_offset = mcus_offset
			+ (scan_bitlen + 7) / 8;
		const int post_byte0
			= FrameBytes[post_scan_offset];
		const int post_bytem1
			= FrameBytes[post_scan_offset - 1];
		pre_scan_offset = post_scan_offset;
		if(post_bytem1 == 0xff && post_byte0 == 0){
			pre_scan_offset = post_scan_offset + 1;
		}
		const int post_byte1
			= FrameBytes[pre_scan_offset];
		const int post_byte2
			= FrameBytes[pre_scan_offset + 1];
		if(post_byte1 == 0xff
			&& post_byte2 == JpegMarkers.EOI){
			pre_scan_offset += 2;
			EOI = 1;
		}
	}
	return pre_scan_offset;
}
int down_sample_block_cutoff(
	const union channel_sw ch){
	return (ch.Q.H*ch.Q.V - 1) * 64 * 8;
}