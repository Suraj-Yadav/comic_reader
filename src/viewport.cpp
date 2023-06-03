#include <viewport.hpp>

void Viewport::ScaleAtPoint(const wxPoint2DDouble& point, double scale) {
	Translate(-point);
	Scale(scale);
	Translate(point);
}

void Viewport::Translate(const wxPoint2DDouble& delta) {
	SetCentre(GetCentre() + delta);
}

void Viewport::Translate(double x, double y) {
	Translate(wxPoint2DDouble(x, y));
}
