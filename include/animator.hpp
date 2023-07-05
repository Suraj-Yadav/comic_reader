#pragma once

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

	void Start(
		int durationMs, T start, T end, std::function<void(T i)> onStep,
		std::function<void()> onEnd) {
		Start(
			durationMs, start, end, tweeny::easing::defaultEasing(), onStep,
			onEnd);
	}

	template <typename EasingT>
	void Start(
		int durationMs, T start, T end, EasingT easing,
		std::function<void(T i)> onStep, std::function<void()> onEnd) {
		Reset();
		tween = tweeny::from(start).to(end).during(durationMs).via(easing);
		isAnimating = true;
		this->onStep = onStep;
		this->onEnd = onEnd;
		if (onStep) { onStep(start); }
		timer.Start(std::max(durationMs / 100, 1));
	}

	void End() {
		isAnimating = false;
		timer.Stop();
		if (onEnd) { onEnd(); }
	}

   private:
	void Process() {
		using namespace std::chrono;
		auto now = steady_clock::now();
		auto elapsedMs =
			duration_cast<duration<int, std::milli>>(now - startTime).count();
		auto val = tween.seek(elapsedMs, true);
		if (onStep) { onStep(val); }
		if (tween.progress() >= 1.0f) { End(); }
	}

	void OnTimer(wxTimerEvent& event) { Process(); }
};
