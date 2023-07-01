#pragma once

#include <wx/graphics.h>

double drawWrappedText(
	std::vector<std::string> lines, wxGraphicsContext* gc, const int cw,
	const int ch);

void drawBottomText(
	std::string text, wxGraphicsContext* gc, const int cw, const int ch);
