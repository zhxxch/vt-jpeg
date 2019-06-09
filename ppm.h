#pragma once
#include<stdio.h>
#include<stdlib.h>
int write_pgm(const char gbytes[], const int width, const int height, const char filename[]){
	FILE* pgm_fp = fopen(filename, "wb+");
	if(pgm_fp == NULL){
		return -1;
	}
	if(fprintf(pgm_fp, "P5 %i %i 255 ", width, height) < 0){
		return -1;
	}
	if(width*height
		!= fwrite(gbytes, 1, width*height, pgm_fp)){
		return -1;
	}
	return fclose(pgm_fp);
}
int write_ppm(
	const char r[], const char g[], const char b[],
	const char filename[], const int width, const int height){
	FILE* ppm_fp = fopen(filename, "wb+");
	if(ppm_fp == NULL){
		return -1;
	}
	if(fprintf(ppm_fp, "P6 %i %i 255 ", width, height) < 0){
		return -1;
	}
	char *rgb = malloc(3 * width*height);
	if(rgb == NULL){
		return -1;
	}
	for(int i = 0; i < width*height; i++){
		rgb[3 * i + 0] = r[i];
		rgb[3 * i + 1] = g[i];
		rgb[3 * i + 2] = b[i];
	}
	if(3*width*height
		!= fwrite(rgb, 1, 3*width*height, ppm_fp)){
		return -1;
	}
	free(rgb);
	return fclose(ppm_fp);
}
