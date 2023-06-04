#include "comic_gallery.hpp"

#include <wx/dcbuffer.h>
#include <wx/progdlg.h>

#include <algorithm>
#include <thread>

#include "comic.hpp"
#include "util.hpp"

const auto MAX_CPU_THREADS = std::max(std::thread::hardware_concurrency(), 1u);

ComicGallery::ComicGallery(
	wxWindow* parent, const std::vector<std::filesystem::path>& paths)
	: wxPanel(parent),
	  index(0),
	  bitmaps(paths.size()),
	  sizes(paths.size()),
	  gBitmaps(paths.size()) {
	Bind(wxEVT_PAINT, &ComicGallery::OnPaint, this);
	Bind(wxEVT_SIZE, &ComicGallery::OnSize, this);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(wxColour(25, 25, 25, 1));

	loadComics(paths);
}

void ComicGallery::loadComics(std::vector<std::filesystem::path> paths) {
	std::sort(paths.begin(), paths.end());
	paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

	wxProgressDialog dialog(
		"Please wait", "Loading Comics", paths.size(), this);

	std::mutex guard;
	auto f = [&](bool isMainThread) {
		for (std::filesystem::path path; !paths.empty();) {
			{
				std::lock_guard<std::mutex> lock(guard);
				if (paths.empty()) {
					break;
				}
				path = paths.back();
				paths.pop_back();
			}
			auto c = Comic(path);
			{
				std::lock_guard<std::mutex> lock(guard);
				comics.push_back(c);
				if (isMainThread) {
					dialog.Update(comics.size());
				}
			}
		}
	};
	std::vector<std::thread> threads;
	for (auto i = 2u; i < MAX_CPU_THREADS; ++i) {
		threads.emplace_back(f, false);
	}
	f(true);
	for (auto& thread : threads) {
		thread.join();
	}
	dialog.Update(comics.size());

	std::sort(comics.begin(), comics.end());
	index = 0;
}

void ComicGallery::verify(const wxGraphicsContext* gc, int i) {
	if (!bitmaps[i].IsOk()) {
		bitmaps[i].LoadFile(comics[i].coverPage.string(), wxBITMAP_TYPE_ANY);
		const auto& b = bitmaps[i];
		sizes[i].Set(b.GetWidth(), b.GetHeight());
		gBitmaps[i] = gc->CreateBitmap(b);
	}
}

std::vector<std::pair<std::string, double>> split(
	const std::string& text, const wxArrayDouble& widths,  //
	const int cw, const int ch) {
	std::vector<std::pair<std::string, double>> words(1);
	double space = 0.0;
	for (auto i = 0u; i < text.size(); ++i) {
		if (text[i] == ' ') {
			space = widths[i];
		}
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
	const std::string& text, wxGraphicsContext* gc, const int cw,
	const int ch) {
	gc->SetFont(
		gc->CreateFont(wxFontInfo(15).Family(wxFONTFAMILY_DEFAULT), *wxWHITE));
	double w, h, d, e;
	gc->GetTextExtent(text, &w, &h, &d, &e);

	wxArrayDouble widths;
	gc->GetPartialTextExtents(text, widths);
	for (auto i = widths.size() - 1; i > 0; --i) {
		widths[i] = widths[i] - widths[i - 1];
	}

	auto words = split(text, widths, cw, ch);

	double y = ch - words.size() * (h + d + e);
	for (auto& elem : words) {
		gc->DrawText(elem.first, cw / 2.0 - elem.second / 2, y);
		y += h + d + e;
	}
	return words.size() * (h + d + e);
}

void ComicGallery::OnPaint(wxPaintEvent& event) {
	const double FOCUSED_COMIC = 0.9, REST_COMIC = 0.7;
	const double GAP = 0.1 * THUMB_DIM;

	if (comics.empty()) {
		return;
	}

	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	// direct2d renderer
	wxGraphicsRenderer* d2dr = wxGraphicsRenderer::GetDirect2DRenderer();
	wxGraphicsContext* gc = d2dr->CreateContext(dc);

	if (gc) {
		const auto cw = GetClientSize().GetWidth();
		const auto ch = GetClientSize().GetHeight();

		auto textHeight = drawWrappedText(comics[index].getName(), gc, cw, ch);

		verify(gc, index);

		std::vector<wxRealPoint> pos(comics.size());
		std::vector<double> scale(comics.size(), 1);
		std::vector<double> width(comics.size(), 0);
		std::vector<int> coversToDraw;

		{
			const auto iw = sizes[index].GetWidth();
			const auto ih = sizes[index].GetHeight();

			scale[index] = FOCUSED_COMIC * double(ch - textHeight) / ih;
			pos[index].x = 0.5 * cw;
			pos[index].y = 0.5 * (ch - textHeight);
			width[index] = scale[index] * iw;
			coversToDraw.push_back(index);
		}

		for (auto i = index + 1; i < comics.size(); ++i) {
			verify(gc, i);
			const auto iw = sizes[i].GetWidth();
			const auto ih = sizes[i].GetHeight();

			scale[i] = REST_COMIC * double(ch - textHeight) / ih;
			pos[i].x = pos[i - 1].x + width[i - 1] + GAP * scale[i];
			pos[i].y = pos[i - 1].y;
			width[i] = scale[i] * iw;
			if ((pos[i].x - width[i] / 2) > cw) {
				break;
			}
			coversToDraw.push_back(i);
		}

		for (auto i = index - 1; i >= 0; --i) {
			verify(gc, i);
			const auto iw = sizes[i].GetWidth();
			const auto ih = sizes[i].GetHeight();

			scale[i] = REST_COMIC * double(ch - textHeight) / ih;
			pos[i].x = pos[i + 1].x - width[i + 1] - GAP * scale[i];
			pos[i].y = pos[i + 1].y;
			width[i] = scale[i] * iw;
			if ((pos[i].x + width[i] / 2) < 0) {
				break;
			}
			coversToDraw.push_back(i);
		}

		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

		for (auto& i : coversToDraw) {
			const auto iw = sizes[i].GetWidth();
			const auto ih = sizes[i].GetHeight();

			gc->Translate(pos[i].x, pos[i].y);
			gc->Scale(scale[i], scale[i]);
			gc->DrawBitmap(gBitmaps[i], -iw / 2.0, -ih / 2.0, iw, ih);
			gc->Scale((1 / scale[i]), (1 / scale[i]));
			gc->Translate(-pos[i].x, -pos[i].y);
		}

		delete gc;
	}
}
void ComicGallery::OnSize(wxSizeEvent& event) { Refresh(); }

void ComicGallery::HandleInput(Navigation input) {
	switch (input) {
		case Navigation::PreviousComic:
			index = std::max(index - 1, 0);
			break;
		case Navigation::NextComic:
			index = std::min(index + 1, int(comics.size() - 1));
			break;
		default:
			return;
	}
	Refresh();
}
