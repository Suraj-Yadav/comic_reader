#pragma once

#include <iostream>
#include <string>

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

#define println(...)        \
	{                       \
		print(__VA_ARGS__); \
		cout << std::endl;  \
	}

#define printloc(...) \
	println(__FILE__ ":" + std::to_string(__LINE__), __VA_ARGS__);

#define printFuncCall println(__FUNCTION__, "called")
