#include <string>
// Need to include this before tweeny

#include <tweeny/tweeny.h>
#include <wx/event.h>
#include <wx/timer.h>

#include <chrono>
#include <functional>

#include "util.hpp"

template <typename T> class Animator : public wxEvtHandler {
	bool isAnimating;
	wxTimer timer;
	tweeny::tween<T> tween;
	std::chrono::steady_clock::time_point startTime;

	std::function<void(T i)> onStep;
	std::function<void()> onEnd;

   public:
	Animator() : isAnimating(false) {
		timer.SetOwner(this);
		Bind(wxEVT_TIMER, &Animator::OnTimer, this);
	}

	void Reset() { startTime = std::chrono::steady_clock::now(); }
	bool IsRunning() const { return isAnimating && tween.progress() < 1.0f; }
	void SetOnStep(std::function<void(T i)> onStep) { this->onStep = onStep; }
	void SetOnEnd(std::function<void()> onEnd) { this->onEnd = onEnd; }

	void Start(int durationMs, T start, T end) {
		Start(durationMs, start, end, tweeny::easing::defaultEasing());
	}

	template <typename EasingT>
	void Start(int durationMs, T start, T end, EasingT easing) {
		startTime = std::chrono::steady_clock::now();
		tween = tweeny::from(start).to(end).during(durationMs).via(easing);
		timer.Start(durationMs / 10);
		isAnimating = true;
	}

	void End() {
		isAnimating = false;
		timer.Stop();
		if (onEnd) { onEnd(); }
	}

   private:
	void OnTimer(wxTimerEvent& event) {
		using namespace std::chrono;
		auto now = steady_clock::now();
		auto elapsedMs =
			duration_cast<duration<int, std::milli>>(now - startTime).count();

		auto val = tween.seek(elapsedMs, true);

		if (onStep) { onStep(val); }

		if (tween.progress() >= 1.0f) { End(); }
	}
};
