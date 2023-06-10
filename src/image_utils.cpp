#include "image_utils.hpp"

#include <FreeImagePlus.h>

#include <filesystem>

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
