#include "archive.hpp"

#include <wx/mimetype.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

#include "util.hpp"

std::string getMimeType(const std::filesystem::path& filePath) {
	auto f = wxTheMimeTypesManager->GetFileTypeFromExtension(
		filePath.extension().c_str());
	wxString mimeType;
	if (f == nullptr || !f->GetMimeType(&mimeType)) {
		return "";
	}
	return mimeType.ToStdString();
}

ArchiveFile::ArchiveFile(struct archive* ap, archive_entry* e)
	: archivePtr(ap), entry(e) {}

std::filesystem::path ArchiveFile::path() const {
	return std::filesystem::path(archive_entry_pathname(entry))
		.make_preferred();
}

int64_t ArchiveFile::size() const { return archive_entry_size(entry); }
int ArchiveFile::type() const { return archive_entry_filetype(entry); }
bool ArchiveFile::isFile() const { return type() == AE_IFREG; }

void ArchiveFile::writeContent(const std::filesystem::path& filePath) const {
	std::filesystem::create_directories(filePath.parent_path());
	std::ofstream file(filePath, std::ios::binary | std::ios::out);

	const auto length = 8192;
	std::array<char, length> buffer;
	auto len = archive_read_data(archivePtr, buffer.data(), length);
	while (len > 0) {
		file.write(buffer.data(), len);
		len = archive_read_data(archivePtr, buffer.data(), length);
	}
	file.flush();
	file.close();
	if (len < 0) {
		std::filesystem::remove(filePath);
		throw std::filesystem::filesystem_error(
			"Unable to read file from archive", filePath, path(),
			std::make_error_code(std::errc::io_error));
	}
}

void processArchiveFile(
	const std::filesystem::path& filePath,
	std::function<void(const ArchiveFile&)> func) {
	auto archive = archive_read_new();
	if (archive == nullptr) {
		throw std::invalid_argument("Couldn't create archive reader");
	}
	if (archive_read_support_filter_all(archive) != ARCHIVE_OK) {
		throw std::invalid_argument("Couldn't enable decompression");
	}
	if (archive_read_support_format_all(archive) != ARCHIVE_OK) {
		throw std::invalid_argument("Couldn't enable read formats");
	}
	if (archive_read_open_filename(archive, filePath.string().c_str(), 0) !=
		ARCHIVE_OK) {
		throw std::filesystem::filesystem_error(
			"Unable to open archive", filePath,
			std::make_error_code(std::errc::io_error));
	}

	struct archive_entry* entry;

	while (true) {
		auto r = archive_read_next_header(archive, &entry);
		if (r == ARCHIVE_EOF) {
			break;
		}
		if (r != ARCHIVE_OK) {
			archive_read_close(archive);
			throw std::invalid_argument(
				std::string("Unable to read next header: ") +
				archive_error_string(archive));
		}
		func(ArchiveFile(archive, entry));
	}

	archive_read_close(archive);
	archive_read_free(archive);
}
