#pragma once

#include <wx/graphics.h>
#include <wx/wx.h>

#include "viewport.hpp"

class ImageViewer : public wxPanel {
   public:
	ImageViewer(wxWindow* parent, const wxString& filePath);

   private:
	void OnPaint(wxPaintEvent&);

	void OnKeyDown(wxKeyEvent&);
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
	wxSize imageSize;

	wxPoint2DDouble inProgressPanVector;
	wxPoint2DDouble inProgressPanStartPoint;
	bool panInProgress;
	bool firstPaint{true};

	wxBitmap rawBitmap;
	wxGraphicsBitmap drawBitmap;
};
