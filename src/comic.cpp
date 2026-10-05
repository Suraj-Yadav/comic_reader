#include "comic.hpp"

#include <wx/settings.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "archive.hpp"
#include "image_utils.hpp"
#include "network.hpp"
#include "util.hpp"

static std::atomic_int COMIC_COUNTER = 0;

int GET_THUMB_DIM() {
	const static int THUMB_DIM =
		(std::min)(wxSystemSettings::GetMetric(wxSYS_SCREEN_X),
				   wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));
	return THUMB_DIM;
}
const std::filesystem::path cacheDirectory =
	std::filesystem::temp_directory_path() / "comicReaderCache";

std::filesystem::path getCoverPath(
	std::string_view storage, const std::filesystem::path& img) {
	auto path = cacheDirectory / storage;
	path += "_Cover";
	path += img.extension();
	return path;
}

void loadCoverForLocalFile(
	const std::filesystem::path& comicPath, std::string_view storage, int& size,
	std::filesystem::path& coverPage, std::string& name) {
	wxString cover;
	processArchiveFile(comicPath, [&](const ArchiveFile& file) {
		if (!file.isFile() || !isImage(file.path())) { return; }
		size++;
		auto fullPath = cacheDirectory / file.path();
		if (!cover.empty() && wxCmpNatural(cover, fullPath.string()) < 0) {
			return;
		}
		cover = fullPath.string();
		coverPage = getCoverPath(storage, fullPath);
		file.writeContent(coverPage);
	});
	name = comicPath.stem().string();
}

void loadFromLocalFile(
	const std::filesystem::path& comicPath, std::string_view storage,
	std::vector<std::filesystem::path>& pages, ProgressUpdate progress) {
	const auto extractedCache = cacheDirectory / storage;
	processArchiveFile(comicPath, [&](const ArchiveFile& file) {
		if (!file.isFile() || !isImage(file.path())) { return; }
		pages.push_back(extractedCache / file.path());
		file.writeContent(pages.back());
		if (progress) { progress(static_cast<int>(pages.size()) - 1); }
	});
}

void loadCoverForRemoteFile(
	int id, std::string_view storage, int& size,
	std::filesystem::path& coverPage, std::string& name) {
	std::tie(name, size) = getChapterInfo(id);
	coverPage = cacheDirectory / (std::string(storage) + "_Cover.webp");
	downloadComic(id, [&](wxInputStream& stream) {
		saveStream(coverPage, stream);
		return false;
	});
}

void loadFromRemoteFile(
	int id, std::string_view storage, std::vector<std::filesystem::path>& pages,
	ProgressUpdate progress) {
	const auto extractedCache = cacheDirectory / storage;
	int index = 0;
	downloadComic(id, [&](wxInputStream& stream) {
		auto path = extractedCache / (std::to_string(index++) + ".webp");
		saveStream(path, stream);
		pages.emplace_back(path);
		if (progress) { progress(static_cast<int>(pages.size()) - 1); }
		return true;
	});
}

Comic::Comic(ComicSource source) : src(std::move(source)), size(0) {
	storage = std::to_string(COMIC_COUNTER.fetch_add(1));

	if (!src.path.empty()) {
		loadCoverForLocalFile(src.path, storage, size, coverPage, name);
	} else {
		loadCoverForRemoteFile(src.chapterId, storage, size, coverPage, name);
	}

	saveThumbnail(coverPage, coverPage, GET_THUMB_DIM());
}

void Comic::load(ProgressUpdate progress) {
	unload();
	if (!src.path.empty()) {
		loadFromLocalFile(src.path, storage, pages, std::move(progress));
	} else {
		loadFromRemoteFile(src.chapterId, storage, pages, std::move(progress));
	}
	std::ranges::sort(pages, [](const auto& a, const auto& b) {
		return wxCmpNatural(a.string(), b.string()) < 0;
	});
	size = pages.size();
}

void Comic::unload() {
	pages.clear();
	std::filesystem::remove_all(cacheDirectory / storage);
}

void Comic::markAsRead() const {
	if (!src.path.empty()) { return; }
	::markAsRead(src.chapterId);
}

int Comic::length() const { return size; };
std::string Comic::getName() const { return name; };
