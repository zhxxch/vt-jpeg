#pragma once
#include"DHTs.h"
#include"markers.h"
#include<string.h>
#include"jpg-fmt.h"
inline int mcu_ttl_num_block(
	const union channel_sw dims[],
	const int num_ch){
	int num_block = 0;
	if(num_ch == 1)return 1;
	for(int i = 0; i < num_ch; i++){
		num_block += dims[i].S.H * dims[i].S.V;
	}
	return num_block;
}

int expand_sign(const int ampl, const int bitlen){
	const int ampl_lo = 1 << (bitlen - 1);
	const int ampl_hi = (ampl_lo << 1) - 1;
	return (ampl < ampl_lo) ?
		(ampl - ampl_hi) : ampl;
}
int mcu_dc_stat_init(
	const unsigned char scan[], const int bitoffset,
	const struct huffman_table DC_table,
	const int Al, struct huffman_unit DC_out[1]){
	int ffbyte_pos = 0;
	const int code = read_code(16,
		&ffbyte_pos, bitoffset, scan);
	if(code < 0)return code;
	DC_out->code = code;
	const int code_len = read_hcode_bitlen(
		code, DC_table.codes);
	DC_out->codebitlen = code_len;
	const int SSSS = read_hcode_byte(
		code, code_len, DC_table);
	DC_out->byte = SSSS;
	const int SSSS_len = code_store_len(
		ffbyte_pos, bitoffset, code_len);
	if(SSSS == 0){
		DC_out->ampl = 0;
		return SSSS_len;
	}
	const int DIFF_bits = read_code(SSSS,
		&ffbyte_pos, bitoffset + SSSS_len, scan);
	DC_out->ampl += (1 << Al)
		* expand_sign(DIFF_bits, SSSS);
	if(DIFF_bits < 0)return DIFF_bits;
	const int ampl_len = code_store_len(
		ffbyte_pos, bitoffset + SSSS_len, SSSS);
	return(SSSS_len + ampl_len);
}
int mcu_dc_stat_approx(
	const unsigned char scan[], const int bitoffset,
	const int Al, struct huffman_unit DC_unit[1]){
	int ffbyte_pos = 0;
	const int approx_bit = read_code(1,
		&ffbyte_pos, bitoffset, scan);
	if(approx_bit < 0)return approx_bit;
	const int fin_len = code_store_len(
		ffbyte_pos, bitoffset, 1);
	DC_unit->ampl += approx_bit * (1 << Al);
	DC_unit->codebitlen = 1;
	return fin_len;
}

int mcu_ac_stat_init(
	const unsigned char scan[], const int bitoffset,
	const struct huffman_table AC_table,
	const int Ss, const int Se,
	const int Al, int *EOB_counter,
	huffman_unit_block AC_out){
	if(*EOB_counter > 0){
		EOB_counter[0]--;
		return 0;
	}
	int finbitlen = 0;
	for(int AC_byte, output_iter = Ss,
		ffbyte_pos; output_iter <= Se;
		output_iter++){
		const int code = read_code(16, &ffbyte_pos,
			bitoffset + finbitlen, scan);
		if(code < 0){ return code; }
		const int code_len = read_hcode_bitlen(
			code, AC_table.codes);
		finbitlen += code_store_len(
			ffbyte_pos, bitoffset + finbitlen,
			code_len);
		AC_byte = read_hcode_byte(
			code, code_len, AC_table);
		if(AC_byte == 0){
			AC_out[output_iter].code = code;
			AC_out[output_iter].codebitlen
				= code_len;
			return finbitlen; }
		const int RRRR = (AC_byte & 0xf0) >> 4;
		const int SSSS = AC_byte & 0x0f;
		if(RRRR < 0xf && SSSS == 0){//End-of-Band
			const int run_bits = read_code(RRRR,
				&ffbyte_pos, bitoffset + finbitlen,
				scan);
			if(RRRR > 0 && run_bits < 0){
				return run_bits;
			}
			const int EOB_run_len
				= run_bits | (1 << RRRR);
			*EOB_counter = EOB_run_len - 1;
			finbitlen += code_store_len(ffbyte_pos,
				bitoffset + finbitlen, RRRR);
			AC_out[output_iter].code = code;
			AC_out[output_iter].codebitlen
				= code_len;
			AC_out[output_iter].byte = AC_byte;
			return finbitlen;
		}
		output_iter += RRRR;
		const int AC_bits = read_code(SSSS,
			&ffbyte_pos, bitoffset + finbitlen,
			scan);
		if(SSSS > 0 && AC_bits < 0)return AC_bits;
		AC_out[output_iter].code = code;
		AC_out[output_iter].codebitlen = code_len;
		AC_out[output_iter].byte = AC_byte;
		AC_out[output_iter].ampl += (1 << Al)
			* expand_sign(AC_bits, SSSS);
		finbitlen += code_store_len(ffbyte_pos,
			bitoffset + finbitlen, SSSS);
	}
	return finbitlen;
}
int mcu_ac_stat_approx(
	const unsigned char scan[], const int bitoffset,
	const struct huffman_table AC_table,
	const int Ss, const int Se,
	const int Al, int *zeros_counter,
	struct huffman_unit *new_ac_coeff,
	const huffman_unit_block AC_prev,
	huffman_unit_block AC_approx){
	const int bandwidth = Se - Ss + 1;
	int finbitlen = 0;
	for(int AC_byte = 0, f = Ss, ffbyte_pos = 0;
		f <= Se;){
		if(AC_prev[f].ampl != 0
			&& new_ac_coeff->codebitlen){
			const int correction_bit_len
				= mcu_dc_stat_approx(scan,
					bitoffset + finbitlen,
					Al, AC_approx + f);
			if(AC_prev[f].ampl < 0){
				AC_approx[f].ampl *= -1;
			}
			if(((new_ac_coeff->byte & 0xf) == 0)
				&& (new_ac_coeff->byte != 0xf0)){
				zeros_counter[0]--;
			}
			finbitlen += correction_bit_len;
			f++;
		} else if(*zeros_counter > 0){
			zeros_counter[0]--;
			f++;
		} else{
			if(new_ac_coeff->codebitlen){
				AC_approx[f] = *new_ac_coeff;
				f++;
				memset(new_ac_coeff, 0,
					sizeof *new_ac_coeff);
			}
			const int code = read_code(
				16, &ffbyte_pos,
				bitoffset + finbitlen, scan);
			if(code < 0){ return finbitlen; }
			const int code_len = read_hcode_bitlen(
				code, AC_table.codes);
			finbitlen += code_store_len(
				ffbyte_pos, bitoffset + finbitlen,
				code_len);
			AC_byte = read_hcode_byte(
				code, code_len, AC_table);
			const int RRRR = (AC_byte & 0xf0) >> 4;
			const int SSSS = AC_byte & 0x0f;
			if(RRRR < 0xf && SSSS == 0){
				//End-of-Band-n
				const int run_bits = read_code(
					RRRR, &ffbyte_pos,
					bitoffset + finbitlen, scan);
				if(RRRR >= 0 && run_bits < 0){
					return run_bits;
				}
				const int EOB_run_len
					= run_bits | (1 << RRRR);
				finbitlen += code_store_len(
					ffbyte_pos,
					bitoffset + finbitlen, RRRR);
				*zeros_counter
					= (EOB_run_len - 1) * bandwidth
					+ Se - f + 1;
				AC_approx[f].code = code;
				AC_approx[f].codebitlen = code_len;
				AC_approx[f].byte = AC_byte;
			} else{
				*zeros_counter = RRRR;
			}
			new_ac_coeff->code = code;
			new_ac_coeff->codebitlen = code_len;
			new_ac_coeff->byte = AC_byte;
			const int AC_bits = read_code(SSSS,
				&ffbyte_pos, bitoffset + finbitlen,
				scan);
			if(SSSS > 0 && AC_bits < 0){
				return AC_bits;
			}
			new_ac_coeff->ampl += (1 << Al)
				* expand_sign(AC_bits, SSSS);
			finbitlen += code_store_len(ffbyte_pos,
				bitoffset + finbitlen, SSSS);
		}
	}
	if(*zeros_counter == 0){
		memset(new_ac_coeff, 0,
			sizeof *new_ac_coeff);
	}
	return finbitlen;
}
int seq_mcu_stat(
	const unsigned char mcubytes[],
	const int readoffset, const int bufbitlen,
	const union dht_slots Tables,
	const struct jpeg_scan_header scan,
	struct huffman_unit MCU_output[][64]){
	int finbitlen = 0, block_iter = 0, eob_ctr = 0;
	for(int ch_i = 0; ch_i < scan.Ns_numch; ch_i++){
		for(int block_i = 0; (block_i
			< scan.sample_dims[ch_i].S.H
			* scan.sample_dims[ch_i].S.V)
			&& ((readoffset + finbitlen)
				< bufbitlen);
			block_i++){
			const int dc_len = mcu_dc_stat_init(
				mcubytes, readoffset + finbitlen,
				Tables.tables.DC[
					scan.channels[ch_i].H.TableDC],
				scan.Al, MCU_output[block_iter]);
			if(dc_len < 0)return dc_len;
			finbitlen += dc_len;
			const int ac_len = mcu_ac_stat_init(
				mcubytes, readoffset + finbitlen,
				Tables.tables.AC[
					scan.channels[ch_i].H.TableAC],
				1, 63, 0, &eob_ctr,
						MCU_output[block_iter]);
			if(ac_len < 0)return ac_len;
			block_iter++;
			finbitlen += ac_len;
		}
	}
	return finbitlen;
}
int pro_dc_mcu_stat(
	const unsigned char mcubytes[],
	const int readoffset, const int bufbitlen,
	const union dht_slots Tables,
	const struct jpeg_scan_header scan,
	huffman_unit_block MCU_output[]){
	int finbitlen = 0;
	for(int ch_i = 0, block_iter = 0;
		scan.Ah == 0 && ch_i < scan.Ns_numch;
		ch_i++){
		for(int block_i = 0; (block_i
			< scan.sample_dims[ch_i].S.H
			* scan.sample_dims[ch_i].S.V)
			&& ((readoffset + finbitlen)
				< bufbitlen);
			block_i++){
			const int dc_len = mcu_dc_stat_init(
				mcubytes, readoffset + finbitlen,
				Tables.tables.DC[
					scan.channels[ch_i].H.TableDC],
				scan.Al, MCU_output[block_iter]);
			if(dc_len < 0)return -finbitlen;
			finbitlen += dc_len;
			block_iter++;
		}
	}
	for(int ch_i = 0, block_iter = 0;
		scan.Ah > 0 && ch_i < scan.Ns_numch;
		ch_i++){
		for(int block_i = 0; (block_i
			< scan.sample_dims[ch_i].S.H
			* scan.sample_dims[ch_i].S.V)
			&& ((readoffset + finbitlen)
				< bufbitlen);
			block_i++){
			const int dc_len = mcu_dc_stat_approx(
				mcubytes, readoffset + finbitlen,
				scan.Al, MCU_output[block_iter]);
			if(dc_len < 0)return -finbitlen;
			finbitlen += dc_len;
			block_iter++;
		}
	}
	return finbitlen;
}
