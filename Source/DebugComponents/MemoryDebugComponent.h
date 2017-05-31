#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../JucyBoy/Memory.h"
#include <vector>

class DebugMMU;

class MemoryMapComponent final : public Component, public ListBoxModel
{
public:
	MemoryMapComponent(DebugMMU &debug_mmu);
	~MemoryMapComponent() = default;

	void UpdateMemoryMap(bool compute_diff);

	// Component overrides
	void paint(Graphics&) override;
	void resized() override;

	// ListBoxModel overrides
	int getNumRows() override;
	void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;

private:
	Memory::Map memory_map_{};

	Label memory_map_list_header_;
	ListBox memory_map_list_box_;

	Memory::Map previous_memory_map_state_{};
	std::vector<Colour> memory_map_colours_{ memory_map_.size(), Colours::black };

	DebugMMU* debug_mmu_{ nullptr };
};

class MemoryWatchpointsComponent final : public Component, public ListBoxModel, public TextEditor::Listener
{
public:
	MemoryWatchpointsComponent(DebugMMU &debug_mmu);
	~MemoryWatchpointsComponent() = default;

	void paint(Graphics&) override;
	void resized() override;

	// ListBoxModel overrides
	int getNumRows() override;
	void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;
	void deleteKeyPressed(int lastRowSelected) override;

	// TextEditor::Listener overrides
	void textEditorReturnKeyPressed(TextEditor &) override;

private:
	std::vector<Memory::Watchpoint> watchpoints_;
	Label watchpoint_list_header_;
	ListBox watchpoint_list_box_;

	TextEditor watchpoint_add_editor_;
	ToggleButton watchpoint_type_read_{ "Read" };
	ToggleButton watchpoint_type_write_{ "Write" };
	Rectangle<int> watchpoint_add_area_;

	DebugMMU* debug_mmu_{ nullptr };
};

class MemoryDebugComponent final : public Component
{
public:
	MemoryDebugComponent(DebugMMU &debug_mmu);
	~MemoryDebugComponent() = default;

	// JucyBoy Listener functions
	void OnStatusUpdateRequested(bool compute_diff);

	void paint (Graphics&) override;
	void resized() override;

private:
	MemoryMapComponent memory_map_component_;
	MemoryWatchpointsComponent memory_watchpoints_component_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MemoryDebugComponent)
};