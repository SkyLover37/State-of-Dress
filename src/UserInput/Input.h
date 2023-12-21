#pragma once

#include <unordered_set>

class Input : RE::BSTEventSink<RE::InputEvent>
{
public:
	static Input* GetSingleton()
	{
		static Input listener;
		return std::addressof(listener);
	}

	/// <summary>
	/// Process the input event, and filter out unwanted input events through pointer adjustments.
	/// </summary>
	/// <param name="a_event">ptr to a list of input events</param>
	void ProcessAndFilter(const RE::InputEvent* a_event);
    RE::BSEventNotifyControl ProcessEvent(const RE::InputEvent* a_event,
                                          RE::BSTEventSource<RE::InputEvent>* a_eventSource); 
    
	static inline const std::unordered_set<std::string> EventsToFilterWhenWheelerActive = {
		"Favorites",
		"Inventory",
		"Stats",
		"Map",
		"Tween Menu",
		"Quick Inventory",
		"Quick Magic",
		"Quick Stats",
		"Quick Map",
		"Wait",
		"Journal"
	};

};
