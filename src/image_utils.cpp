#include "image_utils.hpp"

#include <FreeImagePlus.h>
#include <wx/bitmap.h>
#include <wx/rawbmp.h>

#include "util.hpp"

bool saveThumbnail(
	const std::filesystem::path& src, const std::filesystem::path& dest,
	const unsigned int MAX_DIM) {
	fipImage img;
	if (!img.load(src.string().c_str())) { return false; }

	int W = MAX_DIM, H = MAX_DIM;
	if ((std::max)(img.getWidth(), img.getHeight()) < MAX_DIM) {
		if (src != dest) {
			return std::filesystem::copy_file(
				src, dest, std::filesystem::copy_options::overwrite_existing);
		}
		return true;
	} else if (img.getWidth() > img.getHeight()) {
		H = (img.getHeight() * MAX_DIM) / img.getWidth();
	} else {
		W = (img.getWidth() * MAX_DIM) / img.getHeight();
	}

	if (!img.rescale(W, H, FREE_IMAGE_FILTER::FILTER_LANCZOS3)) {
		return false;
	};

	return img.save(dest.string().c_str());
}

bool isImage(const std::filesystem::path& file) {
	return fipImage::identifyFIF(file.string().c_str()) != FIF_UNKNOWN;
}

bool ImagePool::addImage(const std::filesystem::path& filepath) {
	paths.emplace_back(filepath);
	bitmaps.emplace_back();
	return true;
}

void ImagePool::load(int index) {
	if (bitmaps[index].IsOk()) return;
	fipImage img;
	img.load(paths[index].string().c_str());

	const auto& iw = img.getWidth();
	const auto& ih = img.getHeight();

	auto& bmp = bitmaps[index];
	bmp.Create(iw, ih, 24);
	wxNativePixelData data(bmp);
	wxNativePixelData::Iterator p(data);
	RGBQUAD color;
	for (auto y = 0u; y < ih; ++y) {
		wxNativePixelData::Iterator rowStart = p;
		for (auto x = 0u; x < iw; ++x, ++p) {
			img.getPixelColor(x, ih - y, &color);
			p.Red() = color.rgbRed;
			p.Green() = color.rgbGreen;
			p.Blue() = color.rgbBlue;
		}
		p = rowStart;
		p.OffsetY(data, 1);
	}
}

const wxSize ImagePool::size(int index) {
	load(index);
	return bitmap(index).GetSize();
}

const wxBitmap& ImagePool::bitmap(int index) {
	load(index);
	return bitmaps[index];
}
