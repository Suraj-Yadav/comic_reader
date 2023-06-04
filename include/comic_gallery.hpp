#include <wx/graphics.h>
#include <wx/panel.h>

#include <filesystem>
#include <vector>

#include "comic.hpp"
#include "comic_viewer.hpp"

class ComicGallery : public wxPanel {
	std::vector<Comic> comics;
	int index;
	std::vector<wxBitmap> bitmaps;
	std::vector<wxSize> sizes;
	std::vector<wxGraphicsBitmap> gBitmaps;

	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& event);

	void verify(const wxGraphicsContext* g, int index);

   public:
	ComicGallery(
		wxWindow* parent, const std::vector<std::filesystem::path>& paths);
	void loadComics(std::vector<std::filesystem::path> paths);
	void HandleInput(Navigation);
	Comic& currentComic() { return comics[index]; }
};
