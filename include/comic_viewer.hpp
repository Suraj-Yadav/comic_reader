#pragma once

#include <wx/wx.h>

#include <comic.hpp>
#include <filesystem>
#include <functional>
#include <image_viewer.hpp>

class ComicViewer : public wxPanel {
	void OnKeyDown(wxKeyEvent&);

	Comic comic;
	std::vector<ImageViewer*> imageViewers;
	int index;
	wxSizer* sizer;

   public:
	ComicViewer(wxWindow* parent, const std::filesystem::path& comicPath);
	int length() const;
	void load();
};
