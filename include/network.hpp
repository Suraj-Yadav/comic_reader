#pragma once

#include <vector>

#include "comic.hpp"

std::vector<ComicSource> getUnreadChapters();
std::string getServer(bool changeServer = false);
void downloadComic(int id, const std::function<bool(const std::string& data)>&);
bool saveStream(const std::filesystem::path& path, const std::string& data);
void markAsRead(int id);
