#include "wxUtil.hpp"

#include <wx/brush.h>
#include <wx/debug.h>
#include <wx/textwrapper.h>

#include <cmath>

#include "util.hpp"

// Collects the wrapped lines instead of drawing them.
class LineCollector : public wxTextWrapper {
   public:
	LineCollector(wxWindow* win, const wxString& text, int widthMax) {
		Wrap(win, text, widthMax);
	}

	std::vector<wxString> lines;

   protected:
	void OnOutputLine(const wxString& line) override { lines.push_back(line); }
};

// Each input line (UTF-8) is wrapped to `cw` and centred horizontally; the
// blocks are stacked upwards from the bottom edge `ch`. Returns the height
// drawn.
double drawWrappedText(
	const std::vector<std::string>& utf8Lines, wxWindow* win,
	wxGraphicsContext* gc, int cw, int ch) {
	static auto font =
		gc->CreateFont(wxFontInfo(15).Family(wxFONTFAMILY_DEFAULT), *wxWHITE);

	gc->SetFont(font);

	// Line height from a fixed sample, so it doesn't depend on the first line.
	double w = 0, h = 0, d = 0, e = 0;
	gc->GetTextExtent("Ag", &w, &h, &d, &e);
	const double lineH = h + d + e;

	double y = ch;
	size_t total = 0;
	for (auto it = utf8Lines.rbegin(); it != utf8Lines.rend(); ++it) {
		const wxString txt = wxString::FromUTF8(it->data(), it->size());
		const LineCollector wrapped(win, txt, cw);

		y -= wrapped.lines.size() * lineH;
		for (const auto& line : wrapped.lines) {
			double lw = 0, lh = 0;
			gc->GetTextExtent(
				line, &lw, &lh);  // width of this line, for centring
			gc->DrawText(line, cw / 2.0 - lw / 2, y);
			y += lineH;
		}
		y -= wrapped.lines.size() * lineH;
		total += wrapped.lines.size();
	}
	return total * lineH;
}

void drawBottomText(
	const std::string& text, wxGraphicsContext* gc, const int cw, const int ch,
	Position pos) {
	gc->SetFont(
		gc->CreateFont(wxFontInfo(15).Family(wxFONTFAMILY_DEFAULT), *wxWHITE));
	double w = NAN, h = NAN;
	gc->GetTextExtent(text, &w, &h);
	constexpr auto R = 10.0;
	constexpr auto G = 5.0;
	const auto bw = w + G + R, bh = h + 5;
	gc->SetBrush(wxBrush(*wxGREY_BRUSH));
	if (pos == Position::LEFT) {
		gc->DrawRoundedRectangle(bw / 2 - R, ch - bh, bw / 2 + R, bh, R);
		gc->DrawRectangle(0, ch - bh, bw / 2, bh / 2);
		gc->DrawRectangle(0, ch - bh / 2, bw, bh / 2);
		gc->DrawText(text, (G + R) / 2, ch - h);
	} else if (pos == Position::CENTER) {
		gc->DrawRoundedRectangle((cw - bw) / 2, ch - bh, bw, bh, R);
		gc->DrawRectangle((cw - bw) / 2, ch - bh / 2, bw, bh / 2);
		gc->DrawText(text, (cw - w) / 2, ch - h);
	} else if (pos == Position::RIGHT) {
		gc->DrawRoundedRectangle(cw - bw, ch - bh, bw / 2 + R, bh, R);
		gc->DrawRectangle(cw - bw / 2, ch - bh, bw / 2, bh / 2);
		gc->DrawRectangle(cw - bw, ch - bh / 2, bw, bh / 2);
		gc->DrawText(text, cw - bw + (G + R) / 2, ch - h);
	}
}

bool naturalLess(std::string_view a, std::string_view b) {
	return wxCmpNatural(
			   wxString::FromUTF8(a.data(), a.size()),
			   wxString::FromUTF8(b.data(), b.size())) < 0;
}
