#include <wx/app.h>
#include <wx/config.h>
#include <wx/frame.h>
#include <wx/graphics.h>
#include <wx/msgdlg.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <filesystem>
#include <ranges>
#include <vector>

#include "comic.hpp"
#include "comic_gallery.hpp"
#include "comic_viewer.hpp"
#include "network.hpp"
#include "util.hpp"

enum class Options {
	NONE = wxID_NONE,
	DEFAULT = wxID_DEFAULT,
	LOAD_LOCAL_COMICS = wxID_HIGHEST + 1,
	LOAD_REMOTE_COMICS = wxID_HIGHEST + 2,
	SET_LOCAL_AS_DEFAULT = wxID_HIGHEST + 3,
	SET_REMOTE_AS_DEFAULT = wxID_HIGHEST + 4,
	SET_SERVER_ADDRESS = wxID_HIGHEST + 5,
};

class MyApp : public wxApp {
   public:
	bool OnInit() override;
	int OnExit() override;
};

wxIMPLEMENT_APP(MyApp);

class MyFrame : public wxFrame {
	ComicGallery* comicGallery = nullptr;
	ComicViewer* comicViewer = nullptr;
	wxSizer* sizer = nullptr;
	int lastKeyCode = 0;

	void OnKeyDown(wxKeyEvent& event);
	bool OpenComic();
	void CloseComic();
	void handleOptions(Options option);

   public:
	MyFrame();
	void LoadComics(const wxArrayString& args);
};

const auto DEFAULT_FRAME_TITLE = "Select Comic";

int MyApp::OnExit() {
	std::filesystem::remove_all(cacheDirectory);
	return 0;
}

bool MyApp::OnInit() {
	::wxInitAllImageHandlers();

	SetAppName("comic_reader");
	SetVendorName("suraj-yadav");

	auto* frame = new MyFrame();
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	frame->SetIcon(wxICON(app_icon));
#endif
	frame->Show(true);
	frame->LoadComics(argv.GetArguments());
	frame->SetTitle(DEFAULT_FRAME_TITLE);
	frame->ShowFullScreen(true, wxFULLSCREEN_ALL);
	return true;
}

MyFrame::MyFrame() : wxFrame(nullptr, wxID_ANY, "") {
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
				CloseComic();
				break;
			case 'Z':
			case WXK_UP:
				comicViewer->NextZoom(event.GetPosition());
				break;
			case 'G':
			case WXK_DOWN:
				if (lastKeyCode != event.GetKeyCode()) {
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
				if (!OpenComic()) { return; }
				break;
			case WXK_UP:
				if (lastKeyCode != event.GetKeyCode()) {
					handleOptions(Options::NONE);
				}
				break;
			case WXK_ESCAPE: {
				if (lastKeyCode != event.GetKeyCode()) {
					wxMessageDialog dialog(
						this, "Exit application?", "Confirm Action",
						wxOK | wxCANCEL | wxICON_QUESTION);
					if (dialog.ShowModal() == wxID_OK) { Close(); }
				}
				break;
			}
			default:
				if (std::isalpha(std::clamp(event.GetKeyCode(), -1, 255))) {
					comicGallery->HandleInput(
						Navigation::JumpToComic, event.GetKeyCode());
				}
				break;
		}
	}
	lastKeyCode = event.GetKeyCode();
	event.Skip();
}

bool MyFrame::OpenComic() {
	try {
		comicGallery->currentComic().unload();
		comicViewer = new ComicViewer(this, comicGallery->currentComic());
		comicViewer->load();
	} catch (const std::exception& e) {
		comicViewer->Close();
		comicViewer = nullptr;
		wxMessageBox(e.what(), "Error while opening comic");
		return false;
	}
	SetTitle(comicGallery->currentComic().getName());
	sizer->Add(comicViewer, wxSizerFlags(1).Expand());
	sizer->Hide(size_t(0));
	comicViewer->SetFocus();
	comicViewer->SetFocusFromKbd();
	Layout();
	return true;
}

void MyFrame::CloseComic() {
	sizer->Remove(1);
	sizer->Show(size_t(0));
	comicViewer->Close();
	comicViewer = nullptr;
	comicGallery->currentComic().unload();
	Layout();
	SetTitle(DEFAULT_FRAME_TITLE);
}

void MyFrame::handleOptions(Options option) {
	const auto OPEN_LOCAL = 1;
	const auto OPEN_REMOTE = 2;

	if (option == Options::NONE) {
		wxMenu menu;
		menu.Append(
			static_cast<int>(Options::LOAD_LOCAL_COMICS), "Open Local Comics");
		menu.Append(
			static_cast<int>(Options::LOAD_REMOTE_COMICS),
			"Fetch from Suwayomi Server");
		menu.Append(
			static_cast<int>(Options::SET_LOCAL_AS_DEFAULT),
			"Make \"Open Local Comics\" dialog as default action");
		menu.Append(
			static_cast<int>(Options::SET_REMOTE_AS_DEFAULT),
			"Make \"Fetch from Suwayomi Server\" as default option");
		menu.Append(
			static_cast<int>(Options::SET_SERVER_ADDRESS),
			"Set Suwayomi Server address");
		option = static_cast<Options>(GetPopupMenuSelectionFromUser(menu));
	}

	constexpr auto DEFAULT_KEY = "/DefaultAction";
	wxConfigBase* cfg = wxConfigBase::Get();

	std::vector<ComicSource> paths;

	if (option == Options::DEFAULT) {
		auto defaultAction = cfg->ReadLong(DEFAULT_KEY, OPEN_LOCAL);
		if (defaultAction == OPEN_LOCAL) {
			option = Options::LOAD_LOCAL_COMICS;
		}
		if (defaultAction == OPEN_REMOTE) {
			option = Options::LOAD_REMOTE_COMICS;
		}
	}
	if (option == Options::NONE) { return; }
	if (option == Options::LOAD_LOCAL_COMICS) {
		wxFileDialog openFileDialog(
			this, "Open Comic", "", "",
			"Comic Files (*.cbr;*.cbz)|*.cbr;*.cbz|"
			"All files|*.*",
			wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
		if (openFileDialog.ShowModal() == wxID_CANCEL) { return; }
		wxArrayString selectedFiles;
		openFileDialog.GetPaths(selectedFiles);
		for (auto& e : selectedFiles) { paths.emplace_back(e.ToStdString()); }
	} else if (option == Options::LOAD_REMOTE_COMICS) {
		paths = getUnreadChapters();
	} else if (option == Options::SET_LOCAL_AS_DEFAULT) {
		cfg->Write(DEFAULT_KEY, OPEN_LOCAL);
		cfg->Flush();
	} else if (option == Options::SET_REMOTE_AS_DEFAULT) {
		cfg->Write(DEFAULT_KEY, OPEN_REMOTE);
		cfg->Flush();
	} else if (option == Options::SET_SERVER_ADDRESS) {
		getServer(true);
	}
	if (!paths.empty()) {
		comicGallery->loadComics(paths);
		if (comicGallery->length() == 0) {
			wxMessageBox("No Valid Comic file found");
		}
	}
}

void MyFrame::LoadComics(const wxArrayString& args) {
	std::vector<ComicSource> paths;
	for (auto i = 1u; i < args.size(); ++i) {
		paths.emplace_back(args[i].ToStdString());
	}
	sizer = new wxBoxSizer(wxVERTICAL);
	comicGallery = new ComicGallery(this);

	handleOptions(Options::DEFAULT);

	sizer->Add(comicGallery, wxSizerFlags(1).Expand());

	SetSizer(sizer);
	Layout();
}
