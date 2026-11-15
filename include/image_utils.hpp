#pragma once

#include <wx/bitmap.h>
#include <wx/graphics.h>

#include <filesystem>
#include <vector>

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

	friend class PageViewer;

   public:
	ImagePool();
	bool addImage(const std::filesystem::path& filepath);
	wxSize size(int index);
	int length() { return paths.size(); }
	const wxBitmap& bitmap(int index);
	auto empty() const { return paths.empty() || bitmaps.empty(); }
	void clear();
};

enum class PageView : std::uint8_t {
	SinglePage,
	DoublePageLR,
	DoublePageOddLR,
	DoublePageRL,
	DoublePageOddRL,
};

using SIZE_T = std::vector<std::filesystem::path>::size_type;

class PageViewer {
	ImagePool pool;
	std::vector<wxSize> sizes;
	std::vector<std::vector<SIZE_T>> mapping;
	PageView view = PageView::SinglePage;

   public:
	PageViewer() { update(PageView::SinglePage); };
	bool addImage(const std::filesystem::path& filepath) {
		return pool.addImage(filepath);
	}
	auto empty() const { return pool.empty(); }
	void clear() { pool.clear(); }
	const auto& pageView() { return view; }
	int length() { return mapping.size(); }

	void draw(wxGraphicsContext& gc, int index);
	wxSize size(int index);
	void update(PageView view);
};
