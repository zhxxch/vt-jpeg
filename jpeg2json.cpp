#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include "../pc10/pc10.hpp"
#include "json.hpp"
#include "jpegparsers.hpp"
using namespace pc10_literals;
using namespace jpeg_parsers;
using json = nlohmann::json;
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
	const auto header = JFIFHeader(JpegBitBuffer);
	std::cout << tuple_descend<0>(header).value_or(json::object()) << std::endl;
	return 0;
}