#pragma once
#include"assertexw.h"
inline int subbit(const int input,
	const int start, const int len){
	return((input&(((1 << len) - 1) << start)) >> start);
}
int write_code(
	const int codeword, const int codebitlen,
	const int bitoffset, void* buffer){
	for(int write_len = 0; write_len < codebitlen;){
		const int write_offset
			= write_len + bitoffset;
		const int offset_in_byte = write_offset & 7;
		const int byte_offset = write_offset / 8;
		unsigned char *last_byte
			= (unsigned char*)buffer + byte_offset;
		const int byte_w_len = 8 - offset_in_byte;
		*last_byte |= ((unsigned char)subbit(codeword, codebitlen
			- write_len - byte_w_len, byte_w_len)
			<< (8 - offset_in_byte - byte_w_len));
	}
	return bitoffset + codebitlen;
}
int read_code(
	const int readbitlen, int* ffbyte_end,
	const int bitoffset, const void* buffer){
	const int byte_offset = bitoffset / 8;
	const int offset_in_byte = bitoffset & 7;
	typedef unsigned char u8;
	typedef unsigned int uint;
	const u8 *last_byte
		= (const u8*)buffer + byte_offset;
	const uint byte0 = *last_byte;
	const u8* byte1 = last_byte
		+ (byte0 == 0xff ? 2 : 1);
	const u8* byte2 = byte1
		+ (*byte1 == 0xff ? 2 : 1);
	const uint bits = (byte0 << 16)
		| ((uint)*byte1 << 8) | ((uint)*byte2);
	*ffbyte_end = (byte0 == 0xff ? 2 : 0);
	*ffbyte_end |= (*byte1 == 0xff ? 4 : 0);
	const int frac_len = 3 * 8
		- offset_in_byte - readbitlen;
	const int res = (bits >> frac_len)
		&((1 << readbitlen) - 1);
	return res;
}
int code_store_len(const int ffskip_mask,
	const int read_bit_offset, const int code_len){
	const int offset_in_byte
		= read_bit_offset & 0x7;
	int store_len = code_len;
	for(int i = 2, rest_code_len
		= code_len - (8 - offset_in_byte);
		rest_code_len >= 0; i = (i << 1)){
		store_len += (ffskip_mask&i ? 8 : 0);
		rest_code_len -= 8;
	}
	return store_len;
}
int recover_DHT_table(
	int codes[16], int bytes_offset[16],
	const unsigned char dht[]){
	codes[0] = 0;
	bytes_offset[0] = 0;
	int total_codes = 0;
	for(int n = 0; n < 15; n++){
		codes[n + 1] = codes[n];
		if(dht[n] > 0){
			codes[n + 1] = codes[n + 1]
				+ ((int)dht[n] << (15 - n));
			total_codes += dht[n];
		}
		bytes_offset[n + 1] = total_codes;
	}
	total_codes += dht[15];
	return total_codes;
}
struct huffman_unit{
	int code;
	int codebitlen;
	int byte;
	int ampl;
};
typedef struct huffman_unit huffman_unit_block[64];
struct huffman_table{
	int codes[16];
	int byte_offset[16];
	const unsigned char* bytes;
	const unsigned char* code_counts;
};
union dht_slots{
	struct{
		struct huffman_table DC[4], AC[4];
	}tables;
	struct huffman_table vec[8];
};
int huffman_table_idx(const int selector){
	return (selector & 3) | (selector >> 2);
}
const char* huffman_table_names(const int idx){
	const char *names[] = {
		"DC0","DC1","DC2","DC3",
		"AC0","AC1","AC2","AC3",};
	return names[idx];
}
int read_hcode_bitlen(
	const int code, const int code_table[16]){
	for(int i = 16; i--;){
		if(code >= code_table[i])return(i + 1);
	}
	return 0;
}
int read_hcode_byte(
	const int code, const int codebitlen,
	const struct huffman_table table){
	const int hcode = code & (((1 << codebitlen)
		- 1) << (16 - codebitlen));
	const int byte_idx = (hcode - table.codes
		[codebitlen - 1]) >> (16 - codebitlen);
	if(table.code_counts[codebitlen - 1]
		<=byte_idx){
		return -1;
	}
	return table.bytes[byte_idx
		+ table.byte_offset[codebitlen - 1]];
}
union dht_slots update_dht(
	const unsigned char dht_data[],
	const int dht_len,
	union dht_slots prev_dhts){
	typedef unsigned char u8;
	for(int fin_len = 0; fin_len < dht_len;){
		const u8 *dht_part = dht_data + fin_len;
		struct huffman_table *dst
			= &prev_dhts.vec[
				huffman_table_idx(dht_part[0])];
		const int num_codes = recover_DHT_table(
			dst->codes, dst->byte_offset,
			dht_part + 1);
		dst->code_counts = dht_part + 1;
		dst->bytes = dht_part + 1 + 16;
		fin_len += 1 + 16 + num_codes;
	}
	return prev_dhts;
}
