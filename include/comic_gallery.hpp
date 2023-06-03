#include <wx/graphics.h>
#include <wx/panel.h>

#include <filesystem>
#include <vector>

class ComicGallery : public wxPanel {
	std::vector<std::filesystem::path> paths;
	int index;
	std::vector<wxBitmap> bitmaps;
	std::vector<wxSize> sizes;
	std::vector<wxGraphicsBitmap> gBitmaps;

	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	void verify(const wxGraphicsContext* g, int index);

   public:
	ComicGallery(
		wxWindow* parent, const std::vector<std::filesystem::path>& paths);
};
