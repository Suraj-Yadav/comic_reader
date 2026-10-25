#pragma once

#include <filesystem>
#include <functional>
#include <string>

extern const std::filesystem::path cacheDirectory;

struct ComicSource {
	std::filesystem::path path;
	int chapterId = -1;
	int size = 0;
	std::string name;

	ComicSource(std::filesystem::path path);
	ComicSource(int chapterId, int size, std::string name);

	bool operator<=>(const ComicSource&) const = default;
};

using ProgressUpdate = std::function<void(int i)>;

class Comic {
	ComicSource src;
	std::string storage;
	bool isRead = false;

   public:
	Comic(ComicSource source);
	void load(ProgressUpdate progress = nullptr);
	void unload();
	[[nodiscard]] int length() const;
	[[nodiscard]] std::string getName() const;
	void markAsRead();
	std::filesystem::path coverPage;
	std::vector<std::filesystem::path> pages;
};
