#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"
#include "../../JucyBoy/PPU.h"
#include <optional>

class BackgroundRenderer
{
public:
	BackgroundRenderer();
	~BackgroundRenderer() = default;

	void SetPpu(PPU* ppu) { ppu_ = ppu; }

	void Update();
	void SetViewportArea(const juce::Rectangle<int> &viewport_area);
	std::optional<size_t> GetSelectedTileMap() const { return selected_tile_map_; }
	void SetSelectedTileMap(std::optional<size_t> selected_tile_map);

	// These are not overrides since the class does not inherit from juce::OpenGLAppComponent, but the functions have the same names
	void render();
	void initialise();
	void shutdown();

private:
	struct Vertex
	{
		float position[2];
		float texCoord[3];
	};

	static std::vector<Vertex> InitializeVertices();
	static std::vector<GLuint> InitializeElements();

public:
	static constexpr size_t bg_width_in_tiles_{ 32 };
	static constexpr size_t bg_height_in_tiles_{ 32 };
	static constexpr size_t num_tiles_{ bg_width_in_tiles_ * bg_height_in_tiles_ };
	static constexpr size_t tile_width_{ 8 };
	static constexpr size_t tile_height_{ 8 };

private:
	PPU::Tileset tile_set_{};
	PPU::TileMap tile_map_{};
	size_t active_tile_set_{ 0 };
	std::atomic<bool> update_sync_{ false };

	// OpenGL stuff
	GLuint vertex_array_object_{ 0 };
	GLuint vertex_buffer_object_{ 0 };
	GLuint element_buffer_object_{ 0 };
	GLuint shader_program_{ 0 };
	GLuint texture_{ 0 };

	const std::vector<Vertex> vertices_;
	const std::vector<GLuint> elements_;

	std::array<uint8_t, static_cast<size_t>(PPU::Color::Count)> intensity_palette_;

	std::optional<size_t> selected_tile_map_;

	juce::Rectangle<int> viewport_area_;
	bool opengl_initialization_complete_{ false };

	PPU* ppu_{ nullptr };
	size_t updated_index_{ 0 };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackgroundRenderer)
};
