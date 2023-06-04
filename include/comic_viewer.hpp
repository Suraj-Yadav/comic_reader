#pragma once

#include <wx/wx.h>

#include <filesystem>
#include <functional>

#include "comic.hpp"
#include "image_viewer.hpp"

class ComicViewer : public wxPanel {
	Comic& comic;
	std::vector<ImageViewer*> imageViewers;
	int index;
	wxSizer* sizer;

   public:
	ComicViewer(wxWindow* parent, Comic& comic);
	int length() const;
	void load();
	void HandleInput(Navigation);
};
