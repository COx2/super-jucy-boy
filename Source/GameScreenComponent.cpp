#include "GameScreenComponent.h"
#include <string>

GameScreenComponent::GameScreenComponent() :
	vertices_{ Vertex{ { -1.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
	Vertex{ { 1.0f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
	Vertex{ { 1.0f, -1.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
	Vertex{ { -1.0f, -1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } } },
	elements_{ 0, 1, 2, 2, 3, 0 }
{
	// It's important to set the OpenGL component painting to false, otherwise the OpenGL thread will need to lock Juce's MessageManager, which leads to all sorts of deadlocks
	openGLContext.setComponentPaintingEnabled(false);

	// Continuous repainting leads to greatly decreased performance (although disabling the above might solve this issue too, in case it was caused by locking the MessageManager continuously)
	openGLContext.setContinuousRepainting(false); 

	openGLContext.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
}

GameScreenComponent::~GameScreenComponent()
{
	shutdownOpenGL();
}

void GameScreenComponent::initialise()
{
#pragma region Shaders initialization
	const std::string vertex_shader{
		"#version 400\n"
		"layout(location = 0) in vec2 vertex_position;\n"
		"layout(location = 1) in vec3 vertex_color;\n"
		"layout(location = 2) in vec2 vertex_texcoord;\n"
		"out vec4 color;\n"
		"out vec2 texcoord;\n"
		"void main() {\n"
		"  gl_Position = vec4 (vertex_position, 0.0, 1.0);\n"
		"  color = vec4(vertex_color, 1.0);\n"
		"  texcoord = vertex_texcoord;\n"
		"}\n" };

	const std::string fragment_shader{
		"#version 400\n"
		"in vec4 color;\n"
		"in vec2 texcoord;\n"
		"out vec4 frag_color;\n"
		"uniform sampler2D tex;\n"
		"void main() {\n"
		"  frag_color = mix(texture2D(tex, texcoord), color, 0);\n"
		"}\n" };

	if (!shader_program.addVertexShader(vertex_shader)
		|| !shader_program.addFragmentShader(fragment_shader)
		|| !shader_program.link())
	{
		const auto error = shader_program.getLastError();
		throw std::runtime_error{ std::string{ "Failed to create OpenGL shaders: " } +error.toStdString() };
	}
#pragma endregion

	// Generate Vertex Array Object
	// The binding of the VAO must be done BEFORE binding the GL_ELEMENT_ARRAY_BUFFER
	// Otherwise the GL_ELEMENT_ARRAY_BUFFER won't be tied to the VAO state, and thus not automatically bound with it
	openGLContext.extensions.glGenVertexArrays(1, &vertex_array_object_);
	openGLContext.extensions.glBindVertexArray(vertex_array_object_);

	// Generate Vertex Buffer Object
	openGLContext.extensions.glGenBuffers(1, &vertex_buffer_object_);
	openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object_);
	openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(decltype(vertices_)::value_type), vertices_.data(), GL_STATIC_DRAW);

	// Generate Element Buffer Object
	openGLContext.extensions.glGenBuffers(1, &element_buffer_object_);
	openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object_);
	openGLContext.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_.size() * sizeof(decltype(elements_)::value_type), elements_.data(), GL_STATIC_DRAW);

	// Generate texture
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_2D, texture_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 160, 144, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	openGLContext.extensions.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	openGLContext.extensions.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * 2));
	openGLContext.extensions.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * 5));

	// Attributes are disabled by default, therefore enable them
	openGLContext.extensions.glEnableVertexAttribArray(0);
	openGLContext.extensions.glEnableVertexAttribArray(1);
	openGLContext.extensions.glEnableVertexAttribArray(2);

	openGLContext.extensions.glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GameScreenComponent::shutdown()
{
	openGLContext.extensions.glDeleteBuffers(1, &vertex_buffer_object_);
	openGLContext.extensions.glDeleteBuffers(1, &element_buffer_object_);

	glDeleteTextures(1, &texture_);
}

void GameScreenComponent::OnNewFrame(const PPU::Framebuffer &ppu_framebuffer)
{
	std::unique_lock<std::mutex> lock{ framebuffer_mutex_ };

	for (int i = 0; i < ppu_framebuffer.size(); ++i)
	{
		framebuffer_[3 * i] = PpuColorToIntensity(ppu_framebuffer[i]);
		framebuffer_[3 * i + 1] = PpuColorToIntensity(ppu_framebuffer[i]);
		framebuffer_[3 * i + 2] = PpuColorToIntensity(ppu_framebuffer[i]);
	}

	openGLContext.triggerRepaint();
}

uint8_t GameScreenComponent::PpuColorToIntensity(PPU::Color color)
{
	switch (color)
	{
	case PPU::Color::White:
		return 255;
	case PPU::Color::LightGrey:
		return 192;
	case PPU::Color::DarkGrey:
		return 96;
	case PPU::Color::Black:
		return 0;
	default:
		throw std::invalid_argument("Invalid argument to PpuColorToIntensity: " + std::to_string(static_cast<int>(color)));
	}
}

void GameScreenComponent::render()
{
	const auto desktopScale = openGLContext.getRenderingScale();

	OpenGLHelpers::clear(Colour::greyLevel(0.1f));

	glViewport(0, 0, getWidth(), getHeight());

	shader_program.use();

	// Bind and draw texture
	openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_);
	{std::unique_lock<std::mutex> lock{ framebuffer_mutex_ };
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 160, 144, GL_RGB, GL_UNSIGNED_BYTE, framebuffer_.data()); }

	// Bind VAO
	openGLContext.extensions.glBindVertexArray(vertex_array_object_);

	// Draw triangles
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(elements_.size()), GL_UNSIGNED_INT, 0);

	// Unbind VAO and texture
	openGLContext.extensions.glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GameScreenComponent::paint (Graphics&)
{
	
}

void GameScreenComponent::resized()
{
	
}
