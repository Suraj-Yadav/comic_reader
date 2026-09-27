#include "archive.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <map>
#include <string>

std::atomic_int tempFolderCounter;

class ArchiveTestFixtures
	: public ::testing::TestWithParam<std::filesystem::path> {};

TEST_P(ArchiveTestFixtures, VerifyArchive) {
	auto filePath = GetParam();
	std::map<std::filesystem::path, int64_t> filesExpected{
		{"a/b.txt", 1}, {"c.txt", 2}, {"test.png", 11645}};

	std::map<std::filesystem::path, int64_t> filesFound;
	auto tempDir = std::filesystem::temp_directory_path() / "archive_test" /
				   std::to_string(tempFolderCounter.fetch_add(1));

	processArchiveFile(filePath, [&](const ArchiveFile& file) {
		if (file.isFile()) {
			filesFound.insert({file.path(), file.size()});
			auto path = tempDir / file.path();
			file.writeContent(path);
			EXPECT_EQ(file.size(), std::filesystem::file_size(path));
		}
	});

	EXPECT_EQ(filesExpected, filesFound);
	std::filesystem::remove_all(tempDir);
}

INSTANTIATE_TEST_SUITE_P(
	VerifyArchive, ArchiveTestFixtures,
	::testing::Values("testdata/test.zip", "testdata/test.rar"));
