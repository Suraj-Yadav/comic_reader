#pragma once

#include <wx/stream.h>

#include <vector>

#include "comic.hpp"

std::vector<ComicSource> getUnreadChapters();
std::string getServer(bool changeServer = false);
std::pair<std::string, int> getChapterInfo(int id);
void downloadComic(int id, const std::function<bool(wxInputStream& stream)>&);
bool saveStream(const std::filesystem::path& path, wxInputStream& stream);
void markAsRead(int id);
