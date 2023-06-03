#include <wx/dcbuffer.h>

#include <algorithm>
#include <cmath>
#include <fuzzy.hpp>
#include <image_viewer.hpp>
#include <util.hpp>
#include <viewport.hpp>

std::ostream& operator<<(std::ostream& os, const wxRect2DDouble& r) {
	return os << "[" << r.GetLeft() << "," << r.GetRight() << "]*["
			  << r.GetTop() << "," << r.GetBottom() << "]";
}

std::ostream& operator<<(std::ostream& os, const wxPoint2DDouble& p) {
	return os << "(" << p.m_x << "," << p.m_y << ")";
}

ImageViewer::ImageViewer(wxWindow* parent, const wxString& filePath)
	: wxPanel(parent, wxID_ANY),
	  inProgressPanVector(),
	  inProgressPanStartPoint(),
	  panInProgress(false),
	  firstPaint(true),
	  filePath(filePath) {
	Bind(wxEVT_PAINT, &ImageViewer::OnPaint, this);
	Bind(wxEVT_MOUSEWHEEL, &ImageViewer::OnMouseWheel, this);
	Bind(wxEVT_LEFT_DOWN, &ImageViewer::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &ImageViewer::OnLeftDClick, this);
	Bind(wxEVT_SIZE, &ImageViewer::OnSize, this);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(wxColour(25, 25, 25, 1));
	Show(false);
}

void ImageViewer::OnSize(wxSizeEvent& event) {
	printFuncCall;

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

void ImageViewer::OptimizeViewport() {
	const auto& vs = viewport.GetSize();
	const auto& is = imageSize;

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
	} else if (viewport.GetRight() > imageSize.x) {
		viewport.Translate((imageSize.x - viewport.GetRight()), 0);
	}

	if (vs.GetHeight() > is.GetHeight()) {
		viewport.SetCentre({viewport.GetCentre().m_x, is.GetHeight() / 2.0});
	} else if (viewport.GetTop() < 0) {
		viewport.Translate(0, -viewport.GetTop());
	} else if (viewport.GetBottom() > imageSize.y) {
		viewport.Translate(0, (imageSize.y - viewport.GetBottom()));
	}
}

void ImageViewer::OnPaint(wxPaintEvent& event) {
	printFuncCall;
	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	// direct2d renderer
	wxGraphicsRenderer* d2dr = wxGraphicsRenderer::GetDirect2DRenderer();
	wxGraphicsContext* gc = d2dr->CreateContext(dc);

	if (gc) {
		auto s = GetClientSize();

		if (firstPaint) {
			rawBitmap = wxBitmap(filePath, wxBITMAP_TYPE_ANY);
			imageSize = wxSize(rawBitmap.GetWidth(), rawBitmap.GetHeight());
			drawBitmap = gc->CreateBitmap(rawBitmap);

			if (viewport.IsEmpty()) {
				viewport = Viewport(0, 0, s.GetWidth(), s.GetHeight());
			}

			firstPaint = false;
		}

		OptimizeViewport();

		wxPoint2DDouble totalPan = viewport.GetLeftTop() + inProgressPanVector;

		auto zoom = GetZoom();

		gc->Scale(zoom, zoom);
		gc->Translate(-totalPan.m_x, -totalPan.m_y);

		gc->DrawBitmap(
			drawBitmap, 0, 0, imageSize.GetWidth(), imageSize.GetHeight());

		delete gc;
	}
}

wxPoint2DDouble ImageViewer::MapClientToViewport(const wxPoint& pt) {
	auto cs = GetClientSize();
	auto vs = viewport.GetSize();
	auto vp = viewport.GetPosition();
	return wxPoint2DDouble(
			   double(pt.x * vs.x) / cs.x, double(pt.y * vs.y) / cs.y) +
		   vp;
}

void ImageViewer::OnMouseWheel(wxMouseEvent& event) {
	printFuncCall;
	if (panInProgress) {
		FinishPan(false);
	}

	auto change = (double)event.GetWheelRotation() / event.GetWheelDelta();

	viewport.ScaleAtPoint(
		MapClientToViewport(event.GetPosition()), change > 0 ? 0.8 : 1.25);

	Refresh();

	event.Skip();
}

void ImageViewer::ProcessPan(const wxPoint2DDouble& pt, bool refresh) {
	inProgressPanVector = inProgressPanStartPoint - pt;
	if (refresh) {
		Refresh();
	}
}

void ImageViewer::FinishPan(bool refresh) {
	if (panInProgress) {
		SetCursor(wxNullCursor);

		if (HasCapture()) {
			ReleaseMouse();
		}

		Unbind(wxEVT_LEFT_UP, &ImageViewer::OnLeftUp, this);
		Unbind(wxEVT_MOTION, &ImageViewer::OnMotion, this);
		Unbind(wxEVT_MOUSE_CAPTURE_LOST, &ImageViewer::OnCaptureLost, this);

		viewport.SetCentre(viewport.GetCentre() + inProgressPanVector);
		inProgressPanVector = wxPoint2DDouble(0, 0);
		panInProgress = false;

		if (refresh) {
			Refresh();
		}
	}
}

void ImageViewer::OnLeftDClick(wxMouseEvent& event) {
	printFuncCall;

	auto cs = GetClientSize();

	auto currentZoom = GetZoom();

	std::vector<double> validZooms = {
		double(cs.GetWidth()) / imageSize.GetWidth(),
		double(cs.GetHeight()) / imageSize.GetHeight(),
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

	viewport.ScaleAtPoint(
		MapClientToViewport(event.GetPosition()), currentZoom / nextZoom);

	Refresh();
	event.Skip();
}

void ImageViewer::OnLeftDown(wxMouseEvent& event) {
	printFuncCall;
	wxCursor cursor(wxCURSOR_HAND);
	SetCursor(cursor);

	inProgressPanStartPoint = MapClientToViewport(event.GetPosition());
	inProgressPanVector = wxPoint2DDouble(0, 0);
	panInProgress = true;

	Bind(wxEVT_LEFT_UP, &ImageViewer::OnLeftUp, this);
	Bind(wxEVT_MOTION, &ImageViewer::OnMotion, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &ImageViewer::OnCaptureLost, this);

	CaptureMouse();
}

void ImageViewer::OnMotion(wxMouseEvent& event) {
	printFuncCall;
	ProcessPan(MapClientToViewport(event.GetPosition()), true);
}

void ImageViewer::OnLeftUp(wxMouseEvent& event) {
	printFuncCall;
	ProcessPan(MapClientToViewport(event.GetPosition()), false);
	FinishPan(true);
}

void ImageViewer::OnCaptureLost(wxMouseCaptureLostEvent&) {
	printFuncCall;
	FinishPan(true);
}

double ImageViewer::GetZoom() {
	return GetClientSize().GetWidth() / viewport.m_width;
}

Navigation ImageViewer::MoveViewport(Navigation direction) {
	if (direction == Navigation::NextView) {
		if (fuzzy::greater_equal(viewport.GetRight(), imageSize.GetWidth()) &&
			fuzzy::greater_equal(viewport.GetBottom(), imageSize.GetHeight())) {
			return Navigation::NextPage;
		}
		if (fuzzy::less(viewport.GetRight(), imageSize.GetWidth())) {
			viewport.Translate(
				std::min(
					viewport.m_width / 2,
					imageSize.GetWidth() - viewport.GetRight()),
				0);
		} else {
			viewport.Translate(
				-viewport.GetLeft(),
				std::min(
					viewport.m_height / 2,
					imageSize.GetHeight() - viewport.GetBottom()));
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
				imageSize.GetWidth() - viewport.m_width,
				-std::min(viewport.m_height / 2, viewport.GetTop()));
		}
	}
	if (direction != Navigation::NoOp) {
		Refresh();
	}
	return Navigation::NoOp;
}

void ImageViewer::SetViewport(const Viewport& v) {
	printFuncCall;
	viewport = v;
}

const Viewport& ImageViewer::GetViewport() const { return viewport; }
const wxSize& ImageViewer::GetImageSize() const { return imageSize; }
