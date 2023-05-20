#include <wx/wx.h>

#include <cxxopts.hpp>
#include <memory>

#include "image_viewer.hpp"

class MyApp : public wxApp {
   public:
	bool OnInit() override;
};

wxIMPLEMENT_APP(MyApp);

class MyFrame : public wxFrame {
   public:
	MyFrame(std::string imagePath);
};

bool MyApp::OnInit() {
	::wxInitAllImageHandlers();
	cxxopts::Options options("comic_reader", "A Simple Comic book reader");

	options.add_options()												//
		("image", "Image File to view", cxxopts::value<std::string>())	//
		("h,help", "Print usage");

	options.parse_positional("image");
	options.show_positional_help();

	cxxopts::ParseResult result;
	try {
		result = options.parse(argc, argv);
		if (result.count("help")) {
			wxMessageBox(options.help(), "Options");
			return false;
		}
		if (result["image"].count() == 0) {
			wxMessageBox("Need to pass path to image", "Error");
			return false;
		}
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		return false;
	}

	MyFrame* frame = new MyFrame(result["image"].as<std::string>());
	frame->Show(true);

	return true;
}

MyFrame::MyFrame(std::string imagePath)
	: wxFrame(nullptr, wxID_ANY, "Hello World") {
	new ImageViewer(this, "image.jpg");
}
