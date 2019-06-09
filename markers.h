#pragma once
#include<string.h>
#include<wchar.h>
inline int endianxx(const int be){
	return 0xffff & (((be & 0xff) << 8)
		| ((be & 0xff00) >> 8));
}
inline int endianxxp(const unsigned char* ptr){
	typedef unsigned int u32;
	return ((u32)ptr[0] << 8) | (u32)ptr[1];
}
inline int marker_length(const unsigned char *ptr){
	return endianxxp(ptr + 2);
}

const static struct{
	unsigned char SOF0_Baseline, SOF1_BaselineEx,
		SOF2_Progressive, SOF3_Lossless,
		DHT, RSTm, SOI, EOI, SOS, DQT,
		DNL, DRI, APPn, JPGn, COM;
}JpegMarkers = {0xc0,0xc1,0xc2,0xc3,0xc4,
0xd0,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xe0,0xf0,0xfe};
inline const wchar_t* marker_name(
	const unsigned char byte){
	const wchar_t *names[] = {
		L"SOF0连续", L"SOF1扩展连续",
		L"SOF2渐进", L"SOF3无损",
		L"DHT编码表", L"RSTm复位",
		L"SOI图像起始", L"EOI图像结束",
		L"SOS扫描起始", L"DQT量化表",
		L"DNL行数", L"DRI复位间隔",
		L"APPn", L"JPGn", L"COM注释",
	};
	typedef unsigned char byte_t;
	const byte_t *markers
		= (const byte_t*)&JpegMarkers;
	for(int i = 0; i < sizeof JpegMarkers; i++){
		if(markers[i] == byte){
			return names[i];
		}
	}
	return L"未知标记";
}

inline int marker_RSTn(const unsigned char byte){
	if((byte & 0xf8) == 0xd0){
		return byte - 0xd0;
	} else return -1;
}
inline int marker_SOFn(const unsigned char byte){
	if((byte & 0xfc) == 0xc0){
		return byte - 0xc0;
	} else return -1;
}
inline int marker_APPn(const unsigned char byte){
	if((byte & 0xf0) == 0xe0){
		return byte - 0xe0;
	} else return -1;
}
struct JFIF_header{
	int version, unit;
	int Xdensity, Ydensity;
	int Xthumb, Ythumb;
	const unsigned char *thumb;
};
inline int JFIF_APP0(
	const unsigned char marker_content[],
	struct JFIF_header *JFIF){
	if(memcmp(marker_content, "JFIF", 5) == 0){
		JFIF->version = endianxxp(
			marker_content + 5);
		JFIF->unit = marker_content[7];
		JFIF->Xdensity = endianxxp(
			marker_content + 8);
		JFIF->Ydensity = endianxxp(
			marker_content + 10);
		JFIF->Xthumb = marker_content[12];
		JFIF->Ythumb = marker_content[13];
		JFIF->thumb = marker_content + 14;
		return 14;
	} else return -1;
}
union channel_sw{
	struct{
		int C_id, H, V, TableQ;
	}Q;
	struct{
		int C_id, TableDC, TableAC, TableQ;
	}H;
	struct{
		int x, H, V, y;
	}S;
	int C_id;
};
inline int channel_sw_idx(const int C_id,
	const union channel_sw sw[], const int num_ch){
	for(int i = 0; i < num_ch; i++){
		if(sw[i].C_id == C_id)return i;
	}
	return -1;
}

struct jpeg_frame_header{
	int P_depth, Nf_numch;
	int Y_numline, X_numpx;
	int Hmax, Vmax, Xmcus, Ymcus;
	int ch_block_offset[3];
	int ch_num_block[3];
	union channel_sw channels[3];
};
typedef struct jpeg_frame_header JPEG_SOF_t;
struct jpeg_scan_header{
	union channel_sw channels[3];
	union channel_sw sample_dims[3];
	int Ss, Se, Ah, Al;
	int Hmax, Vmax, Xmcus, Ymcus;
	int Ns_numch;
};
typedef struct jpeg_scan_header JPEG_SOS_t;

typedef unsigned int JPEG_DRI_t;
inline JPEG_DRI_t DRI_value(
	const unsigned char marker_content[]){
	return endianxxp(marker_content);
}
inline int count_SOS(const unsigned char bytes[],
	const int size){
	int counter = 0;
	for(int iter = 0; iter < size - 1; iter++){
		if(bytes[iter] == 0xff
			&& bytes[iter + 1] == JpegMarkers.SOS){
			counter++;
		}
	}
	return counter;
}

/*	markers：解析RST和定义同步间隔的标记；
*/