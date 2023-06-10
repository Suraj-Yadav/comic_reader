#pragma once

#include <wx/graphics.h>
#include <wx/wx.h>

#include <filesystem>
#include <functional>

#include "comic.hpp"
#include "viewport.hpp"

enum Navigation {
	NextComic,		// Move to next Comic
	PreviousComic,	// Move to previous Comic
	JumpToComic,	// Jump to particular Comic
	Back,			// Go back one route
	NextPage,		// Move to next page
	PreviousPage,	// Move to previous page
	NextView,		// Move to next view
	PreviousView,	// Move to previous view
	SwitchScroll,
	JumpToPage,
	NoOp
};

class ComicViewer : public wxPanel {
	Comic& comic;
	int index;
	std::vector<wxBitmap> bitmaps;
	std::vector<wxSize> sizes;
	std::vector<wxGraphicsBitmap> gBitmaps;
	wxPoint2DDouble inProgressPanVector;
	wxPoint2DDouble inProgressPanStartPoint;
	bool panInProgress;

	Viewport viewport;

	void OnPaint(wxPaintEvent&);
	void OnMouseWheel(wxMouseEvent&);
	void OnLeftDown(wxMouseEvent&);
	void OnLeftDClick(wxMouseEvent&);
	void OnMotion(wxMouseEvent&);
	void OnLeftUp(wxMouseEvent&);
	void OnSize(wxSizeEvent&);
	void OnCaptureLost(wxMouseCaptureLostEvent&);

	void ProcessPan(const wxPoint2DDouble&, bool);
	void FinishPan(bool);

	Navigation MoveViewport(Navigation direction);
	void OptimizeViewport();
	void NextZoom(const wxPoint&);
	wxPoint2DDouble MapClientToViewport(const wxPoint&);
	double GetZoom();
	void verify(const wxGraphicsContext* g, int index);

   public:
	ComicViewer(wxWindow* parent, Comic& comic);
	int length() const;
	void load();
	void HandleInput(Navigation);
};
