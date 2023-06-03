#include <wx/arrstr.h>
#include <wx/image.h>
#include <wx/settings.h>

#include <archive.hpp>
#include <comic.hpp>
#include <filesystem>
#include <format>
#include <util.hpp>

const int THUMB_DIM =
	min(wxSystemSettings::GetMetric(wxSYS_SCREEN_X),
		wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));

const std::filesystem::path cacheDirectory =
	std::filesystem::temp_directory_path() / "comicReaderCache2";

bool isImage(const ArchiveFile& file) {
	const std::string prefix = "image/";
	const auto mimeType = getMimeType(file.path());
	return mimeType.compare(0, prefix.size(), prefix) == 0;
}

std::filesystem::path getCoverPath(const std::filesystem::path& img) {
	auto path = cacheDirectory / img.stem();
	path += "_Cover";
	path += img.extension();
	return path;
}

Comic::Comic(const std::filesystem::path& comicPath)
	: comicPath(comicPath), size(0) {
	wxString cover;
	std::string extension;

	processArchiveFile(comicPath, [&](const ArchiveFile& file) {
		if (!file.isFile() || !isImage(file)) {
			return;
		}
		size++;
		auto fullPath = cacheDirectory / file.path();
		if (!cover.empty() && wxCmpNatural(cover, fullPath.string()) < 0) {
			return;
		}
		cover = fullPath.string();
		coverPage = cacheDirectory / (comicPath.stem().string() + "_Cover.png");
		file.writeContent(coverPage);
	});
	auto thumbnail = coverPage;
	thumbnail.replace_filename(comicPath.stem().string() + "_Thumb.png");
	auto img = wxImage(coverPage.string());
	wxSize size(THUMB_DIM, THUMB_DIM);
	if (img.GetWidth() > img.GetHeight()) {
		size.SetHeight(img.GetHeight() * THUMB_DIM / img.GetWidth());
	} else {
		size.SetWidth(img.GetWidth() * THUMB_DIM / img.GetHeight());
	}
	img.Rescale(size.GetWidth(), size.GetHeight(), wxIMAGE_QUALITY_HIGH);
	img.SaveFile(thumbnail.string());
	std::filesystem::rename(thumbnail, coverPage);
}

int Comic::length() const { return size; };

void Comic::load(std::function<void(int i)> progress) {
	pages.clear();
	const auto extractedCache = cacheDirectory / comicPath.stem();
	processArchiveFile(comicPath, [&](const ArchiveFile& file) {
		if (!file.isFile() || !isImage(file)) {
			return;
		}
		pages.push_back(extractedCache / file.path());
		file.writeContent(pages.back());
		if (progress) {
			progress(pages.size());
		}
	});
	std::sort(pages.begin(), pages.end(), [](const auto& a, const auto& b) {
		return wxCmpNatural(a.string(), b.string()) < 0;
	});
	size = pages.size();
}
