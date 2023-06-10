#include <wx/graphics.h>
#include <wx/panel.h>

#include <filesystem>
#include <vector>

#include "animator.hpp"
#include "comic.hpp"
#include "comic_viewer.hpp"

class ComicGallery : public wxPanel {
	std::vector<Comic> comics;
	int index;
	float animatingIndex;
	std::vector<wxBitmap> bitmaps;
	std::vector<wxSize> sizes;
	std::vector<wxGraphicsBitmap> gBitmaps;
	Animator<float> animator;
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& event);

	void verify(const wxGraphicsContext* g, int index);

   public:
	ComicGallery(
		wxWindow* parent, const std::vector<std::filesystem::path>& paths);
	void loadComics(std::vector<std::filesystem::path> paths);
	void HandleInput(Navigation input, char ch = ' ');
	Comic& currentComic() { return comics[index]; }
};
