#pragma once

#include <cstdint>
#include <array>
#include "../JuceLibraryCode/JuceHeader.h"
#include "JucyBoy/PPU.h"

class GameScreenComponent final : public juce::OpenGLAppComponent, public PPU::Listener
{
public:
	GameScreenComponent();
	~GameScreenComponent();

	void initialise() override final;
	void shutdown() override final;
	void render() override;

	void paint(juce::Graphics&) override {}
	void resized() override {}

	void OnNewFrame(const PPU::Framebuffer &ppu_framebuffer) override;

private:
	struct Vertex
	{
		float position[2];
		float texCoord[2];
	};

	static GLuint CompileShaderProgram();
	std::array<uint8_t, static_cast<size_t>(PPU::Color::Count)> intensity_palette_;

private:
	static const size_t width_{ 160 };
	static const size_t height_{ 144 };
	std::array<uint8_t, width_ * height_> framebuffer_;
	std::mutex framebuffer_mutex_;

	// OpenGL stuff
	GLuint vertex_array_object_{ 0 };
	GLuint vertex_buffer_object_{ 0 };
	GLuint element_buffer_object_{ 0 };
	GLuint shader_program_{ 0 };
	GLuint texture_{ 0 };

	const std::vector<Vertex> vertices_;
	const std::vector<GLuint> elements_;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GameScreenComponent)
};
