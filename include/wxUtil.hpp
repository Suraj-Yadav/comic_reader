#pragma once

#include <wx/graphics.h>

double drawWrappedText(
	const std::vector<std::string>& utf8Lines, wxWindow* win,
	wxGraphicsContext* gc, int cw, int ch);

enum class Position { LEFT, CENTER, RIGHT };

void drawBottomText(
	const std::string& text, wxGraphicsContext* gc, int cw, int ch,
	Position pos = Position::CENTER);

bool naturalLess(std::string_view a, std::string_view b);
