#include <cmath>

namespace fuzzy {
	const double epsilon = 0.000001;

	double delta(double a, double b) {
		return std::max(std::max(std::abs(a), std::abs(b)), epsilon) * 0.01;
	}

	bool less(double a, double b) { return a < (b - delta(a, b)); }
	bool greater(double a, double b) { return a > (b + delta(a, b)); }
	bool equal(double a, double b) { return std::abs(a - b) < epsilon; }
	bool less_equal(double a, double b) { return !greater(a, b); }
	bool greater_equal(double a, double b) { return !less(a, b); }
};	// namespace fuzzy
