#pragma once

#include <wx/graphics.h>
#include <wx/panel.h>

#include <filesystem>
#include <future>
#include <vector>

#include "animator.hpp"
#include "comic.hpp"
#include "comic_viewer.hpp"
#include "image_utils.hpp"

class ComicGallery : public wxPanel {
	std::vector<Comic> comics;
	int index;
	float animatingIndex;
	ImagePool pool;
	Animator<float> animator;
	std::atomic_bool workInBackground;
	std::future<void> loader;

	void OnComicAddition(wxCommandEvent& evt);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& event);
	bool AddComic(std::filesystem::path path);

	void verify(const wxGraphicsContext* g, int index);

   public:
	ComicGallery(
		wxWindow* parent, const std::vector<std::filesystem::path>& paths);
	~ComicGallery();
	void loadComics(std::vector<std::filesystem::path> paths);
	void HandleInput(Navigation input, char ch = ' ');
	Comic& currentComic() { return comics[index]; }
	int length() const;
};
