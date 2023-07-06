#include "image_utils.hpp"

#include <webp/decode.h>
#include <webp/encode.h>
#include <wx/bitmap.h>

#include <cstring>
#include <filesystem>
#include <fstream>

#include "archive.hpp"
#include "util.hpp"

bool save(const std::filesystem::path& file, const wxImage& img) {
	if (file.extension() != ".webp") { return img.SaveFile(file.string()); }
	uint8_t* bytes;
	auto size = WebPEncodeLosslessRGB(
		img.GetData(), img.GetWidth(), img.GetHeight(), img.GetWidth() * 3,
		&bytes);
	if (size == 0) { return false; }
	std::basic_ofstream<uint8_t, std::char_traits<uint8_t>> output(
		file, std::ios::binary);
	output.write(bytes, size);
	WebPFree(bytes);
	output.close();
	return true;
}

wxImage load(const std::filesystem::path& file) {
	if (file.extension() != ".webp") {
		return wxImage(file.string(), wxBITMAP_TYPE_ANY);
	}
	std::basic_ifstream<uint8_t, std::char_traits<uint8_t>> input(
		file, std::ios::binary);
	std::vector<uint8_t> bytes(
		(std::istreambuf_iterator<uint8_t>(input)),
		std::istreambuf_iterator<uint8_t>());
	input.close();

	int iw = 0;
	int ih = 0;
	auto pixels = WebPDecodeRGB(bytes.data(), bytes.size(), &iw, &ih);
	wxImage img(iw, ih);
	std::memcpy(img.GetData(), pixels, iw * ih * 3);
	WebPFree(pixels);
	return img;
}

bool saveThumbnail(
	const std::filesystem::path& src, const std::filesystem::path& dest,
	const int MAX_DIM) {
	auto img = load(src);

	int W = MAX_DIM, H = MAX_DIM;
	if ((std::max)(img.GetWidth(), img.GetHeight()) < MAX_DIM) {
		if (src != dest) {
			return std::filesystem::copy_file(
				src, dest, std::filesystem::copy_options::overwrite_existing);
		}
		return true;
	} else if (img.GetWidth() > img.GetHeight()) {
		H = (img.GetHeight() * MAX_DIM) / img.GetWidth();
	} else {
		W = (img.GetWidth() * MAX_DIM) / img.GetHeight();
	}
	return save(dest, img.Rescale(W, H, wxIMAGE_QUALITY_HIGH));
}

bool isImage(const std::filesystem::path& file) {
	const auto& ext = file.extension();
	return ext == ".webp" || ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
		   ext == ".gif";
}

ImagePool::ImagePool() : lru(1024 * 1024 * 200, 3) {  // Limit to ~200 MB
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
	if (!bitmaps[index].IsOk()) return;
	bitmaps[index].UnRef();
}

const wxSize ImagePool::size(int index) {
	load(index);
	return bitmap(index).GetSize();
}

const wxBitmap& ImagePool::bitmap(int index) {
	load(index);
	return bitmaps[index];
}
