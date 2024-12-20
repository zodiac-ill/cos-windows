#include "async.hpp"
#include "main.hpp"

#define DEBUG_PROCESSING false

void AsyncUILayer::handleKeypress(cocos2d::enumKeyCodes key, bool down) {
	auto event = ExtendedCCKeyboardDispatcher::getCurrentEventInfo();

	if (!event) {
		return UILayer::handleKeypress(key, down);
	}

	auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
	m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

	UILayer::handleKeypress(key, down);
	static_cast<CustomGJBaseGameLayer*>(this->m_gameLayer)->fixUntimedInputs();

	m_fields->m_lastTimestamp = 0ull;
}

bool AsyncUILayer::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
	if (!event) {
		return UILayer::ccTouchBegan(touch, event);
	}

	auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
	m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

	auto r = UILayer::ccTouchBegan(touch, event);
	static_cast<CustomGJBaseGameLayer*>(this->m_gameLayer)->fixUntimedInputs();

	m_fields->m_lastTimestamp = 0ull;

	return r;
}

void AsyncUILayer::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
	if (!event) {
		return UILayer::ccTouchMoved(touch, event);
	}

	auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
	m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

	UILayer::ccTouchMoved(touch, event);
	static_cast<CustomGJBaseGameLayer*>(this->m_gameLayer)->fixUntimedInputs();


	m_fields->m_lastTimestamp = 0ull;
}

void AsyncUILayer::ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
	if (!event) {
		return UILayer::ccTouchEnded(touch, event);
	}

	auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
	m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

	UILayer::ccTouchEnded(touch, event);
	static_cast<CustomGJBaseGameLayer*>(this->m_gameLayer)->fixUntimedInputs();

	m_fields->m_lastTimestamp = 0ull;
}

std::uint64_t AsyncUILayer::getLastTimestamp() {
	return m_fields->m_lastTimestamp;
}
