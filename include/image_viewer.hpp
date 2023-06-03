#pragma once

#include <wx/graphics.h>
#include <wx/wx.h>

#include <viewport.hpp>

enum Navigation {
	NextComic,		// Move to next Comic
	PreviousComic,	// Move to previous Comic
	Back,			// Go back one route
	NextPage,		// Move to next page
	PreviousPage,	// Move to previous page
	NextView,		// Move to next view
	PreviousView,	// Move to previous view
	SwitchScroll,
	JumpToPage,
	NoOp
};

class ImageViewer : public wxPanel {
   public:
	ImageViewer(wxWindow* parent, const wxString& filePath);

	Navigation MoveViewport(Navigation direction);
	void SetViewport(const Viewport& v);
	const Viewport& GetViewport() const;
	const wxSize& GetImageSize() const;

   private:
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

	void OptimizeViewport();
	wxPoint2DDouble MapClientToViewport(const wxPoint&);
	double GetZoom();

	Viewport viewport;

	wxPoint2DDouble inProgressPanVector;
	wxPoint2DDouble inProgressPanStartPoint;
	bool panInProgress;
	bool firstPaint{true};

	wxString filePath;
	wxBitmap rawBitmap;
	wxSize imageSize;
	wxGraphicsBitmap drawBitmap;
};
