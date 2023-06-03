#include <wx/app.h>
#include <wx/frame.h>
#include <wx/progdlg.h>

#include <algorithm>
#include <queue>
#include <thread>

#include "comic.hpp"
#include "comic_gallery.hpp"
#include "comic_viewer.hpp"

const auto CPU_THREADS = std::max(std::thread::hardware_concurrency(), 1u);

class MyApp : public wxApp {
   public:
	bool OnInit() override;
	int OnExit() override;
};

wxIMPLEMENT_APP(MyApp);

class MyFrame : public wxFrame {
	std::vector<Comic> comics;
	ComicViewer* comicViewer;

   public:
	MyFrame();
	void LoadComic(const std::filesystem::path& comicPath);
};

int MyApp::OnExit() {
	std::filesystem::remove_all(cacheDirectory);
	std::filesystem::create_directories(cacheDirectory);
	return 0;
}

bool MyApp::OnInit() {
	// std::filesystem::remove_all(cacheDirectory);
	::wxInitAllImageHandlers();
	std::vector<std::filesystem::path> args;
	args.reserve(argc - 1);
	for (int i = 1; i < argc; ++i) {
		args.push_back(argv[i].ToStdString());
	}

	auto frame = new MyFrame();
	frame->Show(true);
	frame->LoadComic(args[0]);
	return true;
}

MyFrame::MyFrame() : wxFrame(nullptr, wxID_ANY, ""), comicViewer(nullptr) {}

void MyFrame::LoadComic(const std::filesystem::path& comicPath) {
	auto paths = std::queue<wxString>();
	{
		wxArrayString selectedFiles;
		wxFileDialog openFileDialog(
			this, "Open Comic", "", "", "Comic Files (*.cbr;*.cbz)|*.cbr;*.cbz",
			wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

		if (openFileDialog.ShowModal() == wxID_CANCEL) {
			return;
		}
		openFileDialog.GetPaths(selectedFiles);

		std::sort(selectedFiles.begin(), selectedFiles.end());
		selectedFiles.erase(
			std::unique(selectedFiles.begin(), selectedFiles.end()),
			selectedFiles.end());

		for (auto& e : selectedFiles) {
			paths.push(e);
		}
	}

	wxProgressDialog dialog(
		"Please wait", "Loading Comics", paths.size(), this);

	std::mutex guard;
	auto f = [&](bool isMainThread) {
		for (wxString path; !paths.empty();) {
			{
				std::lock_guard<std::mutex> lock(guard);
				if (paths.empty()) {
					break;
				}
				path = paths.front();
				paths.pop();
			}
			auto c = Comic(path.ToStdString());
			{
				std::lock_guard<std::mutex> lock(guard);
				comics.push_back(c);
				if (isMainThread) {
					dialog.Update(comics.size());
				}
			}
		}
	};
	std::vector<std::thread> threads;
	for (auto i = 2u; i < CPU_THREADS; ++i) {
		threads.emplace_back(f, false);
	}
	f(true);
	for (auto& thread : threads) {
		thread.join();
	}
	dialog.Update(comics.size());

	std::sort(comics.begin(), comics.end());

	std::vector<std::filesystem::path> covers;
	for (auto& comic : comics) {
		covers.push_back(comic.coverPage);
	}

	new ComicGallery(this, covers);

	Layout();
}
