#include "viewport.hpp"

double Viewport::W() const { return m_width; }
double Viewport::H() const { return m_height; }
double Viewport::X() const { return m_x; }
double Viewport::Y() const { return m_y; }

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
