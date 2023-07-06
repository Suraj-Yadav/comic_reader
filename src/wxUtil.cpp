#include "wxUtil.hpp"

#include <wx/brush.h>

#include "util.hpp"

std::vector<std::pair<std::string, double>> split(
	const std::string& text, const wxArrayDouble& widths,  //
	const int cw, const int ch) {
	std::vector<std::pair<std::string, double>> words(1);
	double space = 0.0;
	for (auto i = 0u; i < text.size(); ++i) {
		if (text[i] == ' ') { space = widths[i]; }
		if (std::isspace(text[i]) && !words.back().first.empty()) {
			words.emplace_back();
		}
		if (!std::isspace(text[i])) {
			words.back().first += text[i];
			words.back().second += widths[i];
		}
	}

	std::vector<std::pair<std::string, double>> compressed(1, words[0]);
	for (auto i = 1u; i < words.size(); ++i) {
		if ((compressed.back().second + space + words[i].second) < cw) {
			compressed.back().first += " " + words[i].first;
			compressed.back().second += space + words[i].second;
		} else {
			compressed.push_back(words[i]);
		}
	}
	return compressed;
}

double drawWrappedText(
	std::vector<std::string> lines, wxGraphicsContext* gc, const int cw,
	const int ch) {
	std::reverse(lines.begin(), lines.end());
	gc->SetFont(
		gc->CreateFont(wxFontInfo(15).Family(wxFONTFAMILY_DEFAULT), *wxWHITE));
	double w, h, d, e;
	gc->GetTextExtent(lines[0], &w, &h, &d, &e);

	int totalWords = 0;
	double maxWidth = 0;
	double y = ch;
	for (const auto& text : lines) {
		wxArrayDouble widths;
		gc->GetPartialTextExtents(text, widths);
		for (auto i = widths.size() - 1; i > 0; --i) {
			widths[i] = widths[i] - widths[i - 1];
		}
		auto words = split(text, widths, cw, ch);
		y -= words.size() * (h + d + e);
		for (auto& elem : words) {
			maxWidth = std::max(maxWidth, elem.second);
			gc->DrawText(elem.first, cw / 2.0 - elem.second / 2, y);
			y += h + d + e;
		}
		y -= words.size() * (h + d + e);
		totalWords += words.size();
	}
	return totalWords * (h + d + e);
}

void drawBottomText(
	std::string text, wxGraphicsContext* gc, const int cw, const int ch) {
	gc->SetFont(
		gc->CreateFont(wxFontInfo(15).Family(wxFONTFAMILY_DEFAULT), *wxWHITE));
	double w, h;
	gc->GetTextExtent(text, &w, &h);
	const auto r = 10;
	const auto bw = w + 5 + r, bh = h + 5;
	gc->SetBrush(wxBrush(*wxGREY_BRUSH));
	gc->DrawRoundedRectangle((cw - bw) / 2, ch - bh, bw, bh, r);
	gc->DrawRectangle((cw - bw) / 2, ch - bh / 2, bw, bh / 2);
	double y = ch - h;
	gc->DrawText(text, (cw - w) / 2, y);
}
