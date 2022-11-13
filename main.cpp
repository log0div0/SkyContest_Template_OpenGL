
#ifdef _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <glad/glad.h>
#include <mogl/mogl.hpp>
#include <glfwpp/glfwpp.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <filesystem>
#include <fstream>
#include <efsw/FileSystem.hpp>
#include <efsw/System.hpp>
#include <efsw/efsw.hpp>

namespace fs = std::filesystem;

using Vertex = std::pair<float, float>;

std::vector<Vertex> vertices {
	{-1.f, -1.f},
	{-1.f,  1.f},
	{ 1.f,  1.f},
	{ 1.f, -1.f},
};

std::vector<uint16_t> indices {
	0, 1, 2,
	0, 3, 2
};

fs::path GetExecPath()
{
#ifdef _WIN32
	wchar_t path[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, path, MAX_PATH);
	return path;
#else
	return std::filesystem::canonical("/proc/self/exe");
#endif
}

fs::path GetExecDir()
{
	return GetExecPath().parent_path();
}

float GetTime() {
	static auto start = std::chrono::system_clock::now();
	auto now = std::chrono::system_clock::now();
	std::chrono::duration<float> time = now - start;
	return time.count();
}

std::string LoadTextFile(const fs::path& path) {
	if (!fs::exists(path)) {
		throw std::runtime_error("file " + path.string() + " does not exist");
	}

	auto file_size = fs::file_size(path);
	std::ifstream f(path, std::ios::binary);
	if (!f) {
		throw std::runtime_error("failed to open file " + path.string());
	}
	std::string str;
	str.resize(file_size);
	f.read(&str[0], file_size);
	return str;
}

fs::path GetShaderPath(GLenum type)
{
	switch (type) {
	case GL_VERTEX_SHADER:
		return GetExecDir() / "assets" / "vertex.glsl";
	case GL_FRAGMENT_SHADER:
		return GetExecDir() / "assets" / "fragment.glsl";
	default:
		throw std::runtime_error("unsupported shader type");
	}
}

mogl::ShaderProgram LoadShaders(const fs::path& vertex, const fs::path& fragment)
{
	mogl::ShaderProgram shader_program;
	mogl::Shader vertex_shader(GL_VERTEX_SHADER);
	mogl::Shader fragment_shader(GL_FRAGMENT_SHADER);
	for (auto shader: {&vertex_shader, &fragment_shader}) {
		shader->compile(LoadTextFile(GetShaderPath(shader->getType())));
		if (!shader->isCompiled())
		{
			throw std::runtime_error(shader->getLog());
		}
		shader_program.attach(*shader);
	}
	if (!shader_program.link()) {
		throw std::runtime_error(shader_program.getLog());
	}
	return shader_program;
}

std::atomic_flag shader_program_is_initialized;

class UpdateListener : public efsw::FileWatchListener
{
public:
	void handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename ) override
	{
		shader_program_is_initialized.clear();
	}
};

void RenderFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	glClear(GL_COLOR_BUFFER_BIT);

	static mogl::ShaderProgram shader_program;
	static std::string last_error_message;

	if (!shader_program_is_initialized.test_and_set()) {
		try {
			shader_program = LoadShaders(
				GetExecDir() / "assets" / "vertex.glsl",
				GetExecDir() / "assets" / "fragment.glsl"
			);
			last_error_message = {};
		} catch (const std::exception& error) {
			last_error_message = error.what();
		}
	}

	shader_program.setUniform("Time", GetTime());
	shader_program.use();

	glDisable(GL_DEPTH_TEST);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);

	ImGui::Begin("SkyContest");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), last_error_message.c_str());
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main(int argc, char** argv) {
	try {
		auto GLFW = glfw::init();

		glfw::WindowHints window_hints;
		window_hints.contextVersionMajor = 4;
		window_hints.contextVersionMinor = 6;
		window_hints.openglProfile = glfw::OpenGlProfile::Core;
		window_hints.apply();

		glfw::Window window {1200, 800, "SkyContest"};
		glfw::makeContextCurrent(window);

		glfw::swapInterval(0);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			throw std::runtime_error("Failed to initialize GLAD");
		}

		efsw::FileWatcher file_watcher;
		UpdateListener listener;
		file_watcher.addWatch( (GetExecDir() / "assets").string(), &listener, true );
		file_watcher.watch();

		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 460 core");

		mogl::ArrayBuffer vertex_buffer;
		mogl::ElementArrayBuffer index_buffer;
		mogl::VertexArray vertex_array;

		const GLuint binding_index = 0;

		vertex_buffer.setData(vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
		index_buffer.setData(indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

		vertex_array.setVertexBuffer(binding_index, vertex_buffer.getHandle(), 0, sizeof(vertices[0]));
		vertex_array.setElementBuffer(index_buffer.getHandle());

		const GLuint location_index = 0;

		vertex_array.setAttribBinding(location_index, binding_index);
		vertex_array.setAttribFormat(location_index, 2, GL_FLOAT, GL_FALSE, 0);
		vertex_array.enableAttrib(location_index);

		vertex_array.bind();

		while (!window.shouldClose())
		{
			if (window.getKey(glfw::KeyCode::Escape)) {
				window.setShouldClose(true);
			}

			RenderFrame();

			window.swapBuffers();
			glfw::pollEvents();
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	catch (const std::exception& error) {
		std::cerr << "UNHANDLED EXCEPTION!" << std::endl;
		std::cerr << error.what() << std::endl;
		return 1;
	}
}
