#pragma once
#include<stdio.h>
#include"markers.h"
#include"DQTs.h"
int print16b(const int input){
	for(int i = 0; i < 16; i++){
		printf("%i", (input >> (15 - i)) & 1);
	}
	return 16;
}
const char mat_ltx_header[]
= "https://wp.com/latex.php?bg=fff&"
"latex=\\left(\\begin{matrix}";
const char mat_ltx_footer[]
= "\\end{matrix}\\right)";
const char mat_ltx_amp[] = "%26";
const char mat_ltx_arr[] = "\\\\";
int print_jfif(const struct JFIF_header jfif_hdr){
	wprintf(L"——JFIF标头：\n"
		L"版本\t缩略宽\t缩略高"
		L"\t水平\t垂直\t单位（分辨率）\n");
	return wprintf(L"%i.%i\t%i\t%i\t%i\t%i\t%i\n",
		jfif_hdr.version>>8,
		jfif_hdr.version&0xff,
		jfif_hdr.Xthumb,jfif_hdr.Ythumb,
		jfif_hdr.Xdensity,jfif_hdr.Ydensity,
		jfif_hdr.unit);
}
const wchar_t *channel_name(const int C_id){
	const wchar_t *names[] = {
		L"",
		L"明度1",
		L"蓝色2",
		L"红色3",
	};
	return names[C_id];
}
int print_frame_header(
	const struct jpeg_frame_header frame_hdr,
	const union dqt_slots q_tables){
	float q_cutoff[3], HV_cutoff[3],
		mcu_num_block[3], ttl_cut = 0;
	for(int i = 0; i < frame_hdr.Nf_numch; i++){
		q_cutoff[i] = quant_block_cutoff_bits(
			q_tables.vec[frame_hdr.channels[i]
			.Q.TableQ].scan) / 64;
		mcu_num_block[i] = (float)(
			frame_hdr.Hmax*frame_hdr.Vmax
				/ (frame_hdr.channels[i].Q.H
					*frame_hdr.channels[i].Q.V));
		HV_cutoff[i] = (mcu_num_block[i] - 1)
			* 8.f / mcu_num_block[i];
		ttl_cut += q_cutoff[i] / mcu_num_block[i]
			+ HV_cutoff[i];
	}
	const int bitrate
		= frame_hdr.P_depth*frame_hdr.Nf_numch;
	wprintf(L"——帧标头：\n"
		L"位深\t通道\t宽\t高\t码率\t截断（bit/px）\n");
	wprintf(L"%i\t%i\t%i\t%i\t%i\t%.1f（%.1f%%）\n",
		frame_hdr.P_depth, frame_hdr.Nf_numch,
		frame_hdr.X_numpx, frame_hdr.Y_numline,
		bitrate, ttl_cut, 100 * ttl_cut / bitrate);
	wprintf(L"——帧通道：\n"
		L"通道C\t量化表Q\t采样高H\t采样宽V\t量化位\t采样位\t截断总计\n");
	for(int i = 0; i < frame_hdr.Nf_numch; i++){
		wprintf(L"%s\t%i\t%i\t%i\t%.1f\t%.1f\t%.1f\n",
			channel_name(
				frame_hdr.channels[i].Q.C_id),
			frame_hdr.channels[i].Q.TableQ,
			frame_hdr.channels[i].Q.H,
			frame_hdr.channels[i].Q.V,
			q_cutoff[i], HV_cutoff[i],
			q_cutoff[i] / mcu_num_block[i]
			+ HV_cutoff[i]);
	}
	return 3;
}
const unsigned int StandardDHTs[4][8] = {
{0xde9d7cc6, 0x12b16f4c, 0x667d7ac2, 0x178f3870,
0xb55eccdf, 0x5a958b0d, 0x3f034624, 0xaf4c73d5,
},
{0x7c70ec7b, 0xdb8178c5, 0x182a2812, 0xbdbcbc80,
0xdaef5081, 0x2c014d70, 0xcee1313e, 0x753bbaf1,
},
{0x37be8da5, 0x8507976, 0xe18e1756, 0xfb01ced1,
0xf5b5520b, 0x83582385, 0xfaf7d1af, 0x3c0b0652,
},
{0x503bd160, 0x8e5a28bb, 0x9c82ca26, 0xc72bc745,
0xccfd438d, 0x73a68453, 0x8c80be9a, 0x86e2620c,
}};
int print_scan_header(
	const struct jpeg_scan_header scan_hdr){
	wprintf(L"——扫描标头：\n" L"通道\t最低频\t最高频"
		L"\t最高位\t最低位\n");
	wprintf(L"%i\t%i\t%i\t%i\t%i\n",
		scan_hdr.Ns_numch,
		scan_hdr.Ss, scan_hdr.Se,
		scan_hdr.Ah, scan_hdr.Al);
	wprintf(L"——扫描通道：\n" L"通道\t直流码\t交流码\n");
	for(int i = 0; i < scan_hdr.Ns_numch; i++){
		wprintf(L"%s\t%i\t%i\n",
			channel_name(
				scan_hdr.channels[i].H.C_id),
			scan_hdr.channels[i].H.TableDC,
			scan_hdr.channels[i].H.TableAC);
	}
	return 3;
}

void print_block(const huffman_unit_block units){
	printf(mat_ltx_header);
	printf("_{%x}^{%x}%i", units[0].codebitlen,
		units[0].code, units[0].ampl);
	for(int i = 1; i < 64; i++){
		printf("%s_{%i}^{%x}%i", ((i & 7) == 0) ?
			mat_ltx_arr : mat_ltx_amp,
			units[i].codebitlen, units[i].code,
			units[i].ampl);
	}
	printf("%s\n", mat_ltx_footer);
}
void print_block_amplarr(const int q[64]){
	printf(mat_ltx_header);
	printf("%i", q[0]);
	for(int i = 1; i < 64; i++){
		printf("%s%i", ((i & 7) == 0) ?
			mat_ltx_arr : mat_ltx_amp,
			q[i]);
	}
	printf("%s\n", mat_ltx_footer);
}
