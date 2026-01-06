#ifndef GRAPHICS_UTILS_HPP
#define GRAPHICS_UTILS_HPP

#if !defined(_WIN32)

#include <string>

#include "graphics_base.hpp"

namespace core {

	namespace modules {

		struct Image {
			int width;
			int height;
			virtual ~Image() = default;
			static Image* load_image(const std::string& filename);
		};

		struct Font {
			int size;
			std::string name;
			int orientation;
			int weight;
			bool italic;
			bool underline;
			bool strike;
			virtual ~Font() = default;
			static Font* create_font(int size = 20, const std::string& name = "Arial", int weight = 0,
				bool italic = false, bool underline = false, bool strike = false, int orientation = 0);
		};

		class Window {
		protected:
			long width, height;
			bool quit = false;

		public:
			Window();
			virtual ~Window() = default;

			virtual bool initialize(const std::string& title, int width, int height);
			virtual int get_width();
			virtual int get_height();
			virtual void clear_screen(Color color);
			virtual TextSize get_text_size(const std::string& text, Font* font);
			virtual void draw_text(int x, int y, const std::string& text, Color color, Font* font);
			virtual void draw_image(Image* image, int x, int y);
			virtual void draw_pixel(int x, int y, Color color);
			virtual void draw_line(int x1, int y1, int x2, int y2, Color color);
			virtual void draw_rect(int x, int y, int width, int height, Color color);
			virtual void fill_rect(int x, int y, int width, int height, Color color);
			virtual void draw_circle(int xc, int yc, int radius, Color color);
			virtual void fill_circle(int xc, int yc, int radius, Color color);
			virtual void update();
			virtual bool is_quit();

			bool is_key_down(int vk) const;
			bool is_mouse_down(int button) const;
			int mouse_x() const;
			int mouse_y() const;
			int mouse_wheel() const;

		};

	}

}

#endif // !defined(_WIN32)

#endif // !GRAPHICS_UTILS_HPP
