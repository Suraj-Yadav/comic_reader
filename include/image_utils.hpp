#pragma once

#include <wx/bitmap.h>

#include <filesystem>

bool saveThumbnail(
	const std::filesystem::path& src, const std::filesystem::path& dest,
	const int MAX_DIM);

bool isImage(const std::filesystem::path& file);

class ImagePool {
	std::vector<std::filesystem::path> paths;
	std::vector<wxBitmap> bitmaps;

	void load(int index);

   public:
	bool addImage(const std::filesystem::path& filepath);
	const wxSize size(int index);
	const wxBitmap& bitmap(int index);
};
