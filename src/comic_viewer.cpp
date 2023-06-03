#include <wx/progdlg.h>

#include <comic_viewer.hpp>
#include <image_viewer.hpp>
#include <util.hpp>

ComicViewer::ComicViewer(
	wxWindow* parent, const std::filesystem::path& comicPath)
	: wxPanel(parent),
	  comic(comicPath),
	  index(0),
	  sizer(new wxBoxSizer(wxHORIZONTAL)) {
	SetSizer(sizer);
	Bind(wxEVT_CHAR_HOOK, &ComicViewer::OnKeyDown, this);
}

int ComicViewer::length() const { return comic.length(); };

void ComicViewer::load() {
	wxProgressDialog dialog(
		_("Please wait"), _("Loading Comic"), comic.length() + 1, this);
	imageViewers.reserve(comic.length());

	comic.load([&](int i) {
		imageViewers.push_back(
			new ImageViewer(this, comic.pages.back().string()));
		sizer->Add(imageViewers.back(), 1, wxEXPAND);
		dialog.Update(1 + imageViewers.size());
		if (i == 1) {
			sizer->Show(size_t(0), true);
			Layout();
		} else {
			sizer->Show(size_t(i - 1), false);
		}
		dialog.Update(i);
	});
	Layout();
}

void ComicViewer::OnKeyDown(wxKeyEvent& event) {
	auto nextIndex = index;
	auto dir = Navigation::NoOp;

	// println(dbg(event.GetKeyCode()));

	switch (event.GetKeyCode()) {
		case 314:
			dir = imageViewers[index]->MoveViewport(Navigation::PreviousView);
			break;
		case 316:
			dir = imageViewers[index]->MoveViewport(Navigation::NextView);
			break;
		case 315:
			// m_panVector.m_y--;
			break;
		case 317:
			// m_panVector.m_y++;
			break;
	}

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
	event.Skip();
}
