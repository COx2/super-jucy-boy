#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <functional>
#include <vector>
#include <memory>
#include "JucyBoy/Debug/DebugCPU.h"
#include "JucyBoy/MMU.h"
#include "JucyBoy/Debug/DebugPPU.h"
#include "JucyBoy/APU.h"
#include "JucyBoy/Timer.h"
#include "JucyBoy/Joypad.h"
#include "JucyBoy/Cartridge.h"
#include "GameScreenComponent.h"
#include "AudioPlayerComponent.h"
#include "DebugComponents/CpuDebugComponent.h"
#include "DebugComponents/MemoryMapComponent.h"
#include "DebugComponents/PpuDebugComponent.h"
#include "OptionsComponents/OptionsWindow.h"

class JucyBoy final : public juce::Component, public CPU::Listener, public DebugCPU::Listener
{
public:
	JucyBoy();
	~JucyBoy();

	void paint (juce::Graphics&) override;
	void resized() override;

	void mouseDown(const juce::MouseEvent &event) override;
	bool keyPressed(const juce::KeyPress &key) override;
	bool keyStateChanged(bool isKeyDown) override;

	// CPU::Listener overrides
	void OnRunningLoopInterrupted() override;

private:
	void LoadRom(std::string file_path);
	void StartEmulation();
	void PauseEmulation();
	void UpdateDebugComponents(bool compute_diff);

	// Save/load state
	void SaveState() const;
	void LoadState();

	// Toggle GUI features on/off
	void EnableDebugging(Component &component, bool enable);
	int ComputeWindowWidth() const;

private:
	static const size_t cpu_status_width_{ 150 };
	static const size_t memory_map_width_{ 430 };
	static const size_t ppu_tileset_width_{ 128 * 2 };

private:
	std::unique_ptr<DebugCPU> cpu_;
	std::unique_ptr<MMU> mmu_;
	std::unique_ptr<DebugPPU> ppu_;
	std::unique_ptr<APU> apu_;
	std::unique_ptr<Timer> timer_;
	std::unique_ptr<Joypad> joypad_;
	std::unique_ptr<Cartridge> cartridge_;

	std::vector<std::function<void()>> listener_deregister_functions_;

	std::string loaded_rom_file_path_;

	juce::LookAndFeel_V4 look_and_feel_{ juce::LookAndFeel_V4::getLightColourScheme() };

	GameScreenComponent game_screen_component_;
	AudioPlayerComponent audio_player_component_;

	juce::Rectangle<int> usage_instructions_area_;
	CpuDebugComponent cpu_debug_component_;
	MemoryMapComponent memory_map_component_;
	PpuDebugComponent ppu_debug_component_;

	OptionsWindow options_window_{ game_screen_component_, audio_player_component_, look_and_feel_ };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JucyBoy)
};
