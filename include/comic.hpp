#pragma once

#include <filesystem>
#include <functional>
#include <string>

extern const std::filesystem::path cacheDirectory;

struct ComicSource {
	std::filesystem::path path;
	int chapterId = -1;

	bool operator<=>(const ComicSource&) const = default;
};

using ProgressUpdate = std::function<void(int i)>;

class Comic {
	ComicSource src;
	int size;
	std::string name;
	std::string storage;

   public:
	Comic(ComicSource source);
	void load(ProgressUpdate progress = nullptr);
	void unload();
	[[nodiscard]] int length() const;
	[[nodiscard]] std::string getName() const;
	void markAsRead() const;
	std::filesystem::path coverPage;
	std::vector<std::filesystem::path> pages;
};
