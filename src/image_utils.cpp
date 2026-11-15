#include "image_utils.hpp"

#include <webp/decode.h>
#include <webp/encode.h>
#include <wx/bitmap.h>
#include <wx/gdicmn.h>
#include <wx/graphics.h>

#include <filesystem>
#include <vector>
#include <vips/vips8>

bool save(const std::filesystem::path& file, const wxImage& img) {
	return img.SaveFile(file.string());
}

wxSize size(const std::filesystem::path& file) {
	auto img = vips::VImage::new_from_file(file.c_str());
	return {img.width(), img.height()};
}

wxImage load(const std::filesystem::path& file) {
	return {file.string(), wxBITMAP_TYPE_ANY};
}

bool saveThumbnail(
	const std::filesystem::path& src, const std::filesystem::path& dest,
	const int MAX_DIM) {
	if (!std::filesystem::exists(src)) { return false; }
	const auto img = vips::VImage::new_from_file(src.c_str());

	if ((std::max)(img.width(), img.height()) < MAX_DIM) {
		if (src != dest) {
			return std::filesystem::copy_file(
				src, dest, std::filesystem::copy_options::overwrite_existing);
		}
		return true;
	}
	auto thumb = vips::VImage::thumbnail(
		src.c_str(), MAX_DIM,
		vips::VImage::option()
			->set("height", MAX_DIM)
			->set("size", VIPS_SIZE_DOWN));

	thumb.write_to_file(dest.c_str());
	return true;
}

bool isImage(const std::filesystem::path& file) {
	const auto& ext = file.extension();
	return ext == ".webp" || ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
		   ext == ".gif";
}

constexpr auto MEMORY_LIMIT = 200 * MB;

ImagePool::ImagePool() : lru(MEMORY_LIMIT, 3) {	 // Limit to ~200 MB
	lru.addEvictionHook([this](int i) { unload(i); });
}

bool ImagePool::addImage(const std::filesystem::path& filepath) {
	paths.emplace_back(filepath);
	bitmaps.emplace_back();
	return true;
}

void ImagePool::load(int index) {
	if (!bitmaps[index].IsOk()) { bitmaps[index] = ::load(paths[index]); }
	const auto& s = bitmaps[index].GetSize();
	// Approx mem of an image
	const auto size = s.GetHeight() * s.GetWidth() * 3 * sizeof(unsigned char);
	lru.hit(index, size);
}

void ImagePool::unload(int index) {
	if (!bitmaps[index].IsOk()) { return; }
	bitmaps[index].UnRef();
}

wxSize ImagePool::size(int index) {
	load(index);
	return bitmap(index).GetSize();
}

const wxBitmap& ImagePool::bitmap(int index) {
	load(index);
	return bitmaps[index];
}

void ImagePool::clear() {
	int size = static_cast<int>(bitmaps.size());
	for (int i = 0; i < size; i++) { unload(i); }
	paths.clear();
	bitmaps.clear();
}

wxSize PageViewer::size(int index) { return sizes[index]; }

#include <wx/dcmemory.h>

inline wxBitmap joinHorizontal(const wxBitmap& left, const wxBitmap& right) {
	const int h = std::max(left.GetHeight(), right.GetHeight());
	wxBitmap out(left.GetWidth() + right.GetWidth(), h);
	{
		wxMemoryDC dc(out);
		dc.SetBackground(*wxBLACK_BRUSH);
		dc.Clear();
		dc.DrawBitmap(left, 0, (h - left.GetHeight()) / 2);
		dc.DrawBitmap(right, left.GetWidth(), (h - right.GetHeight()) / 2);
	}  // the dc must be destroyed before `out` is used
	return out;
}

void PageViewer::draw(wxGraphicsContext& gc, int index) {
	if (mapping[index].size() == 1) {
		auto i = mapping[index][0];
		auto sz = pool.size(i);
		gc.DrawBitmap(pool.bitmap(i), 0, 0, sz.GetWidth(), sz.GetHeight());
		return;
	}
	auto l = mapping[index][0];
	auto r = mapping[index][1];
	if (view == PageView::DoublePageRL || view == PageView::DoublePageOddRL) {
		std::swap(l, r);
	}
	auto szl = pool.size(l), szr = pool.size(r);
	auto h = std::max(szl.GetHeight(), szr.GetHeight());
	gc.DrawBitmap(
		pool.bitmap(l), 0, (h - szl.GetHeight()) / 2, szl.GetWidth(),
		szl.GetHeight());
	gc.DrawBitmap(
		pool.bitmap(r), szl.GetWidth() - 1, (h - szr.GetHeight()) / 2,
		szr.GetWidth(), szr.GetHeight());
}

void PageViewer::update(PageView v) {
	view = v;
	sizes.clear();
	mapping.clear();
	if (view == PageView::SinglePage) {
		for (auto i = 0u; i < pool.length(); ++i) {
			sizes.emplace_back(::size(pool.paths[i]));
			mapping.emplace_back(std::vector<SIZE_T>{i});
		}
	}
	auto i = 0u;
	if (view == PageView::DoublePageOddLR ||
		view == PageView::DoublePageOddRL) {
		mapping.emplace_back(std::vector<SIZE_T>{i});
		sizes.emplace_back(::size(pool.paths[i]));
		i++;
	}
	while ((i + 1) < pool.length()) {
		auto sz1 = ::size(pool.paths[i]);
		auto sz2 = ::size(pool.paths[i + 1]);
		auto wide1 = sz1.GetWidth() >= sz1.GetHeight();
		auto wide2 = sz2.GetWidth() >= sz2.GetHeight();
		if (wide1 || wide2) {
			mapping.emplace_back(std::vector<SIZE_T>{i});
			sizes.emplace_back(::size(pool.paths[i]));
			i++;
			if (wide1) { continue; }
		}
		if (wide2) {
			mapping.emplace_back(std::vector<SIZE_T>{i});
			sizes.emplace_back(::size(pool.paths[i]));
			i++;
			continue;
		}
		sizes.emplace_back(
			sz1.GetWidth() + sz2.GetWidth(),
			std::max(sz1.GetHeight(), sz2.GetHeight()));
		mapping.emplace_back(std::vector<SIZE_T>{i, i + 1});
		i += 2;
	}
	if (i < pool.length()) {
		mapping.emplace_back(std::vector<SIZE_T>{i});
		sizes.emplace_back(::size(pool.paths[i]));
		i++;
	}
}
