#pragma once

#include <wx/graphics.h>
#include <wx/wx.h>

#include <filesystem>
#include <functional>

#include "animator.hpp"
#include "comic.hpp"
#include "image_utils.hpp"
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
	JumpToPage,		// Jump to page
	SwitchScroll,
	NoOp
};

enum PanSource { Mouse, Animation };
enum AnimationType { None, Pan, View };

class ComicViewer : public wxPanel {
	Comic& comic;
	int index;
	AnimationType animation;

	ImagePool pool;

	wxPoint2DDouble inProgressPanVector;
	wxPoint2DDouble inProgressPanStartPoint;

	Animator<wxPoint2DDouble> viewportAnimator;

	Viewport viewport;

	void OnPaint(wxPaintEvent&);
	void OnMouseWheel(wxMouseEvent&);
	void OnLeftDown(wxMouseEvent&);
	void OnLeftDClick(wxMouseEvent&);
	void OnMotion(wxMouseEvent&);
	void OnLeftUp(wxMouseEvent&);
	void OnSize(wxSizeEvent&);
	void OnCaptureLost(wxMouseCaptureLostEvent&);
	void OnClose(wxCloseEvent&);

	void StartPan(const wxPoint2DDouble&, PanSource);
	void ProcessPan(const wxPoint2DDouble&, bool, PanSource);
	void FinishPan(bool refresh, PanSource);

	std::pair<Navigation, wxPoint2DDouble> ComputeMove(Navigation direction);
	void OptimizeViewport();
	void NextZoom(const wxPoint&);
	wxPoint2DDouble MapClientToViewport(const wxPoint&);
	double GetZoom();
	bool verify(const wxGraphicsContext* g, int index);

   public:
	ComicViewer(wxWindow* parent, Comic& comic);
	void load();
	void HandleInput(Navigation);
};
