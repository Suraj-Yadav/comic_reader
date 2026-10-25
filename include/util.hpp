#pragma once

#include <wx/log.h>

#define dbg(X) #X, "=", (X)

namespace hide {
	template <typename S> void write(S& /*unused*/) {}
	inline void appendUtf8(wxString& os, std::string_view sv) {
		wxString s = wxString::FromUTF8(sv.data(), sv.size());
		// Not valid UTF-8 (FromUTF8 gives an empty string): still show
		// something.
		if (s.empty() && !sv.empty()) {
			s = wxString::From8BitData(sv.data(), sv.size());
		}
		os << s;
	}
	template <typename S, typename T> void write(S& os, const T& t) {
		if constexpr (std::is_same_v<T, wxString>) {
			os << t;
		} else if constexpr (
			std::is_convertible_v<const T&, std::string_view>) {
			// std::string, const char*, literals: treated as UTF-8
			appendUtf8(os, t);
		} else if constexpr (requires { os << t; }) {
			// check if "os << t" is defined;
			os << t;
		} else {
			// hope t is a container and print its elements
			bool first = true;
			os << "[ ";
			for (const auto& e : t) {
				if (!first) { os << ", "; }
				os << e;
				first = false;
			}
			os << " ]";
		}
	}
	template <typename S, typename T, typename... Args>
	void write(S& os, const T& t, Args&&... args) {
		write(os, t);
		os << " ";
		write(os, std::forward<Args>(args)...);
	}
	template <typename... Args>
	wxString log(auto file, auto line, Args... args) {
		wxString msg;
		msg << file << ":" << line << ": ";
		write(msg, std::forward<Args>(args)...);
		return msg;
	}
}  // namespace hide

#define LOG_FATAL(...) \
	wxLogFatalError(hide::log(__FILE__, __LINE__, __VA_ARGS__))
#define LOG_ERROR(...) wxLogError(hide::log(__FILE__, __LINE__, __VA_ARGS__))
#define LOG_WARN(...)  wxLogWarning(hide::log(__FILE__, __LINE__, __VA_ARGS__))
#define LOG_INFO(...)  wxLogInfo(hide::log(__FILE__, __LINE__, __VA_ARGS__))
#define LOG_DEBUG(...) wxLogDebug(hide::log(__FILE__, __LINE__, __VA_ARGS__))
