#pragma once

#include <archive.h>
#include <archive_entry.h>
#include <stdint.h>

#include <cstdint>
#include <filesystem>
#include <functional>

std::string getMimeType(const std::filesystem::path& filePath);

class ArchiveFile {
	friend void processArchiveFile(
		const std::filesystem::path& filePath,
		std::function<void(const ArchiveFile&)> func);

	struct archive* archivePtr;
	archive_entry* entry;

	ArchiveFile(struct archive* archivePtr, archive_entry* entry);
	int type() const;

   public:
	std::filesystem::path path() const;
	int64_t size() const;
	bool isFile() const;
	void writeContent(const std::filesystem::path& filePath) const;
};

void processArchiveFile(
	const std::filesystem::path& filePath,
	std::function<void(const ArchiveFile&)> func);
