#include "comic_gallery.hpp"

#include <wx/dcbuffer.h>
#include <wx/progdlg.h>

#include <algorithm>
#include <cmath>
#include <queue>
#include <thread>

#include "comic.hpp"
#include "comic_viewer.hpp"
#include "fuzzy.hpp"
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
				if (paths.empty()) { break; }
				path = paths.back();
				paths.pop_back();
			}
			auto c = Comic(path);
			{
				std::lock_guard<std::mutex> lock(guard);
				comics.push_back(c);
				if (isMainThread) { dialog.Update(comics.size()); }
			}
		}
	};
	std::vector<std::thread> threads;
	for (auto i = 2u; i < MAX_CPU_THREADS; ++i) {
		threads.emplace_back(f, false);
	}
	f(true);
	for (auto& thread : threads) { thread.join(); }
	dialog.Update(comics.size());

	std::sort(comics.begin(), comics.end(), [](const auto& a, const auto& b) {
		return wxCmpNatural(a.getName(), b.getName()) < 0;
	});
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

template <typename T, typename U> T mix(T x, T y, U a) {
	return x * (1 - a) + a * y;
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

	if (comics.empty()) { return; }

	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	// direct2d renderer
	wxGraphicsRenderer* d2dr = wxGraphicsRenderer::GetDirect2DRenderer();
	wxGraphicsContext* gc = d2dr->CreateContext(dc);

	if (gc) {
		const auto cw = GetClientSize().GetWidth();
		const auto ch = GetClientSize().GetHeight();

		int idx = index, nextIdx = index;
		float frac = 0.0f;

		if (animator.IsRunning()) {
			idx = static_cast<int>(std::floor(animatingIndex));
			nextIdx = static_cast<int>(std::ceil(animatingIndex));
			frac = animatingIndex - idx;
		}

		auto textHeight = drawWrappedText(comics[idx].getName(), gc, cw, ch);
		const double GAP = 0.1 * (ch - textHeight);

		std::vector<wxRealPoint> pos(comics.size());
		std::vector<double> scale(comics.size(), 1);
		std::vector<double> width(comics.size(), 0);

		std::queue<int> coversToConsider;
		std::vector<int> coversToDraw;

		{
			verify(gc, idx);
			const auto iw = sizes[idx].GetWidth();
			const auto ih = sizes[idx].GetHeight();

			scale[idx] = double(ch - textHeight) / ih;
			scale[idx] *= mix(FOCUSED_COMIC, REST_COMIC, frac);

			width[idx] = scale[idx] * iw;

			pos[idx].x = 0.5 * cw;
			pos[idx].y = 0.5 * (ch - textHeight);
		}

		coversToConsider.push(idx + 1);
		coversToConsider.push(idx - 1);

		while (coversToConsider.size() > 0) {
			auto i = coversToConsider.front();
			coversToConsider.pop();

			if (i < 0 || i >= comics.size()) continue;
			verify(gc, i);

			auto sgn = i > idx ? 1 : -1;

			const auto iw = sizes[i].GetWidth();
			const auto ih = sizes[i].GetHeight();

			scale[i] = double(ch - textHeight) / ih;
			if (i == nextIdx) {
				scale[i] *= mix(REST_COMIC, FOCUSED_COMIC, frac);
			} else {
				scale[i] *= REST_COMIC;
			}

			width[i] = scale[i] * iw;

			pos[i].x = pos[i - sgn].x +
					   sgn * (0.5 * (width[i - sgn] + width[i]) + GAP);
			pos[i].y = pos[i - sgn].y;

			if ((sgn > 0 && (pos[i].x - width[i] / 2) <= cw) ||
				(sgn < 0 && (pos[i].x + width[i] / 2) >= 0)) {
				coversToDraw.push_back(i);
				coversToConsider.push(i + sgn);
			}
			if (i == nextIdx) {
				auto delta = frac * (pos[nextIdx].x - pos[idx].x);
				pos[i].x -= delta;
				pos[idx].x -= delta;
			}
		}
		coversToDraw.push_back(idx);

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

void ComicGallery::HandleInput(Navigation input, char ch) {
	if (animator.IsRunning()) { return; }

	auto nextIndex = index;
	switch (input) {
		case Navigation::PreviousComic:
			nextIndex = std::max(index - 1, 0);
			break;
		case Navigation::NextComic:
			nextIndex = std::min(index + 1, int(comics.size() - 1));
			break;
		case Navigation::JumpToComic: {
			auto itr =
				std::find_if(comics.begin(), comics.end(), [ch](const auto& c) {
					return wxCmpNatural(wxString(ch), c.getName()) < 0;
				});
			if (itr == comics.end()) { itr--; }
			nextIndex = std::distance(comics.begin(), itr);
			break;
		}
		default:
			return;
	}
	if (index != nextIndex) {
		animator.Start(200, index, nextIndex);
		animator.SetOnStep([this](float v) {
			animatingIndex = v;
			Refresh();
		});
		animator.SetOnEnd([this, nextIndex]() {
			index = nextIndex;
			Refresh();
		});
	}
}
