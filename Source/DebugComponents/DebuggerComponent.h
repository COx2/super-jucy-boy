#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"
#include "CpuDebugComponent.h"
#include "MemoryMapComponent.h"
#include "PpuDebugComponent.h"

class JucyBoy;

class DebuggerComponent : public juce::Component
{
public:
	DebuggerComponent();
	~DebuggerComponent() = default;

	void SetJucyBoy(JucyBoy &jucy_boy_);

	void UpdateState(bool compute_diff);
	void OnEmulationStarted();
	void OnEmulationPaused();

	void paint(juce::Graphics&) override;
	void resized() override;

private:
	static const size_t cpu_status_width_{ 150 };
	static const size_t memory_map_width_{ 430 };
	static const size_t ppu_tileset_width_{ 128 * 2 };

private:
	juce::Rectangle<int> usage_instructions_area_;
	CpuDebugComponent cpu_debug_component_;
	MemoryMapComponent memory_map_component_;
	PpuDebugComponent ppu_debug_component_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebuggerComponent)
};