#include "network.hpp"

#include <wx/config.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/string.h>
#include <wx/textdlg.h>
#include <wx/webrequest.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "comic.hpp"
#include "util.hpp"

constexpr auto ApplicationJson = "application/json";

std::optional<wxString> isValidServer(const wxString& address) {
	if (address.IsEmpty()) { return "Server input is empty"; }
	auto request = wxWebSessionSync::GetDefault().CreateRequest(
		wxString::Format("http://%s/api/graphql", address));
	request.SetData(
		R"({"query": "{ aboutServer { name version revision buildType } }"})",
		ApplicationJson);
	auto result = request.Execute();
	if (!result) { return result.error; }
	if (request.GetResponse().AsString().Contains("Suwayomi")) { return {}; }
	return "Server doesn't seem to be a Suwayomi server";
}

std::string getServer(bool changeServer) {
	constexpr auto KEY = "/Server/Address";
	wxConfigBase* cfg = wxConfigBase::Get();
	wxString last = cfg->Read(KEY, ""), error = "", server;
	if (last.size() > 0 && !changeServer) { return last.ToStdString(); }
	while (true) {
		server = wxGetTextFromUser(
			"Enter server address and port: " + error, "Connect to server",
			last);
		if (auto err = isValidServer(server)) {
			error = err.value();
			last = server;
			continue;
		}
		cfg->Write(KEY, server);
		cfg->Flush();
		break;
	}
	return server.ToStdString();
}

std::vector<ComicSource> getUnreadChapters() {
	auto server = getServer();
	auto request = wxWebSessionSync::GetDefault().CreateRequest(
		wxString::Format("http://%s/api/graphql", server));
	request.SetData(
		R"({
    "query": "query GetUnreadChapters { chapters(filter: { isRead: { equalTo: false }, inLibrary: { equalTo: true } }) { totalCount nodes { id name chapterNumber isRead isDownloaded isBookmarked pageCount uploadDate mangaId manga { title } } } }"
})",
		ApplicationJson);
	auto result = request.Execute();
	if (!result) {
		wxMessageBox(result.error, "Unable to fetch comics");
		exit(0);
	}
	auto j = nlohmann::json::parse(request.GetResponse().AsString());
	std::vector<ComicSource> out;
	for (auto& e : j["data"]["chapters"]["nodes"]) {
		out.emplace_back(ComicSource{{}, e.value("id", 0)});
	}
	return out;
}

std::pair<std::string, int> getChapterInfo(int id) {
	auto server = getServer();
	auto request = wxWebSessionSync::GetDefault().CreateRequest(
		wxString::Format("http://%s/api/graphql", server));
	request.SetData(
		wxString::Format(
			R"({
	"query": "query GetChapterInfo($id: Int!) { chapter(id: $id) { chapterNumber pageCount manga { title } } }",
    "variables": { "id": %d }
})",
			id),
		ApplicationJson);
	auto result = request.Execute();
	if (!result) {
		wxMessageBox(result.error, "Unable to get chapter name");
		exit(0);
	}
	auto j = nlohmann::json::parse(request.GetResponse().AsString());
	auto title = j["data"]["chapter"]["manga"]["title"].get<std::string>();
	auto chapterNumber = j["data"]["chapter"]["chapterNumber"].get<int>();
	auto pageCount = j["data"]["chapter"]["pageCount"].get<int>();
	return std::make_pair(
		title + " " + std::to_string(chapterNumber), pageCount);
}

void downloadComic(
	int id, const std::function<bool(wxInputStream& stream)>& func) {
	auto server = getServer();

	auto request = wxWebSessionSync::GetDefault().CreateRequest(
		wxString::Format("http://%s/api/graphql", server));
	request.SetData(
		wxString::Format(
			R"({
    "query": "mutation FetchChapterPages($chapterId: Int!) { fetchChapterPages(input: { chapterId: $chapterId }) { pages } }",
    "variables": { "chapterId": %d }
})",
			id),
		ApplicationJson);
	auto result = request.Execute();
	if (!result) { wxMessageBox(result.error, "Unable to get comic pages"); }
	auto j = nlohmann::json::parse(request.GetResponse().AsString());

	for (auto& page : j["data"]["fetchChapterPages"]["pages"]) {
		auto download = wxWebSessionSync::GetDefault().CreateRequest(
			wxString::Format("http://%s%s", server, page.get<std::string>()));
		auto res = download.Execute();
		if (!res) {
			wxMessageBox(result.error, "Unable to download page");
			return;
		}
		if (!func(*download.GetResponse().GetStream())) { return; }
	}
}

void markAsRead(int id) {
	auto server = getServer();
	auto request = wxWebSessionSync::GetDefault().CreateRequest(
		wxString::Format("http://%s/api/graphql", server));
	request.SetData(
		wxString::Format(
			R"({
    "query": "mutation MarkChapterRead($id: Int!) { updateChapter(input: { id: $id, patch: { isRead: true, lastPageRead: 0 } }) { chapter { id isRead lastPageRead } } }",
    "variables": { "id": %d }
})",
			id),
		ApplicationJson);
	request.Execute();
}

bool saveStream(const std::filesystem::path& path, wxInputStream& stream) {
	std::filesystem::create_directories(path.parent_path());
	wxFileOutputStream outStream(path.string());
	if (!outStream.IsOk()) { return false; }
	outStream.Write(stream);
	return outStream.IsOk() && stream.Eof();
}
