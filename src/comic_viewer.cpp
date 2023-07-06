#include "comic_viewer.hpp"

#include <wx/dcbuffer.h>
#include <wx/numdlg.h>
#include <wx/progdlg.h>

#include "fuzzy.hpp"
#include "util.hpp"
#include "wxUtil.hpp"

ComicViewer::ComicViewer(wxWindow* parent, Comic& comic)
	: wxPanel(parent), comic(comic), index(0), animation(AnimationType::None) {
	Bind(wxEVT_PAINT, &ComicViewer::OnPaint, this);
	Bind(wxEVT_MOUSEWHEEL, &ComicViewer::OnMouseWheel, this);
	Bind(wxEVT_LEFT_DOWN, &ComicViewer::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &ComicViewer::OnLeftDClick, this);
	Bind(wxEVT_SIZE, &ComicViewer::OnSize, this);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(wxColour(25, 25, 25, 1));
}

void ComicViewer::load() {
	wxProgressDialog dialog(
		"Loading Comic", "Loading Pages", comic.length(), this);
	comic.load([&](int i) { dialog.Update(i + 1); });
	for (auto& page : comic.pages) { pool.addImage(page); }
}

bool ComicViewer::verify(const wxGraphicsContext* gc, int i) {
	if (i < 0 || i >= comic.length()) { return false; }
	return true;
}

void ComicViewer::OnPaint(wxPaintEvent& event) {
	if (comic.pages.empty()) { return; }

	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	// direct2d renderer
	wxGraphicsRenderer* d2dr = wxGraphicsRenderer::GetDirect2DRenderer();
	wxGraphicsContext* gc = d2dr->CreateContext(dc);

	if (gc) {
		const auto& cw = GetClientSize().GetWidth();
		const auto& ch = GetClientSize().GetHeight();
		const auto& iw = pool.size(index).GetWidth();
		const auto& ih = pool.size(index).GetHeight();

		if (viewport.IsEmpty()) {
			viewport = Viewport(0, 0, cw, ch);
			NextZoom(wxPoint());
			NextZoom(wxPoint());
		}

		OptimizeViewport();

		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

		const auto totalPan = viewport.GetLeftTop() + inProgressPanVector;

		const auto zoom = GetZoom();

		gc->Scale(zoom, zoom);
		gc->Translate(-totalPan.m_x, -totalPan.m_y);

		gc->DrawBitmap(pool.bitmap(index), 0, 0, iw, ih);

		gc->Translate(totalPan.m_x, totalPan.m_y);
		gc->Scale(1.0 / zoom, 1.0 / zoom);
		drawBottomText(
			std::to_string(index + 1) + "/" + std::to_string(comic.length()),
			gc, cw, ch);
		delete gc;
	}
}

void ComicViewer::OnSize(wxSizeEvent& event) {
	if (viewport.IsEmpty()) { return; }
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
	if (animation != AnimationType::None) { return; }
	auto dir = Navigation::NoOp;
	wxPoint2DDouble delta;
	auto nextIndex = index;

	switch (input) {
		case Navigation::NextView:
		case Navigation::PreviousView:
			std::tie(dir, delta) = ComputeMove(input);
			break;
		default:
			dir = input;
			break;
	}

	const auto& is =
		wxPoint2DDouble(pool.size(index).GetX(), pool.size(index).GetY());

	if (dir == Navigation::NextPage) {
		nextIndex = std::min(index + 1, comic.length() - 1);
	} else if (dir == Navigation::PreviousPage) {
		nextIndex = std::max(index - 1, 0);
	} else if (dir == Navigation::JumpToPage) {
		nextIndex = wxGetNumberFromUser(
						"Go To Page", "", "", index + 1, 1, comic.length()) -
					1;
		if (nextIndex < 0) { nextIndex = index; }
	}

	if (index != nextIndex) {
		if (nextIndex > index) {
			viewport.MoveLeftTopTo({0, 0});
		} else {
			const auto ns = pool.size(nextIndex);
			viewport.MoveRightBottomTo(
				wxPoint2DDouble(ns.GetWidth(), ns.GetHeight()));
		}
		index = nextIndex;
		Refresh();
	} else if (dir == Navigation::PreviousView || dir == Navigation::NextView) {
		const int MAX_DURATION_MS = 100;
		StartPan({}, PanSource::Animation);
		viewportAnimator.Start(
			MAX_DURATION_MS, {}, delta,	 //
			[this](auto v) { ProcessPan(v, true, PanSource::Animation); },
			[this]() { FinishPan(true, PanSource::Animation); });
	}
}

std::pair<Navigation, wxPoint2DDouble> ComicViewer::ComputeMove(
	Navigation direction) {
	const auto iw = pool.size(index).GetWidth();
	const auto ih = pool.size(index).GetHeight();
	if (direction == Navigation::NextView) {
		if (fuzzy::greater_equal(viewport.GetRight(), iw) &&
			fuzzy::greater_equal(viewport.GetBottom(), ih)) {
			return {Navigation::NextPage, {}};
		}
		if (fuzzy::less(viewport.GetRight(), iw)) {
			return {
				Navigation::NextView,
				{std::min(viewport.W() / 2, iw - viewport.GetRight()), 0}};
		}
		return {
			Navigation::NextView,
			{-std::max(0.0, iw - viewport.W()),
			 std::min(viewport.H() / 2, ih - viewport.GetBottom())}};
	} else if (direction == Navigation::PreviousView) {
		if (fuzzy::less_equal(viewport.GetLeft(), 0) &&
			fuzzy::less_equal(viewport.GetTop(), 0)) {
			return {Navigation::PreviousPage, {}};
		}
		if (fuzzy::greater(viewport.GetLeft(), 0)) {
			return {
				Navigation::PreviousView,
				{-std::min(viewport.W() / 2, viewport.GetLeft()), 0}};
		}
		return {
			Navigation::PreviousView,
			{std::max(0.0, iw - viewport.W()),
			 -std::min(viewport.H() / 2, viewport.GetTop())}};
	}
	return {Navigation::NoOp, {}};
}

void ComicViewer::NextZoom(const wxPoint& pt) {
	auto cs = GetClientSize();
	auto currentZoom = GetZoom();

	std::vector<double> validZooms = {
		double(cs.GetWidth()) / pool.size(index).GetWidth(),
		double(cs.GetHeight()) / pool.size(index).GetHeight(),
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
	return GetClientSize().GetWidth() / viewport.W();
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
	const auto& is = pool.size(index);

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
	FinishPan(false, PanSource::Mouse);

	auto change = (double)event.GetWheelRotation() / event.GetWheelDelta();

	viewport.ScaleAtPoint(
		MapClientToViewport(event.GetPosition()), change > 0 ? 0.8 : 1.25);

	Refresh();
	event.Skip();
}

void ComicViewer::OnLeftDClick(wxMouseEvent& event) {
	NextZoom(event.GetPosition());
	Refresh();
	event.Skip();
}

void ComicViewer::OnLeftDown(wxMouseEvent& event) {
	StartPan(MapClientToViewport(event.GetPosition()), PanSource::Mouse);
}

void ComicViewer::OnMotion(wxMouseEvent& event) {
	ProcessPan(
		MapClientToViewport(event.GetPosition()), true, PanSource::Mouse);
}

void ComicViewer::OnLeftUp(wxMouseEvent& event) {
	ProcessPan(
		MapClientToViewport(event.GetPosition()), false, PanSource::Mouse);
	FinishPan(true, PanSource::Mouse);
}

void ComicViewer::OnCaptureLost(wxMouseCaptureLostEvent&) {
	FinishPan(true, PanSource::Mouse);
}

void ComicViewer::StartPan(const wxPoint2DDouble& pt, PanSource src) {
	if (animation != AnimationType::None) { return; }
	switch (src) {
		case PanSource::Mouse:
			animation = AnimationType::Pan;
			SetCursor(wxCursor(wxCURSOR_HAND));
			Bind(wxEVT_LEFT_UP, &ComicViewer::OnLeftUp, this);
			Bind(wxEVT_MOTION, &ComicViewer::OnMotion, this);
			Bind(wxEVT_MOUSE_CAPTURE_LOST, &ComicViewer::OnCaptureLost, this);
			CaptureMouse();
			break;
		case PanSource::Animation:
			animation = AnimationType::View;
			break;
	}
	inProgressPanStartPoint = pt;
	inProgressPanVector = wxPoint2DDouble(0, 0);
}

void ComicViewer::ProcessPan(
	const wxPoint2DDouble& pt, bool refresh, PanSource src) {
	inProgressPanVector = pt - inProgressPanStartPoint;
	if (src == PanSource::Mouse) { inProgressPanVector = -inProgressPanVector; }
	if (refresh) { Refresh(); }
}

void ComicViewer::FinishPan(bool refresh, PanSource src) {
	if (animation == AnimationType::None) { return; }
	switch (src) {
		case PanSource::Mouse:
			if (animation != AnimationType::Pan) { return; }
			SetCursor(wxNullCursor);
			if (HasCapture()) { ReleaseMouse(); }
			Unbind(wxEVT_LEFT_UP, &ComicViewer::OnLeftUp, this);
			Unbind(wxEVT_MOTION, &ComicViewer::OnMotion, this);
			Unbind(wxEVT_MOUSE_CAPTURE_LOST, &ComicViewer::OnCaptureLost, this);
			break;
		case PanSource::Animation:
			if (animation != AnimationType::View) { return; }
			break;
	}
	viewport.SetCentre(viewport.GetCentre() + inProgressPanVector);
	inProgressPanVector = wxPoint2DDouble(0, 0);
	animation = AnimationType::None;
	if (refresh) { Refresh(); }
}
