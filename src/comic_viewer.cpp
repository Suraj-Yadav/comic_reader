#include "comic_viewer.hpp"

#include <wx/dcbuffer.h>
#include <wx/progdlg.h>

#include "fuzzy.hpp"
#include "util.hpp"

ComicViewer::ComicViewer(wxWindow* parent, Comic& comic)
	: wxPanel(parent),
	  comic(comic),
	  index(0),
	  bitmaps(comic.length()),
	  sizes(comic.length()),
	  gBitmaps(comic.length()) {
	Bind(wxEVT_PAINT, &ComicViewer::OnPaint, this);
	Bind(wxEVT_MOUSEWHEEL, &ComicViewer::OnMouseWheel, this);
	Bind(wxEVT_LEFT_DOWN, &ComicViewer::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &ComicViewer::OnLeftDClick, this);
	Bind(wxEVT_SIZE, &ComicViewer::OnSize, this);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(wxColour(25, 25, 25, 1));
}

int ComicViewer::length() const { return comic.length(); };

void ComicViewer::load() {
	wxProgressDialog dialog(
		"Loading Comic", "Loading Pages", comic.length(), this);
	comic.load([&](int i) { dialog.Update(i + 1); });
}

void ComicViewer::verify(const wxGraphicsContext* gc, int i) {
	if (!bitmaps[i].IsOk()) {
		bitmaps[i].LoadFile(comic.pages[i].string(), wxBITMAP_TYPE_ANY);
		const auto& b = bitmaps[i];
		sizes[i].Set(b.GetWidth(), b.GetHeight());
		gBitmaps[i] = gc->CreateBitmap(b);
	}
}

void ComicViewer::OnPaint(wxPaintEvent& event) {
	if (comic.pages.empty()) {
		return;
	}

	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	// direct2d renderer
	wxGraphicsRenderer* d2dr = wxGraphicsRenderer::GetDirect2DRenderer();
	wxGraphicsContext* gc = d2dr->CreateContext(dc);

	if (gc) {
		auto cs = GetClientSize();
		verify(gc, index);
		if (viewport.IsEmpty()) {
			viewport = Viewport(0, 0, cs.GetWidth(), cs.GetHeight());
			NextZoom(wxPoint());
			NextZoom(wxPoint());
		}

		OptimizeViewport();

		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

		wxPoint2DDouble totalPan = viewport.GetLeftTop() + inProgressPanVector;

		auto zoom = GetZoom();

		gc->Scale(zoom, zoom);
		gc->Translate(-totalPan.m_x, -totalPan.m_y);

		gc->DrawBitmap(
			gBitmaps[index], 0, 0, sizes[index].GetWidth(),
			sizes[index].GetHeight());

		delete gc;
	}
}

void ComicViewer::OnSize(wxSizeEvent& event) {
	if (viewport.IsEmpty()) {
		return;
	}
	auto const& cs = GetClientSize();
	auto const& vs = viewport.GetSize();

	const double ca = (double)cs.x / cs.y;
	const double va = (double)vs.x / vs.y;

	if (ca > va) {
		viewport.Inset((vs.x - ca * vs.y) / 2, 0);
	} else if (ca < va) {
		viewport.Inset(0, (vs.y - vs.x / ca) / 2);
	}

	Refresh();
	event.Skip();
}

void ComicViewer::HandleInput(Navigation input) {
	auto dir = Navigation::NoOp;
	switch (input) {
		case Navigation::NextView:
		case Navigation::PreviousView:
			dir = MoveViewport(input);
			break;
		default:
			dir = input;
	}
	auto nextIndex = index;
	if (dir == Navigation::NextPage) {
		nextIndex = std::min(index + 1, comic.length() - 1);
	} else if (dir == Navigation::PreviousPage) {
		nextIndex = std::max(index - 1, 0);
	}
	if (nextIndex != index) {
		if (nextIndex > index) {
			viewport.Translate(-viewport.GetLeftTop());
		} else {
			const auto& is = sizes[index];
			viewport.Translate(
				wxPoint2DDouble(is.GetX(), is.GetY()) + viewport.GetLeftTop());
		}
		index = nextIndex;
	}
	Refresh();
}

Navigation ComicViewer::MoveViewport(Navigation direction) {
	const auto iw = sizes[index].GetWidth();
	const auto ih = sizes[index].GetHeight();
	if (direction == Navigation::NextView) {
		if (fuzzy::greater_equal(viewport.GetRight(), iw) &&
			fuzzy::greater_equal(viewport.GetBottom(), ih)) {
			return Navigation::NextPage;
		}
		if (fuzzy::less(viewport.GetRight(), iw)) {
			viewport.Translate(
				std::min(viewport.m_width / 2, iw - viewport.GetRight()), 0);
		} else {
			viewport.Translate(
				-viewport.GetLeft(),
				std::min(viewport.m_height / 2, ih - viewport.GetBottom()));
		}
	} else if (direction == Navigation::PreviousView) {
		if (fuzzy::less_equal(viewport.GetLeft(), 0) &&
			fuzzy::less_equal(viewport.GetTop(), 0)) {
			return Navigation::PreviousPage;
		}
		if (fuzzy::greater(viewport.GetLeft(), 0)) {
			viewport.Translate(
				-std::min(viewport.m_width / 2, viewport.GetLeft()), 0);
		} else {
			viewport.Translate(
				iw - viewport.m_width,
				-std::min(viewport.m_height / 2, viewport.GetTop()));
		}
	}
	return Navigation::NoOp;
}

void ComicViewer::NextZoom(const wxPoint& pt) {
	auto cs = GetClientSize();
	auto currentZoom = GetZoom();

	std::vector<double> validZooms = {
		double(cs.GetWidth()) / sizes[index].GetWidth(),
		double(cs.GetHeight()) / sizes[index].GetHeight(),
		1.0,
	};
	std::sort(validZooms.begin(), validZooms.end());

	auto nextZoom = validZooms[0];

	for (auto z : validZooms) {
		bool isLess = currentZoom < z;
		bool isAlmostEqual = std::fabs(currentZoom - z) <= 0.001;
		if (isLess && !isAlmostEqual) {
			nextZoom = z;
			break;
		}
	}

	viewport.ScaleAtPoint(MapClientToViewport(pt), currentZoom / nextZoom);
}

double ComicViewer::GetZoom() {
	return GetClientSize().GetWidth() / viewport.m_width;
}

wxPoint2DDouble ComicViewer::MapClientToViewport(const wxPoint& pt) {
	auto cs = GetClientSize();
	auto vs = viewport.GetSize();
	auto vp = viewport.GetPosition();
	return wxPoint2DDouble(
			   double(pt.x * vs.x) / cs.x, double(pt.y * vs.y) / cs.y) +
		   vp;
}

void ComicViewer::OptimizeViewport() {
	const auto& vs = viewport.GetSize();
	const auto& is = sizes[index];

	// If there is extra space in both directions, scale viewport down to
	// closest edge
	if (vs.GetWidth() > is.GetWidth() && vs.GetHeight() > is.GetHeight()) {
		auto widthScale = double(is.GetWidth()) / vs.GetWidth();
		auto heightScale = double(is.GetHeight()) / vs.GetHeight();
		viewport.ScaleAtPoint(
			viewport.GetCentre(), std::max(widthScale, heightScale));
	}

	if (vs.GetWidth() > is.GetWidth()) {
		viewport.SetCentre({is.GetWidth() / 2.0, viewport.GetCentre().m_y});
	} else if (viewport.GetLeft() < 0) {
		viewport.Translate(-viewport.GetLeft(), 0);
	} else if (viewport.GetRight() > is.x) {
		viewport.Translate((is.x - viewport.GetRight()), 0);
	}

	if (vs.GetHeight() > is.GetHeight()) {
		viewport.SetCentre({viewport.GetCentre().m_x, is.GetHeight() / 2.0});
	} else if (viewport.GetTop() < 0) {
		viewport.Translate(0, -viewport.GetTop());
	} else if (viewport.GetBottom() > is.y) {
		viewport.Translate(0, (is.y - viewport.GetBottom()));
	}
}

void ComicViewer::OnMouseWheel(wxMouseEvent& event) {
	if (panInProgress) {
		FinishPan(false);
	}

	auto change = (double)event.GetWheelRotation() / event.GetWheelDelta();

	viewport.ScaleAtPoint(
		MapClientToViewport(event.GetPosition()), change > 0 ? 0.8 : 1.25);

	Refresh();
	event.Skip();
}

void ComicViewer::ProcessPan(const wxPoint2DDouble& pt, bool refresh) {
	inProgressPanVector = inProgressPanStartPoint - pt;
	if (refresh) {
		Refresh();
	}
}

void ComicViewer::FinishPan(bool refresh) {
	if (panInProgress) {
		SetCursor(wxNullCursor);

		if (HasCapture()) {
			ReleaseMouse();
		}

		Unbind(wxEVT_LEFT_UP, &ComicViewer::OnLeftUp, this);
		Unbind(wxEVT_MOTION, &ComicViewer::OnMotion, this);
		Unbind(wxEVT_MOUSE_CAPTURE_LOST, &ComicViewer::OnCaptureLost, this);

		viewport.SetCentre(viewport.GetCentre() + inProgressPanVector);
		inProgressPanVector = wxPoint2DDouble(0, 0);
		panInProgress = false;

		if (refresh) {
			Refresh();
		}
	}
}

void ComicViewer::OnLeftDClick(wxMouseEvent& event) {
	NextZoom(event.GetPosition());
	Refresh();
	event.Skip();
}

void ComicViewer::OnLeftDown(wxMouseEvent& event) {
	wxCursor cursor(wxCURSOR_HAND);
	SetCursor(cursor);

	inProgressPanStartPoint = MapClientToViewport(event.GetPosition());
	inProgressPanVector = wxPoint2DDouble(0, 0);
	panInProgress = true;

	Bind(wxEVT_LEFT_UP, &ComicViewer::OnLeftUp, this);
	Bind(wxEVT_MOTION, &ComicViewer::OnMotion, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &ComicViewer::OnCaptureLost, this);

	CaptureMouse();
}

void ComicViewer::OnMotion(wxMouseEvent& event) {
	ProcessPan(MapClientToViewport(event.GetPosition()), true);
}

void ComicViewer::OnLeftUp(wxMouseEvent& event) {
	ProcessPan(MapClientToViewport(event.GetPosition()), false);
	FinishPan(true);
}

void ComicViewer::OnCaptureLost(wxMouseCaptureLostEvent&) { FinishPan(true); }
