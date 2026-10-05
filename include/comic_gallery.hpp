#pragma once

#include <wx/graphics.h>
#include <wx/panel.h>

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

	void OnComicAddition(wxCommandEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	bool AddComic(ComicSource src);

	void verify(const wxGraphicsContext* g, int index);

   public:
	ComicGallery(wxWindow* parent);
	~ComicGallery();
	void loadComics(std::vector<ComicSource> paths);
	void HandleInput(Navigation input, char ch = ' ');
	Comic& currentComic() { return comics[index]; }
	int length() const;
};
