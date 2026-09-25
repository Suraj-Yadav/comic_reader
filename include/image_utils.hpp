#pragma once

#include <wx/bitmap.h>

#include <filesystem>

#include "lru.hpp"

bool saveThumbnail(
	const std::filesystem::path& src, const std::filesystem::path& dest,
	int MAX_DIM);

bool isImage(const std::filesystem::path& file);

constexpr unsigned long long B = 1;
constexpr auto KB = 1024 * B;
constexpr auto MB = 1024 * KB;
constexpr auto GB = 1024 * MB;
constexpr auto TB = 1024 * GB;

class ImagePool {
	std::vector<std::filesystem::path> paths;
	std::vector<wxBitmap> bitmaps;
	LRU<int, unsigned long long> lru;

	void load(int index);
	void unload(int index);

   public:
	ImagePool();
	bool addImage(const std::filesystem::path& filepath);
	wxSize size(int index);
	const wxBitmap& bitmap(int index);
	auto empty() const { return paths.empty() || bitmaps.empty(); }
	void clear();
};
