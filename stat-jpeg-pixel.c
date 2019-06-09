/*
光色分圆
vt-jpeg/stat-jpeg-pixel
*/
#include<wchar.h>
#include<locale.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include"DHTs.h"
#include"jpg-fmt.h"
#include"markers.h"
#include"MCUs.h"
#include"DQTs.h"
#include"SOSs.h"
#include"SOFs.h"
#include"zigzag.h"
#include"DCTs.h"
#include"RGB.h"
#include"bmp.h"
#include"UP.h"
#include"assertexw.h"
inline FILE *wfopen(
	const wchar_t *filename,
	const wchar_t *mode){
	return _wfopen(filename, mode);
}
int find_next_mark(const unsigned char* buffer,
	const int current, const int length){
	const unsigned char* next
		= (const unsigned char*)memchr(
			buffer + current + 1, 0xff, length);
	if(next == NULL){
		return length;
	}
	return (int)((next - buffer) - current);
}
int size_MB(const int n){
	return n * 1024 * 1024;
}
int file_size(FILE* fp){
	fseek(fp, 0, SEEK_END);
	const int sz = ftell(fp);
	rewind(fp);
	return sz;
}
int main(int argc, char *argv[]){
	setlocale(LC_ALL, "");
	if(argc < 3)return -1;
	const char *filepath = argv[1];
	FILE* JpgFp = fopen(filepath, "rb");
	const int JpgLen = file_size(JpgFp);
	unsigned char *JpgMem = malloc(JpgLen);
	if(fread(JpgMem, 1, JpgLen, JpgFp)
		!= JpgLen){
		wprintf(L"无法读取文件\n");
		return -1;
	}
	//TODO: sha256
	fclose(JpgFp);
	wprintf(L"文件大小：%i字节\n", JpgLen);
	int NumScans = count_SOS(JpgMem, JpgLen);
	struct jpeg_scan_header *scan_headers
		= calloc(sizeof(JPEG_SOS_t), NumScans);
	const int jfif_offset = 2;
	struct JFIF_header JFIF_info = {0};
	const int jfif_len
		= marker_length(JpgMem + jfif_offset);
	JFIF_APP0(JpgMem + jfif_offset + 4, &JFIF_info);
	print_jfif(JFIF_info);
	const int pre_frame_offset
		= jfif_offset + 2 + jfif_len;
	union dht_slots HuffmanTables = {0};
	union dqt_slots QTables = {0};
	JPEG_DRI_t Ri = 0;
	const int frame_offset = pre_frame_offset
		+ update_tables(JpgMem + pre_frame_offset,
			&HuffmanTables, &QTables, &Ri);
	struct jpeg_frame_header frame_info = {0};
	const int post_frame_offset = frame_offset + 4
		+ SOF_header(JpgMem + frame_offset + 4,
			&frame_info);
	print_frame_header(frame_info);
	const int NumBlock = mcu_ttl_num_block(
		frame_info.channels, frame_info.Nf_numch);
	const int NumMcu
		= frame_info.Xmcus*frame_info.Ymcus;
	huffman_unit_block *hstream_interleav
		= calloc(sizeof(huffman_unit_block), NumBlock*NumMcu * (3 + NumScans));
	huffman_unit_block *hstream_collect
		= hstream_interleav + NumBlock * NumMcu;
	huffman_unit_block *hstream_transpose
		= hstream_collect + NumBlock * NumMcu;
	huffman_unit_block **hstreams_approx
		= calloc(sizeof(void*), NumScans);
	for(int i = 0; i < NumScans; i++){
		hstreams_approx[i] = hstream_transpose
			+ (i + 1) * NumBlock*NumMcu;
	}
	int(*coeffs_array)[64] = calloc(
		sizeof(int[64]), NumBlock*NumMcu);
	float(*real_coeffs_array)[64] = calloc(
		sizeof(float[64]), NumBlock * NumMcu * 2);
	float(*scans_array)[64]
		= real_coeffs_array + NumBlock * NumMcu;
	const int channel_num_px = 8 * 8
		* frame_info.Xmcus*frame_info.Hmax
		*frame_info.Ymcus*frame_info.Vmax;
	unsigned char *rgba = calloc(sizeof(char[4]),
		channel_num_px);
	const int post_scans_offset = frame_stat(
		JpgMem + post_frame_offset,
		JpgLen - post_frame_offset,
		frame_info, &Ri, &HuffmanTables,
		scan_headers, hstream_interleav,
		hstream_collect, hstreams_approx,
		hstream_transpose);
	zz_to_scanline_vec(hstream_transpose,
		NumMcu*NumBlock, hstream_transpose);
	copy_ampl(
		*hstream_transpose, 64 * NumBlock*NumMcu,
		*coeffs_array);
	wprintf(L"反量化……\n");
	quant_inv_frame(coeffs_array, QTables,
		frame_info, coeffs_array);
	wprintf(L"离散余弦变换……\n");
	DCT_W_t DCT_inv_w;
	init_dctinv_table(DCT_inv_w);
	dct_inv(coeffs_array, real_coeffs_array,
		DCT_inv_w, NumBlock*NumMcu);
	wprintf(L"转换颜色空间YCbCr->RGB……\n");
	frame_block_to_yccscan(frame_info,
		real_coeffs_array, scans_array);
	frame_ycc_rgb_ycc(frame_info,
		scans_array[frame_info.ch_block_offset[0]],
		scans_array[frame_info.ch_block_offset[1]],
		scans_array[frame_info.ch_block_offset[2]],
		rgba, rgba + 1 * channel_num_px, rgba + 2 * channel_num_px);
	bmp_write_channels(
		rgba + 0 * channel_num_px,
		rgba + 1 * channel_num_px,
		rgba + 2 * channel_num_px,
		rgba + 3 * channel_num_px,
		frame_info.Xmcus*frame_info.Hmax * 8,
		frame_info.Ymcus*frame_info.Vmax * 8,
		argv[2]);
	if(argc > 2){
		wprintf(L"不确定性原理……");
		float(*entropy_up_img)[64] = calloc(
			sizeof(float[64]), NumBlock*NumMcu);
		entropy_up_ycc(coeffs_array, real_coeffs_array,
			NumBlock*NumMcu, entropy_up_img);
		frame_block_to_yccscan(frame_info, entropy_up_img, scans_array);
		frame_ycc_rgb_ycc(frame_info,
			scans_array[frame_info.ch_block_offset[0]],
			scans_array[frame_info.ch_block_offset[1]],
			scans_array[frame_info.ch_block_offset[2]],
			rgba, rgba + 1 * channel_num_px,
			rgba + 2 * channel_num_px);
		bmp_write_channels(
			rgba + 0 * channel_num_px,
			rgba + 1 * channel_num_px,
			rgba + 2 * channel_num_px,
			rgba + 3 * channel_num_px,
			frame_info.Xmcus*frame_info.Hmax * 8,
			frame_info.Ymcus*frame_info.Vmax * 8,
			argv[3]);
	}
	/*
		*/
	return(0);
}