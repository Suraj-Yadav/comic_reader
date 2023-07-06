#pragma once

#include <filesystem>
#include <functional>

extern const std::filesystem::path cacheDirectory;
extern const int THUMB_DIM;

class Comic {
	std::filesystem::path comicPath;
	int size;

   public:
	Comic(const std::filesystem::path& comicPath);
	void load(std::function<void(int i)> progress = nullptr);
	void unload();
	int length() const;
	std::string getName() const;
	std::filesystem::path coverPage;
	std::vector<std::filesystem::path> pages;
};
