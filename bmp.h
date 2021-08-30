#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
struct bmp_header{
	int16_t type;
	uint16_t size_lo;
	uint16_t size_hi;
	uint16_t r1;
	uint16_t r2;
	uint16_t offset_lo;
	uint16_t offset_hi;
};
struct bmp_dib_header{
	int32_t header_size;
	int32_t width;
	int32_t height;
	int16_t num_plane;
	int16_t pxbitdepth;
	int32_t compression;
	int32_t img_size;
	int32_t hresol;
	int32_t vresol;
	int32_t num_color;
	int32_t num_vip_color;
	/*
	uint32_t R_mask;
	uint32_t G_mask;
	uint32_t B_mask;
	uint32_t A_mask;
	int32_t win_clr_sp;
	char unused[0x30];
	*/
};
int bmp_size(const struct bmp_header header){
	return((unsigned int)header.size_lo
		+ ((unsigned int)header.size_hi << 16));
}
int bmp_offset(const struct bmp_header header){
	return((unsigned int)header.offset_lo
		+ ((unsigned int)header.offset_hi << 16));
}
struct bmp_header bmp_make_header(
	const struct bmp_dib_header iheader){
	struct bmp_header header = {0};
	header.type = (int16_t)0x4d42;
	const unsigned int size
		= sizeof header + sizeof iheader
		+ iheader.img_size;
	const unsigned int offset
		= sizeof header + sizeof iheader;
	header.size_hi = (uint16_t)(size >> 16);
	header.size_lo = (uint16_t)(size & 0xffff);
	header.offset_lo = (uint16_t)offset;
	header.offset_hi = 0;
	return header;
}
struct bmp_dib_header bmp_make_dib_hdr(
	const int width, const int height){
	struct bmp_dib_header header = {0};
	header.header_size = sizeof header;
	header.width = width;
	header.height = -height;
	header.num_plane = 1;
	header.pxbitdepth = 32;
	header.compression = 0;
	header.img_size = width * height * 4;
	header.hresol = 2835;
	header.vresol = 2835;
	header.num_color = 0;
	header.num_vip_color = 0;
	/*
	header.R_mask = 0x00ff0000;
	header.G_mask = 0x0000ff00;
	header.B_mask = 0x000000ff;
	header.A_mask = 0xff000000;
	header.win_clr_sp = 0x57696e20;
	*/
	return header;
}
/* A=[0,255],[K=100%,k=%0],[100%transparent,100%solid] */
uint32_t bmp_rgba(
	const unsigned int R, const unsigned int G,
	const unsigned int B, const unsigned int A){
	return((R << 16) | (G << 8) | B | (A << 24));
}
int bmp_write_channels(
	const uint8_t r[], const uint8_t g[],
	const uint8_t b[], const uint8_t a[],
	const int width, const int height,
	const char filename[]){
	FILE* bmp_fp = fopen(filename, "wb+");
	if(bmp_fp == NULL){
		return -1;
	}
	const struct bmp_dib_header hdr2
		= bmp_make_dib_hdr(width, height);
	const struct bmp_header hdr1
		= bmp_make_header(hdr2);
	uint32_t *rgba = (uint32_t*)malloc(4 * width*height);
	if(rgba == NULL){
		return -1;
	}
	for(int i = 0; i < width*height; i++){
		rgba[i] = bmp_rgba(r[i], g[i], b[i], a[i]);
	}
	if(sizeof hdr1
		!= fwrite(&hdr1, 1, sizeof hdr1, bmp_fp)){
		return -1;
	}
	if(sizeof hdr2
		!= fwrite(&hdr2, 1, sizeof hdr2, bmp_fp)){
		return -1;
	}
	if(hdr2.img_size
		!= fwrite(rgba, 1, hdr2.img_size, bmp_fp)){
		return -1;
	}
	free(rgba);
	return fclose(bmp_fp);
}
int bmp_write_px(const uint8_t rgba[], const int width,
	const int height, const char filename[]){
	FILE* bmp_fp = fopen(filename, "wb+");
	if(bmp_fp == NULL){
		return -1;
	}
	const struct bmp_dib_header hdr2
		= bmp_make_dib_hdr(width, height);
	const struct bmp_header hdr1
		= bmp_make_header(hdr2);

	if(sizeof hdr1
		!= fwrite(&hdr1, 1, sizeof hdr1, bmp_fp)){
		return -1;
	}
	if(sizeof hdr2
		!= fwrite(&hdr2, 1, sizeof hdr2, bmp_fp)){
		return -1;
	}
	if(hdr2.img_size
		!= fwrite(rgba, 1, hdr2.img_size, bmp_fp)){
		return -1;
	}
	return fclose(bmp_fp);
}
