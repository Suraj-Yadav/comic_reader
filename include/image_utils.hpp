#include <filesystem>

bool saveThumbnail(
	const std::filesystem::path& src, const std::filesystem::path& dest,
	const unsigned int MAX_DIM);
