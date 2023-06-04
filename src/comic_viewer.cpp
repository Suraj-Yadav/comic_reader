#include "comic_viewer.hpp"

#include <wx/progdlg.h>

#include "image_viewer.hpp"
#include "util.hpp"

ComicViewer::ComicViewer(wxWindow* parent, Comic& comic)
	: wxPanel(parent),
	  comic(comic),
	  index(0),
	  sizer(new wxBoxSizer(wxHORIZONTAL)) {
	SetSizer(sizer);
	Bind(wxEVT_MOUSEWHEEL, [](wxMouseEvent&) { printFuncCall; });
	Bind(wxEVT_LEFT_DOWN, [](wxMouseEvent&) { printFuncCall; });
	Bind(wxEVT_LEFT_DCLICK, [](wxMouseEvent&) { printFuncCall; });
}

int ComicViewer::length() const { return comic.length(); };

void ComicViewer::load() {
	wxProgressDialog dialog(
		"Loading Comic", "Loading Pages", comic.length() * 2, this);
	imageViewers.reserve(comic.length());

	comic.load([&](int i) { dialog.Update(i + 1); });
	for (auto i = 0; i < comic.length(); ++i) {
		const auto& page = comic.pages[i];
		imageViewers.push_back(new ImageViewer(this, page.string()));
		sizer->Add(imageViewers.back(), 1, wxEXPAND);
		dialog.Update(i + 1 + comic.length(), "Sorting Pages");
		sizer->Show(size_t(i), false);
	}
	sizer->Show(size_t(0), true);
	Layout();
}

void ComicViewer::HandleInput(Navigation input) {
	auto dir = Navigation::NoOp;
	switch (input) {
		case Navigation::NextView:
		case Navigation::PreviousView:
			dir = imageViewers[index]->MoveViewport(input);
			break;
		default:
			dir = input;
	}
	auto nextIndex = index;
	if (dir == Navigation::NextPage) {
		nextIndex = std::min(index + 1, int(comic.pages.size()) - 1);
	} else if (dir == Navigation::PreviousPage) {
		nextIndex = std::max(index - 1, 0);
	}

	if (nextIndex != index) {
		auto v = imageViewers[index]->GetViewport();

		if (nextIndex > index) {
			v.Translate(-v.GetLeftTop());
		} else {
			const auto& is = imageViewers[index]->GetImageSize();
			v.Translate(wxPoint2DDouble(is.GetX(), is.GetY()) + v.GetLeftTop());
		}

		imageViewers[nextIndex]->SetViewport(v);
		sizer->Show(size_t(nextIndex), true);
		sizer->Show(size_t(index), false);
		index = nextIndex;
		Layout();
	}
}
