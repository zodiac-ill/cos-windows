#include "main.hpp"
#include "async.hpp"
#include "input.hpp"

#define DEBUG_STEPS false

#if DEBUG_STEPS
// this one's me being lazy
template <typename T>
inline T* ptr_to_offset(void* base, unsigned int offset) {
	return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + offset);
};

template <typename T>
inline T get_from_offset(void* base, unsigned int offset) {
	return *ptr_to_offset<T>(base, offset);
};
#endif

// get timestamp, but with custom keybinds (or similar mods) compatibility
std::uint64_t getTimestampCompat() {
	auto event = ExtendedCCKeyboardDispatcher::getCurrentEventInfo();
	if (!event) {
		return 0;
	}

	auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
	return extendedInfo->getTimestamp();
}

// i had to rename this function bc queueButton is inlined and Geode wont let you
// try to hook inlined functions anymore
void CustomGJBaseGameLayer::customQueueButton(int btnType, bool push, bool secondPlayer) {
	// this is another workaround for it not being very easy to pass arguments to things
	// oh well, ig

	auto inputTimestamp = static_cast<AsyncUILayer*>(this->m_uiLayer)->getLastTimestamp();
	auto timeRelativeBegin = this->m_fields->m_timeBeginMs;

	if (!inputTimestamp) {
		inputTimestamp = getTimestampCompat();
	}

	auto currentTime = inputTimestamp - timeRelativeBegin;
	if (inputTimestamp < timeRelativeBegin || !inputTimestamp || !timeRelativeBegin) {
		// holding at the start can queue a button before time is initialized
		currentTime = 0;
	}

#if DEBUG_STEPS
	geode::log::debug("queueing input type={} down={} p2={} at time {} (ts {} -> {})", btnType, push, secondPlayer, currentTime, timeRelativeBegin, inputTimestamp);
#endif

	// if you felt like it, you could calculate the step too
	// i personally don't. this maintains compatibility with physics bypass

	this->m_fields->m_timedCommands.push({
		static_cast<PlayerButton>(btnType),
		push,
		secondPlayer,
		// currentTime shouldn't overflow unless you have one frame every month
		static_cast<std::int32_t>(currentTime)
	});
}

void CustomGJBaseGameLayer::resetLevelVariables() {
	GJBaseGameLayer::resetLevelVariables();

	m_fields->m_timeBeginMs = 0;
	m_fields->m_timeOffset = 0.0;
	m_fields->m_timedCommands = {};
}

void CustomGJBaseGameLayer::processTimedInputs() {
	// if you calculated steps upfront, you could also just rewrite processQueuedButtons to stop after it handles the current step
	// not done here as processQueuedButtons is inlined on macos :(

	// calculate the current time offset (in ms) that we stop handling inputs at
	auto timeMs = static_cast<std::uint64_t>(m_fields->m_timeOffset * 1000.0);

	auto& commands = this->m_fields->m_timedCommands;
	if (!commands.empty()) {
		auto nextTime = commands.front().m_step;

#if DEBUG_STEPS
		geode::log::debug("step info: time={}, waiting for {}", timeMs, nextTime);
#endif

		while (!commands.empty() && nextTime <= timeMs) {
			auto btn = commands.front();
			commands.pop();

#if DEBUG_STEPS
			geode::log::debug("queuedInput: btn={} push={} p2={} timeMs={}",
				static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2, btn.m_step
			);
#endif

			this->m_queuedButtons.push_back(btn);

			if (!commands.empty()) {
				nextTime = commands.front().m_step;
			}
		}
	}
}

void CustomGJBaseGameLayer::dumpInputQueue() {
	// failsafe, if an input hasn't been processed then we'll force it to be processed by the next frame
	auto& commands = this->m_fields->m_timedCommands;
	while (!commands.empty()) {
		auto btn = commands.front();
		commands.pop();

#if DEBUG_STEPS
		geode::log::debug("failsafe queuedInput: btn={} push={} p2={} timeMs={}",
			static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2, btn.m_step
		);
#endif
		this->m_queuedButtons.push_back(btn);
	}
}

void CustomGJBaseGameLayer::update(float dt) {
	m_fields->m_timeBeginMs = platform_get_time();
	m_fields->m_timeOffset = 0.0;

	GJBaseGameLayer::update(dt);

	dumpInputQueue();
}

void CustomGJBaseGameLayer::processCommands(float timeStep) {
	auto timeWarp = m_gameState.m_timeWarp;
	m_fields->m_timeOffset += timeStep / timeWarp;

	processTimedInputs();

	GJBaseGameLayer::processCommands(timeStep);
}

void CustomGJBaseGameLayer::fixUntimedInputs() {
	// windows workaround as queueButton and its vector insert is supposedly inlined everywhere!!
	// as long as this is called before the timestamp is cleared, it should work
	for (const auto& btn : this->m_queuedButtons) {
		this->customQueueButton(static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2);
	}

	this->m_queuedButtons.clear();
}

