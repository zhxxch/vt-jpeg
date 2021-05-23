#pragma once
#include "../pc10/pc10.hpp"
#include "json.hpp"
#include <vector>
namespace jpeg_parsers {
using std::distance;
using std::advance;
using std::copy;
using std::equal;
using namespace pc10;
using namespace pc10_literals;
using json = nlohmann::json;
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
	return epsilon(Int8)
		>>= fold(
			[length](std::vector<char> thumbnail,
				const char byte)
				-> Option<std::vector<char>> {
				if(thumbnail.size() >= length)
					return std::nullopt;
				thumbnail.push_back(byte);
				return std::make_optional(thumbnail);
			},
			std::vector<char>(0));
};
const auto JFIF_header_json =
	[](auto thumbnail_vec_res) {
		return [thumbnail_vec_res](auto t) {
			return json({{"length", std::get<0>(t)},
				{"version",
					{{"major", std::get<1>(t) >> 8},
						{"minor",
							std::get<1>(t) & 0xff}}},
				{"units", std::get<2>(t)},
				{"Xdensity",
					(unsigned char)std::get<3>(t)},
				{"Ydensity",
					(unsigned char)std::get<4>(t)},
				{"Xthumbnail",
					(unsigned char)std::get<5>(t)},
				{"Ythumbnail",
					(unsigned char)std::get<6>(t)},
				{"RGBn",
					(thumbnail_vec_res
							? json(*thumbnail_vec_res)
							: json::array())}});
		};
	};
const auto JFIF_fin = [](auto header_r) {
	const auto header_tuple = [](auto t) {
		return tuple_dfs(t);
	} * header_r;
	const int thumbnail_size = (header_tuple
			? (std::get<0>(*header_tuple) - 16)
			: 0);
	return JFIF_thumbnail(thumbnail_size) >>=
		[header_tuple](Option<std::vector<char>>
				thumbnail_vec_res) {
			return epsilon(
				JFIF_header_json(thumbnail_vec_res)
				* header_tuple);
		};
};
const auto JFIF_ext_fin = [](auto header_r) {
	const auto header_tuple = [](auto t) {
		return tuple_dfs(t);
	} * header_r;
	const int thumbnail_size = (header_tuple
			? (std::get<0>(*header_tuple) - 8)
			: 0);
	return JFIF_thumbnail(thumbnail_size) >>=
		[header_tuple](Option<std::vector<char>>
				thumbnail_vec_res) {
			return epsilon([thumbnail_vec_res](
							   auto t) {
				return json({{"length",
								 std::get<0>(t)},
					{"extension_code", std::get<1>(t)},
					{"extension_data",
						(thumbnail_vec_res
								? json(
									*thumbnail_vec_res)
								: json::array())}});
			} * header_tuple);
		};
};
const auto JFIFExtHeader
	= ((("\xff\xd8\xff\xe0"_seq && IntBE16)
		>>= seq_descend<1>)&&(("JFXX\0"_seq && IntBE16)
		>>= seq_descend<1>)&&Int8)
	>>= JFIF_ext_fin;
const auto JFIFHeader
	= ((("\xff\xd8\xff\xe0"_seq && IntBE16)
		   >>= seq_descend<1>)&&(("JFIF\0"_seq
									 && IntBE16)
		   >>= seq_descend<1>)&&Int8
		&& IntBE16 && IntBE16 && Int8 && Int8)
	>>= JFIF_fin;
} // namespace jpeg_parsers