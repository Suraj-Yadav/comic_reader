#include "comic_gallery.hpp"

#include <wx/dcbuffer.h>
#include <wx/progdlg.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <queue>

#include "comic.hpp"
#include "comic_viewer.hpp"
#include "fuzzy.hpp"
#include "util.hpp"
#include "wxUtil.hpp"

const int GALLERY_UPDATE_ID = 100000;

ComicGallery::ComicGallery(
	wxWindow* parent, const std::vector<std::filesystem::path>& paths)
	: wxPanel(parent), index(0), workInBackground(false) {
	Bind(wxEVT_PAINT, &ComicGallery::OnPaint, this);
	Bind(wxEVT_SIZE, &ComicGallery::OnSize, this);
	Bind(
		wxEVT_COMMAND_TEXT_UPDATED, &ComicGallery::OnComicAddition, this,
		GALLERY_UPDATE_ID);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(wxColour(25, 25, 25));

	loadComics(paths);
}

ComicGallery::~ComicGallery() {
	if (workInBackground.load()) {
		wxProgressDialog dialog("Stopping Background Threads", "");
		dialog.Pulse();
		workInBackground.store(false);
		loader.wait();
	}
}

bool ComicGallery::AddComic(std::filesystem::path path) {
	Comic c(path);
	if (c.length() > 0) {
		pool.addImage(c.coverPage);
		comics.push_back(c);
		return true;
	}
	return false;
}

void ComicGallery::loadComics(std::vector<std::filesystem::path> paths) {
	if (paths.empty()) { return; }
	std::sort(paths.begin(), paths.end(), [](const auto& a, const auto& b) {
		return wxCmpNatural(a.stem().string(), b.stem().string()) < 0;
	});
	paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
	comics.reserve(paths.size());

	auto offset = 0u;
	for (; offset < paths.size(); ++offset) {
		if (AddComic(paths[offset])) { break; }
	}

	workInBackground.store(false);

	if (!paths.empty()) {
		workInBackground.store(true);
		loader = std::async([paths, offset, this]() {
			for (auto i = offset + 1; i < paths.size(); ++i) {
				if (!workInBackground.load()) { return; }
				if (AddComic(paths[i])) {
					GetEventHandler()->AddPendingEvent(wxCommandEvent(
						wxEVT_COMMAND_TEXT_UPDATED, GALLERY_UPDATE_ID));
				};
			}
			workInBackground.store(false);
		});
	}
	index = 0;
}

void ComicGallery::verify(const wxGraphicsContext* gc, int i) {}

template <typename T, typename U> T mix(T x, T y, U a) {
	return x * (1 - a) + a * y;
}

void ComicGallery::OnComicAddition(wxCommandEvent& event) { Refresh(); }

void ComicGallery::OnPaint(wxPaintEvent& event) {
	const double FOCUSED_COMIC = 0.9, REST_COMIC = 0.7;

	if (comics.empty()) { return; }

	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	// direct2d renderer
	wxGraphicsRenderer* d2dr = wxGraphicsRenderer::GetDefaultRenderer();
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

		auto textHeight = drawWrappedText(
			{comics[idx].getName(), std::to_string(comics[idx].length())}, gc,
			cw, ch);
		const double GAP = 0.1 * (ch - textHeight);

		const auto size = comics.size();
		std::vector<wxRealPoint> pos(size);
		std::vector<double> scale(size, 1);
		std::vector<double> width(size, 0);

		std::queue<int> coversToConsider;
		std::vector<int> coversToDraw;

		{
			verify(gc, idx);
			const auto iw = pool.size(idx).GetWidth();
			const auto ih = pool.size(idx).GetHeight();

			scale[idx] =
				std::min(double(ch - textHeight) / ih, double(cw) / iw);
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

			if (i < 0 || i >= size) continue;
			verify(gc, i);

			auto sgn = i > idx ? 1 : -1;

			const auto iw = pool.size(i).GetWidth();
			const auto ih = pool.size(i).GetHeight();

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

		// Draw loading bar
		if (workInBackground.load()) {
			gc->SetBrush(wxBrush(*wxRED_BRUSH));
			gc->DrawRectangle(0, 0, cw, 5);
			gc->SetBrush(wxBrush(*wxGREEN_BRUSH));
			gc->DrawRectangle(0, 0, float(size * cw) / comics.capacity(), 5);
		}

		// Draw comics
		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);
		for (auto& i : coversToDraw) {
			const auto iw = pool.size(i).GetWidth();
			const auto ih = pool.size(i).GetHeight();

			gc->Translate(pos[i].x, pos[i].y);
			gc->Scale(scale[i], scale[i]);
			gc->DrawBitmap(pool.bitmap(i), -iw / 2.0, -ih / 2.0, iw, ih);
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
		animator.Start(
			200, index, nextIndex,
			[this](float v) {
				animatingIndex = v;
				Refresh();
			},
			[this, nextIndex]() {
				index = nextIndex;
				Refresh();
			});
	}
}

int ComicGallery::length() const { return comics.size(); };
