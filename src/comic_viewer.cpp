#include "comic_viewer.hpp"

#include <wx/colour.h>
#include <wx/config.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/numdlg.h>
#include <wx/progdlg.h>

#include <cmath>
#include <format>

#include "fuzzy.hpp"
#include "image_utils.hpp"
#include "util.hpp"
#include "wxUtil.hpp"

const wxColour BACKGROUND_COLOR = wxColour(25, 25, 25);

PageView getView(const Comic& comic) {
	if (comic.source().titleId == -1) { return PageView::SinglePage; }
	const auto KEY = std::format("/Comic/{}/View", comic.source().titleId);
	wxConfigBase* cfg = wxConfigBase::Get();
	return static_cast<PageView>(cfg->Read(KEY, 0L));
}

void saveView(const Comic& comic, PageView view) {
	if (comic.source().titleId == -1) { return; }
	const auto KEY = std::format("/Comic/{}/View", comic.source().titleId);
	wxConfigBase* cfg = wxConfigBase::Get();
	cfg->Write(KEY, static_cast<long>(view));
	cfg->Flush();
}

ComicViewer::ComicViewer(wxWindow* parent, Comic& comic)
	: wxPanel(parent), comic(comic), index(0), animation(AnimationType::None) {
	Bind(wxEVT_PAINT, &ComicViewer::OnPaint, this);
	Bind(wxEVT_MOUSEWHEEL, &ComicViewer::OnMouseWheel, this);
	Bind(wxEVT_LEFT_DOWN, &ComicViewer::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &ComicViewer::OnLeftDClick, this);
	Bind(wxEVT_SIZE, &ComicViewer::OnSize, this);
	Bind(wxEVT_CLOSE_WINDOW, &ComicViewer::OnClose, this);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(BACKGROUND_COLOR);
}

void ComicViewer::load() {
	wxProgressDialog dialog(
		"Loading Comic", "Loading Pages", comic.length(), this);
	comic.load([&](int i) { dialog.Update(i + 1); });
	for (auto& page : comic.pages) { pool.addImage(page); }
	pool.update(getView(comic));
}

void ComicViewer::OnClose(wxCloseEvent& /* event */) {
	comic.unload();
	pool.clear();
}

bool ComicViewer::verify(const wxGraphicsContext* /* gc */, int i) {
	return i >= 0 && i < pool.length();
}

void ComicViewer::OnPaint(wxPaintEvent& /* event */) {
	if (pool.empty()) { return; }
	if (comic.pages.empty()) { return; }

	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	// direct2d renderer
	wxGraphicsRenderer* d2dr = wxGraphicsRenderer::GetDefaultRenderer();
	wxGraphicsContext* gc = d2dr->CreateContext(dc);

	if (gc) {
		const auto& cw = GetClientSize().GetWidth();
		const auto& ch = GetClientSize().GetHeight();

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

		pool.draw(*gc, index);

		gc->Translate(totalPan.m_x, totalPan.m_y);
		gc->Scale(1.0 / zoom, 1.0 / zoom);

		drawBottomText(
			std::format("{}%", static_cast<int>(100 * GetZoom())), gc, cw, ch,
			Position::LEFT);
		drawBottomText(
			std::format("{}/{}", index + 1, pool.length()), gc, cw, ch);
		{
			std::string text;
			switch (pool.pageView()) {
				case PageView::SinglePage:
					text = "Single Page";
					break;
				case PageView::DoublePageLR:
					text = "Double Page L to R";
					break;
				case PageView::DoublePageOddLR:
					text = "Double Page Odd L to R";
					break;
				case PageView::DoublePageRL:
					text = "Double Page R to L";
					break;
				case PageView::DoublePageOddRL:
					text = "Double Page Odd R to L";
					break;
			}
			drawBottomText(text, gc, cw, ch, Position::RIGHT);
		}
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
		nextIndex = std::min(index + 1, pool.length() - 1);
	} else if (dir == Navigation::PreviousPage) {
		nextIndex = std::max(index - 1, 0);
	} else if (dir == Navigation::JumpToPage) {
		nextIndex = wxGetNumberFromUser(
						"Go To Page", "", "", index + 1, 1, pool.length()) -
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
	} else if (index == (pool.length() - 1) && input == Navigation::NextView) {
		comic.markAsRead();
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

void ComicViewer::NextPageMode() {
	auto mode = pool.pageView();
	switch (mode) {
		case PageView::SinglePage:
			mode = PageView::DoublePageLR;
			break;
		case PageView::DoublePageLR:
			mode = PageView::DoublePageOddLR;
			break;
		case PageView::DoublePageOddLR:
			mode = PageView::DoublePageRL;
			break;
		case PageView::DoublePageRL:
			mode = PageView::DoublePageOddRL;
			break;
		case PageView::DoublePageOddRL:
			mode = PageView::SinglePage;
			break;
	}
	pool.update(mode);
	saveView(comic, mode);
	Refresh();
}

void ComicViewer::NextZoom(const wxPoint& pt) {
	auto cs = GetClientSize();
	auto currentZoom = GetZoom();

	std::vector<double> validZooms = {
		double(cs.GetWidth()) / pool.size(index).GetWidth(),
		0.5 * double(cs.GetWidth()) / pool.size(index).GetWidth(),
		0.75 * double(cs.GetWidth()) / pool.size(index).GetWidth(),
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
	Refresh();
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
