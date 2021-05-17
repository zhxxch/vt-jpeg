#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include "../pc10/pc10.hpp"
#include "rapidjson/rapidjson.h"
using namespace pc10_literals;
int main(const int ac, const char *av[]) {
	std::ios::sync_with_stdio(false);
	if(ac < 2) return -1;
	std::fstream JpegFile(
		av[1], std::ios::in | std::ios::binary);
	if(!JpegFile) return -1;
	std::vector<char> JpegVec(
		(std::istreambuf_iterator<char>(JpegFile)),
		std::istreambuf_iterator<char>());
	auto JpegBitBuffer = std::make_pair(
		pc10::bit_iter(JpegVec.cbegin()),
		pc10::bit_iter(JpegVec.cend()));
	return 0;
}