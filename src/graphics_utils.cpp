#if !defined(_WIN32)

#include "graphics_utils.hpp"

#include <stdexcept>

using namespace core::modules;

Image* Image::load_image(const std::string&) {
	throw std::runtime_error("Not implemented yet");
}

Font* Font::create_font(int, const std::string&, int, bool, bool, bool, int) {
	throw std::runtime_error("Not implemented yet");

	return new Font{};
}

Window::Window() : width(0), height(0) {}

bool Window::initialize(const std::string&, int, int) {
	throw std::runtime_error("Not implemented yet");

	return true;
}

int Window::get_width() {
	throw std::runtime_error("Not implemented yet");
	return width;
}

int Window::get_height() {
	throw std::runtime_error("Not implemented yet");
	return height;
}

void Window::clear_screen(Color) {
	throw std::runtime_error("Not implemented yet");
}

TextSize Window::get_text_size(const std::string&, Font*) {
	throw std::runtime_error("Not implemented yet");

	return TextSize{};
}

void Window::draw_text(int, int, const std::string&, Color, Font*) {
	throw std::runtime_error("Not implemented yet");
}

void Window::draw_image(Image*, int, int) {
	throw std::runtime_error("Not implemented yet");
}

void Window::draw_pixel(int, int, Color) {
	throw std::runtime_error("Not implemented yet");
}

void Window::draw_line(int, int, int, int, Color) {
	throw std::runtime_error("Not implemented yet");
}

void Window::draw_rect(int, int, int, int, Color) {
	throw std::runtime_error("Not implemented yet");
}

void Window::fill_rect(int, int, int, int, Color) {
	throw std::runtime_error("Not implemented yet");
}

void Window::draw_circle(int, int, int, Color) {
	throw std::runtime_error("Not implemented yet");
}

void Window::fill_circle(int, int, int, Color) {
	throw std::runtime_error("Not implemented yet");
}

void Window::update() {
	throw std::runtime_error("Not implemented yet");
}

bool Window::is_quit() {
	throw std::runtime_error("Not implemented yet");
	return quit;
}

bool Window::is_key_down(int) const {
	throw std::runtime_error("Not implemented yet");
	return false;
}

bool Window::is_mouse_down(int) const {
	throw std::runtime_error("Not implemented yet");
	return false;
}

int Window::mouse_x() const {
	throw std::runtime_error("Not implemented yet");
	return 0;
}

int Window::mouse_y() const {
	throw std::runtime_error("Not implemented yet");
	return 0;
}

int Window::mouse_wheel() const {
	throw std::runtime_error("Not implemented yet");
	return 0;
}

#endif // !defined(_WIN32)
