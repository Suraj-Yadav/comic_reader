#pragma once

#include <wx/graphics.h>

class Viewport : public wxRect2DDouble {
   public:
	Viewport() = default;
	Viewport(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
		: wxRect2DDouble(x, y, w, h) {}

	[[nodiscard]] double W() const;
	[[nodiscard]] double H() const;
	[[nodiscard]] double X() const;
	[[nodiscard]] double Y() const;

	void Translate(const wxPoint2DDouble& delta);
	void Translate(double deltaX, double deltaY);
	void ScaleAtPoint(const wxPoint2DDouble& point, double scale);
};
