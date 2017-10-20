#include "../../JuceLibraryCode/JuceHeader.h"
#include <sstream>
#include "DebuggerComponent.h"
#include "../JucyBoy/JucyBoy.h"

DebuggerComponent::DebuggerComponent()
{
	// Debug components
	cpu_debug_component_.addMouseListener(this, true);
	addAndMakeVisible(cpu_debug_component_);

	memory_map_component_.addMouseListener(this, true);
	addAndMakeVisible(memory_map_component_);

	ppu_debug_component_.addMouseListener(this, true);
	addAndMakeVisible(ppu_debug_component_);

	setSize(cpu_status_width_ + memory_map_width_ + ppu_status_width_, 144 * 4);
}

void DebuggerComponent::SetJucyBoy(JucyBoy* jucy_boy)
{
	if (jucy_boy != nullptr)
	{
		cpu_debug_component_.SetCpu(&jucy_boy->GetCpu());
		memory_map_component_.SetMmu(&jucy_boy->GetMmu());
		ppu_debug_component_.SetPpu(&jucy_boy->GetPpu());

		// Set listener interfaces
		listener_deregister_functions_.emplace_back(jucy_boy->GetPpu().AddNewFrameListener([this]() { ppu_debug_component_.Update(); }));
	}
	else
	{
		cpu_debug_component_.SetCpu(nullptr);
		memory_map_component_.SetMmu(nullptr);
		ppu_debug_component_.SetPpu(nullptr);

		// Remove previous listener interfaces
		for (auto &deregister : listener_deregister_functions_) { deregister(); }
		listener_deregister_functions_.clear();
	}

	jucy_boy_ = jucy_boy;
}

void DebuggerComponent::UpdateState(bool compute_diff)
{
	cpu_debug_component_.UpdateState(compute_diff);
	memory_map_component_.UpdateMemoryMap(compute_diff);
	ppu_debug_component_.Update();
}

void DebuggerComponent::OnEmulationStarted()
{
	cpu_debug_component_.OnEmulationStarted();
	memory_map_component_.OnEmulationStarted();
}

void DebuggerComponent::OnEmulationPaused()
{
	cpu_debug_component_.OnEmulationPaused();
	memory_map_component_.OnEmulationPaused();
}

void DebuggerComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::white);

	g.setColour(juce::Colours::orange);
	g.setFont(14.0f);

	std::stringstream usage_instructions;
	usage_instructions << "Space: run / stop" << std::endl;
	usage_instructions << "Right: step over" << std::endl;
	g.drawFittedText(usage_instructions.str(), usage_instructions_area_, juce::Justification::centred, 2);

	g.drawRect(usage_instructions_area_, 1);
}

void DebuggerComponent::resized()
{
	auto working_area = getLocalBounds();

	if (cpu_debug_component_.isVisible())
	{
		auto cpu_debug_area = working_area.removeFromLeft(cpu_status_width_);
		usage_instructions_area_ = cpu_debug_area.removeFromTop(40);
		cpu_debug_component_.setBounds(cpu_debug_area);
	}

	if (memory_map_component_.isVisible())
	{
		auto memory_debug_area = working_area.removeFromLeft(memory_map_width_);
		memory_map_component_.setBounds(memory_debug_area);
	}

	if (ppu_debug_component_.isVisible())
	{
		auto ppu_tileset_area = working_area.removeFromLeft(ppu_status_width_);
		ppu_debug_component_.setBounds(ppu_tileset_area);
	}
}

void DebuggerComponent::visibilityChanged()
{
	// When closing the debugger component, remove all interfaces with JucyBoy
	if (!isVisible() && jucy_boy_)
	{
		SetJucyBoy(nullptr);
	}
}
