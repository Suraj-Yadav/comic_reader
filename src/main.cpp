#include <wx/app.h>
#include <wx/frame.h>
#include <wx/msgdlg.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <vector>

#include "comic.hpp"
#include "comic_gallery.hpp"
#include "comic_viewer.hpp"
#include "util.hpp"

class MyApp : public wxApp {
   public:
	bool OnInit() override;
	int OnExit() override;
};

wxIMPLEMENT_APP(MyApp);

class MyFrame : public wxFrame {
	ComicGallery* comicGallery;
	ComicViewer* comicViewer;
	wxSizer* sizer;

	void OnKeyDown(wxKeyEvent& event);

   public:
	MyFrame();
	void LoadComic();
};

int MyApp::OnExit() {
	std::filesystem::remove_all(cacheDirectory);
	return 0;
}

bool MyApp::OnInit() {
	::wxInitAllImageHandlers();

	auto frame = new MyFrame();
	frame->SetIcon(wxICON(app_icon));
	frame->Show(true);
	frame->LoadComic();
	return true;
}

MyFrame::MyFrame()
	: wxFrame(nullptr, wxID_ANY, ""),
	  comicGallery(nullptr),
	  comicViewer(nullptr) {
	Bind(wxEVT_CHAR_HOOK, &MyFrame::OnKeyDown, this);
}

void MyFrame::OnKeyDown(wxKeyEvent& event) {
	if (comicViewer != nullptr) {
		switch (event.GetKeyCode()) {
			case WXK_LEFT:
				comicViewer->HandleInput(Navigation::PreviousView);
				break;
			case WXK_RIGHT:
				comicViewer->HandleInput(Navigation::NextView);
				break;
			case WXK_ESCAPE:
				sizer->Remove(1);
				sizer->Show(size_t(0));
				comicViewer->Destroy();
				comicViewer = nullptr;
				comicGallery->currentComic().unload();
				Layout();
				SetTitle("Select Comic");
				break;
			case 'G':
				if (event.CmdDown() || event.ControlDown()) {
					comicViewer->HandleInput(Navigation::JumpToPage);
				}
				break;
		}
	} else if (comicGallery != nullptr) {
		switch (event.GetKeyCode()) {
			case WXK_LEFT:
				comicGallery->HandleInput(Navigation::PreviousComic);
				break;
			case WXK_RIGHT:
				comicGallery->HandleInput(Navigation::NextComic);
				break;
			case WXK_RETURN:
				try {
					comicGallery->currentComic().unload();
					comicViewer =
						new ComicViewer(this, comicGallery->currentComic());
					comicViewer->load();
				} catch (const std::exception& e) {
					comicViewer->Destroy();
					comicViewer = nullptr;
					wxMessageBox(e.what(), "Error while opening comic");
					return;
				}
				SetTitle(comicGallery->currentComic().getName());
				sizer->Add(comicViewer, wxSizerFlags(1).Expand());
				sizer->Hide(size_t(0));
				comicViewer->SetFocus();
				comicViewer->SetFocusFromKbd();
				Layout();
				break;
			default:
				if (std::isalpha(std::clamp(event.GetKeyCode(), -1, 255))) {
					comicGallery->HandleInput(
						Navigation::JumpToComic, event.GetKeyCode());
				}
				break;
		}
	}
	event.Skip();
}

void MyFrame::LoadComic() {
	std::vector<std::filesystem::path> paths;
	{
		wxFileDialog openFileDialog(
			this, "Open Comic", "", "",
			"Comic Files (*.cbr;*.cbz)|*.cbr;*.cbz|"
			"All files|*.*",
			wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

		if (openFileDialog.ShowModal() == wxID_CANCEL) { return; }
		wxArrayString selectedFiles;
		openFileDialog.GetPaths(selectedFiles);

		for (auto& e : selectedFiles) { paths.push_back(e.ToStdString()); }
	}

	sizer = new wxBoxSizer(wxVERTICAL);
	comicGallery = new ComicGallery(this, paths);
	sizer->Add(comicGallery, wxSizerFlags(1).Expand());

	SetSizer(sizer);
	Layout();
}
