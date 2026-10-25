#include "network.hpp"

#include <cpr/cpr.h>
#include <wx/config.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/string.h>
#include <wx/textdlg.h>

#include <filesystem>
#include <format>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "comic.hpp"
#include "util.hpp"

cpr::ConnectionPool pool;

auto handlerError(cpr::Response&& resp) {
	if (resp.error) { return std::make_pair(std::move(resp), resp.error); }
	if ((resp.status_code / 100) != 2) {
		auto msg =
			std::format("Code: {}, Body: {}", resp.status_code, resp.text);
		return std::make_pair(
			std::move(resp), cpr::Error(resp.status_code, std::move(msg)));
	}
	return std::make_pair(std::move(resp), cpr::Error{});
}

auto graphQuery(const nlohmann::json& json, std::string server = getServer()) {
	return handlerError(
		cpr::Post(
			cpr::Url{std::format("http://{}/api/graphql", server)}, pool,
			cpr::BodyView{json.dump()},
			cpr::Header{{"Content-Type", "application/json"}},
			cpr::Timeout{1000 * 60}));
}

constexpr std::string_view SERVER_INFO = R"({
  aboutServer {
    name
    version
    buildType
  }
})";

std::optional<std::string> isValidServer(const std::string& address) {
	if (address.empty()) { return "Server input is empty"; }
	auto [resp, err] = graphQuery(
		nlohmann::json{
			{"query", SERVER_INFO},
		},
		address);
	if (err) { return err.message; }
	if (resp.text.contains("Suwayomi")) { return {}; }
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
		if (auto err = isValidServer(server.ToStdString())) {
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

constexpr std::string_view GET_UNREAD_CHAPTERS = R"({
  chapters(filter: {isRead: {equalTo: false}, inLibrary: {equalTo: true}}) {
    totalCount
    nodes {
      id
      name
      chapterNumber
      isRead
      isDownloaded
      isBookmarked
      pageCount
      uploadDate
      mangaId
      manga {
        title
      }
    }
  }
})";

std::vector<ComicSource> getUnreadChapters() {
	auto [resp, err] = graphQuery(
		nlohmann::json{
			{"query", GET_UNREAD_CHAPTERS},
		});
	if (err) {
		wxMessageBox(err.message, "Unable to fetch comics");
		exit(0);
	}
	auto j = nlohmann::json::parse(resp.text);
	std::vector<ComicSource> out;
	for (auto& e : j["data"]["chapters"]["nodes"]) {
		auto title = e["manga"]["title"].get<std::string>();
		auto pageCount = e["pageCount"].get<int>();
		title += " ";
		title += e["name"].get<std::string>();
		out.emplace_back(e.value("id", 0), pageCount, title);
	}
	return out;
}

constexpr std::string_view FETCH_CHAPTER_PAGES =
	R"(mutation FetchChapterPages($id: Int!) { 
	fetchChapterPages(input: { chapterId: $id }) { 
		pages 
	} 
})";

void downloadComic(
	int id, const std::function<bool(const std::string& data)>& func) {
	auto [resp, err] = graphQuery(
		nlohmann::json{
			{"query", FETCH_CHAPTER_PAGES},
			{"variables", {{"id", id}}},
		});
	if (err) {
		LOG_WARN("Unable to get comic pages", err.message);
		return;
	}
	auto j = nlohmann::json::parse(resp.text);
	if (j.contains("errors")) {
		LOG_WARN("Unable to get comic pages", nlohmann::to_string(j["errors"]));
		return;
	}
	auto server = getServer();
	for (auto& page : j["data"]["fetchChapterPages"]["pages"]) {
		auto [dResp, dErr] = handlerError(
			cpr::Get(
				cpr::Url{std::format(
					"http://{}{}", server, page.get<std::string>())}));
		if (dErr) {
			LOG_WARN("Unable to download page", dErr.message);
			return;
		}
		if (!func(dResp.text)) { return; }
	}
}

constexpr std::string_view MARK_CHAPTER_AS_READ =
	R"(mutation MarkChapterRead($id: Int!) { 
	updateChapter(input: { id: $id, patch: { isRead: true, lastPageRead: 0 } }) { 
		chapter { 
			id 
			isRead 
			lastPageRead 
		} 
	} 
})";
void markAsRead(int id) {
	graphQuery(
		nlohmann::json{
			{"query", MARK_CHAPTER_AS_READ},
			{"variables", {{"id", id}}},
		});
}

bool saveStream(const std::filesystem::path& path, const std::string& data) {
	std::filesystem::create_directories(path.parent_path());
	std::ofstream of(path, std::ios::binary);
	return bool(of.write(data.data(), data.size()));
}
