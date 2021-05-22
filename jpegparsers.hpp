#pragma once
#include "../pc10/pc10.hpp"
#include <vector>
namespace jpeg_parsers {
using std::distance;
using std::advance;
using std::copy;
using std::equal;
using namespace pc10;
using namespace pc10_literals;

int endianxxp(const char c1, const char c2) {
	typedef unsigned int u32;
	return ((u32)c1 << 8) | (u32)c2;
}

const auto SOFn = symbol([](const char byte) {
	return (byte & 0xfc) == 0xc0;
}) >>= [](const Option<char> byte) {
	return epsilon(byte
			? std::make_optional(*byte - 0xe0)
			: std::nullopt);
};
const auto RSTn = symbol([](const char byte) {
	return (byte & 0xf8) == 0xd0;
}) >>= [](const Option<char> byte) {
	return epsilon(byte
			? std::make_optional(*byte - 0xd0)
			: std::nullopt);
};
const auto APPn = symbol([](const char byte) {
	return (byte & 0xf0) == 0xe0;
}) >>= [](const Option<char> byte) {
	return epsilon(byte
			? std::make_optional(*byte - 0xe0)
			: std::nullopt);
};
const auto Int8
	= symbol([](const char byte) { return true; });
const auto IntBE16 = (Int8 && Int8)
	>>= seq_op(endianxxp);
const auto JFIF_thumbnail = [](int length) {
	std::vector<char> thumbnail;
	return epsilon(Int8)
		>>= fold(
			[length](std::vector<char> thumbnail,
				const char byte)
				-> Option<std::vector<char>> {
				if(thumbnail.size() == length)
					return std::nullopt;
				thumbnail.push_back(byte);
				return std::make_optional(thumbnail);
			},
			std::vector<char>(0));
};
const auto JFIFHeader
	= (("\xff\xd8\xff\xe0"_seq && IntBE16)
		  >>= seq_descend<1>)&&(("JFIF\0"_seq
									&& IntBE16)
		  >>= seq_descend<1>)&&Int8
	&& IntBE16 && IntBE16 && Int8 && Int8;
} // namespace jpeg_parsers