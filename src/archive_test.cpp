#include "archive.hpp"

#include <gtest/gtest.h>

#include <map>

class ArchiveTestFixtures
	: public ::testing::TestWithParam<std::filesystem::path> {};

TEST_P(ArchiveTestFixtures, VerifyArchive) {
	auto filePath = GetParam();
	std::map<std::filesystem::path, int> filesExpected{
		{"a/b.txt", 1}, {"c.txt", 2}, {"test.png", 11645}};

	std::map<std::filesystem::path, int> filesFound;
	auto tempDir = std::filesystem::path(std::tmpnam(nullptr));

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
