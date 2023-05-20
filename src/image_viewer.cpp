#include "image_viewer.hpp"

#include <algorithm>
#include <cmath>

#include "image_viewer.hpp"
#include "viewport.hpp"

using std::cout;
#define dbg(X) #X, "=", (X)
inline void print() {}
template <typename T> void print(T t) { cout << t; }
template <typename T, typename... Args> void print(T t, Args... args) {
	cout << t << " ";
	print(args...);
}
template <typename T> inline void printC(T t) {
	for (auto& elem : t) print(elem, "");
	cout << std::endl;
}
#define println(...)                                                 \
	{                                                                \
		print(__FILE__ ":" + std::to_string(__LINE__), __VA_ARGS__); \
		cout << std::endl;                                           \
	}
#define printFuncCall println(__FUNCTION__, "called")

#include <wx/dcbuffer.h>

std::ostream& operator<<(std::ostream& os, const wxRect2DDouble& r) {
	return os << "[" << r.GetLeft() << "," << r.GetRight() << "]*["
			  << r.GetTop() << "," << r.GetBottom() << "]";
}

std::ostream& operator<<(std::ostream& os, const wxPoint2DDouble& p) {
	return os << "(" << p.m_x << "," << p.m_y << ")";
}

ImageViewer::ImageViewer(wxWindow* parent, const wxString& filePath)
	: wxPanel(parent, wxID_ANY) {
	SetSize(100, 100);

	// m_scaleToFit = false;
	panInProgress = false;
	firstPaint = true;

	rawBitmap = wxBitmap(filePath, wxBITMAP_TYPE_ANY);
	imageSize.Set(rawBitmap.GetWidth(), rawBitmap.GetHeight());

	Bind(wxEVT_PAINT, &ImageViewer::OnPaint, this);
	Bind(wxEVT_MOUSEWHEEL, &ImageViewer::OnMouseWheel, this);
	Bind(wxEVT_LEFT_DOWN, &ImageViewer::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &ImageViewer::OnLeftDClick, this);
	Bind(wxEVT_SIZE, &ImageViewer::OnSize, this);
	Bind(wxEVT_CHAR_HOOK, &ImageViewer::OnKeyDown, this);

	SetBackgroundStyle(wxBG_STYLE_PAINT);

	inProgressPanStartPoint = wxPoint(0, 0);
	inProgressPanVector = wxPoint2DDouble(0, 0);
	panInProgress = false;
}

void ImageViewer::OnSize(wxSizeEvent& event) {
	printFuncCall;
	// if (m_scaleToFit) ScaleToFit();

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

void ImageViewer::OnKeyDown(wxKeyEvent& event) {
	switch (event.GetKeyCode()) {
		case 314:
			// m_panVector.m_x--;
			break;
		case 315:
			// m_panVector.m_y--;
			break;
		case 316:
			// m_panVector.m_x++;
			break;
		case 317:
			// m_panVector.m_y++;
			break;
		case 45:
			// m_ZoomFactor *= 0.5;
			break;
		case 61:
			// m_ZoomFactor *= 2;
			break;
	}
	event.Skip();

	Refresh();
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

	// wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

	if (gc) {
		auto s = GetClientSize();

		OptimizeViewport();

		if (firstPaint) {
			drawBitmap = gc->CreateBitmap(rawBitmap);
			viewport = Viewport(s.GetWidth(), 0, s.GetWidth(), s.GetHeight());
			firstPaint = false;
		}

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
