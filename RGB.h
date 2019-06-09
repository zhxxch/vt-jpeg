#pragma once
#include"markers.h"
#include<string.h>
#include<math.h>
float range_ctrl(const float lo, const float hi, const float x){
	const float lo_x = (x > hi ? hi : x);
	const float hi_x = (lo_x < lo ? lo : lo_x);
	return hi_x;
}
float YCC_R(
	const float Y, const float Cb, const float Cr){
	return(Y + 1.402f*(Cr - 128.f) + 0.f*Cb);
}
float YCC_G(
	const float Y, const float Cb, const float Cr){
	return(Y - 0.34414f*(Cb - 128.f)
		- 0.71414f*(Cr - 128.f));
}
float YCC_B(
	const float Y, const float Cb, const float Cr){
	return(Y + 1.772f*(Cb - 128.f) + 0.f*Cr);
}
float RGB_Y(
	const float R, const float G, const float B){
	return(0.299f*R + 0.587f*G + 0.114f*B);
}
float RGB_Cb(
	const float R, const float G, const float B){
	return(-0.1687f*R - 0.3313f*G + 0.5f*B + 128.f);
}
float RGB_Cr(
	const float R, const float G, const float B){
	return(0.5f*R - 0.4187f*G - 0.0813f*B + 128.f);
}
unsigned int rbg_rgba(const int rgb){
	return 0xff000000ul + rgb;
}
unsigned int g_rgba(const int grayscale){
	return(grayscale | (grayscale << 8)
		| (grayscale << 16) | 0xff000000ul);
}
void block_to_scans(
	const int x_num_block, const int y_num_block,
	const float real_coeffs[][64], float scans[]){
	for(int y = 0; y < y_num_block; y++){
		for(int x = 0; x < x_num_block; x++){
			for(int v = 0; v < 8; v++){
				memcpy(
					scans + y * x_num_block * 64
					+ v * x_num_block * 8 + x * 8,
					real_coeffs[y*x_num_block + x]
					+ v * 8,
					8 * sizeof(float));
			}
		}
	}
}
void frame_block_to_yccscan(const JPEG_SOF_t frame,
	const float real_coeffs[][64],
	float scans[][64]){
	for(int i = 0; i < frame.Nf_numch; i++){
		block_to_scans(
			frame.Xmcus*frame.channels[i].Q.H,
			frame.Ymcus*frame.channels[i].Q.V,
			real_coeffs + frame.ch_block_offset[i],
			scans[frame.ch_block_offset[i]]);
	}
}
void frame_ycc_rgb(const JPEG_SOF_t Frame,
	const float Y[], const float Cb[],
	const float Cr[], unsigned char R[],
	unsigned char G[], unsigned char B[]){
	typedef unsigned char u8;
	const int Yh
		= Frame.Hmax / Frame.channels[0].Q.H;
	const int Cbh
		= Frame.Hmax / Frame.channels[1].Q.H;
	const int Crh
		= Frame.Hmax / Frame.channels[2].Q.H;
	const int Yv
		= Frame.Vmax / Frame.channels[0].Q.V;
	const int Cbv
		= Frame.Vmax / Frame.channels[1].Q.V;
	const int Crv
		= Frame.Vmax / Frame.channels[2].Q.V;
	const int X_px = Frame.Xmcus*Frame.Hmax * 8;
	const int Y_px = Frame.Ymcus*Frame.Vmax * 8;
	for(int y = 0; y < Y_px; y++){
		for(int x = 0; x < X_px; x++){
			const float Yval = 128.f + Y[
				(y / Yv)*(X_px / Yh) + x / Yh];
			const float Cbval = 128.f + Cb[
				(y / Cbv)*(X_px / Cbh) + x / Cbh];
			const float Crval = 128.f + Cr[
				(y / Crv)*(X_px / Crh) + x / Crh];
			R[y*X_px + x] = (u8)range_ctrl(0, 255,
				YCC_R(Yval, Cbval, Crval));
			G[y*X_px + x] = (u8)range_ctrl(0, 255,
				YCC_G(Yval, Cbval, Crval));
			B[y*X_px + x] = (u8)range_ctrl(0, 255,
				YCC_B(Yval, Cbval, Crval));
		}
	}
}

void frame_ycc_rgb_ycc(const JPEG_SOF_t Frame,
	const float Y[], const float Cb[],
	const float Cr[], unsigned char R[],
	unsigned char G[], unsigned char B[]){
	typedef unsigned char u8;
	const int Yh
		= Frame.Hmax / Frame.channels[0].Q.H;
	const int Cbh
		= Frame.Hmax / Frame.channels[1].Q.H;
	const int Crh
		= Frame.Hmax / Frame.channels[2].Q.H;
	const int Yv
		= Frame.Vmax / Frame.channels[0].Q.V;
	const int Cbv
		= Frame.Vmax / Frame.channels[1].Q.V;
	const int Crv
		= Frame.Vmax / Frame.channels[2].Q.V;
	const int X_px = Frame.Xmcus*Frame.Hmax * 8;
	const int Y_px = Frame.Ymcus*Frame.Vmax * 8;
	for(int y = 0; y < Y_px; y++){
		for(int x = 0; x < X_px; x++){
			const float Yval = 128.f + Y[
				(y / Yv)*(X_px / Yh) + x / Yh];
			const float Cbval = 128.f + Cb[
				(y / Cbv)*(X_px / Cbh) + x / Cbh];
			const float Crval = 128.f + Cr[
				(y / Crv)*(X_px / Crh) + x / Crh];
			R[y*X_px + x] = (u8)range_ctrl(
				0, 255, Crval);
			G[y*X_px + x] = (u8)range_ctrl(
				0, 255, Yval);
			B[y*X_px + x] = (u8)range_ctrl(
				0, 255, Cbval);
		}
	}
}
