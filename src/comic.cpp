#include "comic.hpp"

#include <wx/settings.h>

#include <filesystem>

#include "archive.hpp"
#include "image_utils.hpp"
#include "util.hpp"

const int THUMB_DIM = (std::min)(
	wxSystemSettings::GetMetric(wxSYS_SCREEN_X),
	wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));

const std::filesystem::path cacheDirectory =
	std::filesystem::temp_directory_path() / "comicReaderCache";

std::filesystem::path getCoverPath(
	const std::filesystem::path& comic, const std::filesystem::path& img) {
	auto path = cacheDirectory / comic.stem();
	path += "_Cover";
	path += img.extension();
	return path;
}

Comic::Comic(const std::filesystem::path& comicPath)
	: comicPath(comicPath), size(0) {
	wxString cover;
	std::string extension;

	processArchiveFile(comicPath, [&](const ArchiveFile& file) {
		if (!file.isFile() || !isImage(file.path())) { return; }
		size++;
		auto fullPath = cacheDirectory / file.path();
		if (!cover.empty() && wxCmpNatural(cover, fullPath.string()) < 0) {
			return;
		}
		cover = fullPath.string();
		coverPage = getCoverPath(comicPath, fullPath);
		file.writeContent(coverPage);
	});
	saveThumbnail(coverPage, coverPage, THUMB_DIM);
}

int Comic::length() const { return size; };
std::string Comic::getName() const { return comicPath.stem().string(); };

void Comic::load(std::function<void(int i)> progress) {
	unload();
	const auto extractedCache = cacheDirectory / comicPath.stem();
	processArchiveFile(comicPath, [&](const ArchiveFile& file) {
		if (!file.isFile() || !isImage(file.path())) { return; }
		pages.push_back(extractedCache / file.path());
		file.writeContent(pages.back());
		if (progress) { progress(pages.size() - 1); }
	});
	std::sort(pages.begin(), pages.end(), [](const auto& a, const auto& b) {
		return wxCmpNatural(a.string(), b.string()) < 0;
	});
	size = pages.size();
}

void Comic::unload() {
	pages.clear();
	std::filesystem::remove_all(cacheDirectory / comicPath.stem());
}
