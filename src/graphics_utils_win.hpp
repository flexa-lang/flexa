#ifndef GRAPHICS_UTILS_WIN_HPP
#define GRAPHICS_UTILS_WIN_HPP

#if defined(_WIN32)

#include <Windows.h>
#include <windowsx.h>
#include <map>
#include <string>

#include "graphics_base.hpp"

namespace core {

	namespace modules {

		struct Image {
			HBITMAP bitmap;
			int width;
			int height;
			~Image();
			static Image* load_image(const std::string& filename);
		};

		struct Font {
			HFONT font;
			int size;
			std::string name;
			int orientation;
			int weight;
			bool italic;
			bool underline;
			bool strike;
			~Font();
			static Font* create_font(int size = 20, const std::string& name = "Arial", int weight = 0,
				bool italic = false, bool underline = false, bool strike = false, int orientation = 0);
		};

		struct KeyboardState {
			bool keys[256]{};
		};

		struct MouseState {
			int x = 0;
			int y = 0;
			bool buttons[3]{};
			int wheel_delta = 0;
		};

		class Window {
		private:
			HWND hwnd;
			HDC hdc;
			HBITMAP hbm_back_buffer;
			HDC hdc_back_buffer;
			long width, height;
			MSG msg = { 0 };
			bool quit = false;

			static std::map<HWND, Window*> hwnd_map;

			KeyboardState keyboard;
			MouseState mouse;

		public:
			Window();
			~Window();

			bool initialize(const std::string& title, int width, int height);
			int get_width();
			int get_height();
			void clear_screen(Color color);
			TextSize get_text_size(const std::string& text, Font* font);
			void draw_text(int x, int y, const std::string& text, Color color, Font* font);
			void draw_image(Image* image, int x, int y);
			void draw_pixel(int x, int y, Color color);
			void draw_line(int x1, int y1, int x2, int y2, Color color);
			void draw_rect(int x, int y, int width, int height, Color color);
			void fill_rect(int x, int y, int width, int height, Color color);
			void draw_circle(int xc, int yc, int radius, Color color);
			void fill_circle(int xc, int yc, int radius, Color color);
			void update();
			bool is_quit();

			bool is_key_down(int vk) const;
			bool is_mouse_down(int button) const;
			int mouse_x() const;
			int mouse_y() const;
			int mouse_wheel() const;

		private:
			void resize_back_buffer();

			static LRESULT CALLBACK window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
			LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam);
		};

	}

}

#endif // defined(_WIN32)

#endif // !GRAPHICS_UTILS_HPP
