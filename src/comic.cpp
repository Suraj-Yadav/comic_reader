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
#include "wxUtil.hpp"

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
	ComicSource& src, std::string_view storage,
	std::filesystem::path& coverPage) {
	std::string cover;
	processArchiveFile(src.path, [&](const ArchiveFile& file) {
		if (!file.isFile() || !isImage(file.path())) { return; }
		src.size++;
		auto fullPath = cacheDirectory / file.path();
		if (!cover.empty() && naturalLess(cover, fullPath.string())) { return; }
		cover = fullPath.string();
		coverPage = getCoverPath(storage, fullPath);
		file.writeContent(coverPage);
	});
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
	ComicSource& src, std::string_view storage,
	std::filesystem::path& coverPage) {
	coverPage = cacheDirectory / (std::string(storage) + "_Cover.webp");
	downloadComic(src.chapterId, [&](const std::string& data) {
		saveStream(coverPage, data);
		return false;
	});
	if (!std::filesystem::exists(coverPage)) { src.size = 0; }
}

void loadFromRemoteFile(
	int id, std::string_view storage, std::vector<std::filesystem::path>& pages,
	ProgressUpdate progress) {
	const auto extractedCache = cacheDirectory / storage;
	int index = 0;
	downloadComic(id, [&](const std::string& data) {
		auto path = extractedCache / (std::to_string(index++) + ".webp");
		saveStream(path, data);
		pages.emplace_back(path);
		if (progress) { progress(static_cast<int>(pages.size()) - 1); }
		return true;
	});
}

ComicSource::ComicSource(std::filesystem::path _path)
	: path(std::move(_path)), size(0), name(path.stem()) {}
ComicSource::ComicSource(int _chapterId, int _size, std::string _name)
	: chapterId(_chapterId), size(_size), name(std::move(_name)) {}

Comic::Comic(ComicSource source) : src(std::move(source)) {
	storage = std::to_string(COMIC_COUNTER.fetch_add(1));

	if (!src.path.empty()) {
		loadCoverForLocalFile(src, storage, coverPage);
	} else {
		loadCoverForRemoteFile(src, storage, coverPage);
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
		return naturalLess(a.string(), b.string());
	});
	src.size = pages.size();
}

void Comic::unload() {
	pages.clear();
	std::filesystem::remove_all(cacheDirectory / storage);
}

void Comic::markAsRead() {
	if (!src.path.empty() || isRead) { return; }
	::markAsRead(src.chapterId);
	isRead = true;
}

int Comic::length() const { return src.size; };
std::string Comic::getName() const { return src.name; };
