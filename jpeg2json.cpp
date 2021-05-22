#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include "../pc10/pc10.hpp"
#include "rapidjson/document.h"
#include "jpegparsers.hpp"
using namespace pc10_literals;
using namespace jpeg_parsers;
using rapidjson::Value;
int main(const int ac, const char *av[]) {
	std::ios::sync_with_stdio(false);
	if(ac < 2) return -1;
	std::fstream JpegFile(
		av[1], std::ios::in | std::ios::binary);
	if(!JpegFile) return -1;
	std::vector<char> JpegVec(
		(std::istreambuf_iterator<char>(JpegFile)),
		std::istreambuf_iterator<char>());
	auto JpegBitBuffer = range_t(
		pc10::bit_iter<const char *>(JpegVec.data()),
		pc10::bit_iter<const char *>(
			JpegVec.data() + JpegVec.size()));
	rapidjson::Document JsonDoc;
	Value JpegFileHeader;
	const auto JFIFHeader_mod = JFIFHeader>>=[](auto r){return epsilon(tuple_dfs(*r));};
	const auto header = JFIFHeader(JpegBitBuffer);
	const auto header2 = JFIFHeader_mod(JpegBitBuffer);
	std::cout << typeid(header2).name()
			  << std::endl;
	return 0;
}