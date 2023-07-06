#pragma once

#include <wx/bitmap.h>

#include <filesystem>

#include "lru.hpp"

bool saveThumbnail(
	const std::filesystem::path& src, const std::filesystem::path& dest,
	const int MAX_DIM);

bool isImage(const std::filesystem::path& file);

class ImagePool {
	std::vector<std::filesystem::path> paths;
	std::vector<wxBitmap> bitmaps;
	LRU<int, unsigned long long> lru;

	void load(int index);
	void unload(int index);

   public:
	ImagePool();
	bool addImage(const std::filesystem::path& filepath);
	const wxSize size(int index);
	const wxBitmap& bitmap(int index);
};
