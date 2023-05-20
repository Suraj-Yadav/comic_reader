#pragma once

#include <wx/graphics.h>

class Viewport : public wxRect2DDouble {
   public:
	Viewport() : wxRect2DDouble() {}
	Viewport(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
		: wxRect2DDouble(x, y, w, h) {}

	void Translate(const wxPoint2DDouble& delta);
	void Translate(double deltaX, double deltaY);
	void ScaleAtPoint(const wxPoint2DDouble& point, double scale);
};
