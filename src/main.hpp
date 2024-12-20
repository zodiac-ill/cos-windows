#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <queue>

struct CustomGJBaseGameLayer : geode::Modify<CustomGJBaseGameLayer, GJBaseGameLayer> {
	struct Fields {
		// stores the time of the last frame
		// all input time calculations are based off this interval
		std::uint64_t m_timeBeginMs{0ull};

		double m_timeOffset{0.0};

		// store timed commands separately from the typical input queue
		// they will instead be added at the correct timestamp
		std::queue<PlayerButtonCommand> m_timedCommands{};
	};

	void update(float dt);
	void customQueueButton(int btnType, bool push, bool secondPlayer);
	void resetLevelVariables();
	void processCommands(float timeStep);

	void processTimedInputs();
	void dumpInputQueue();

	// windows workaround
	void fixUntimedInputs();
};


