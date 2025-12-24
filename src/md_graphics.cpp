#include "md_graphics.hpp"

#include <memory>

#include "vm.hpp"
#include "semantic_analysis.hpp"
#include "constants.hpp"

#if defined(_WIN32)
#include "graphics_utils_win.hpp"
#else
#include "graphics_utils.hpp"
#endif // defined(_WIN32)

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleGraphics::ModuleGraphics() {}

ModuleGraphics::~ModuleGraphics() = default;

void ModuleGraphics::register_functions(SemanticAnalyser* visitor) {
	visitor->builtin_functions["create_window"] = nullptr;
	visitor->builtin_functions["get_current_width"] = nullptr;
	visitor->builtin_functions["get_current_height"] = nullptr;
	visitor->builtin_functions["clear_screen"] = nullptr;
	visitor->builtin_functions["get_text_size"] = nullptr;
	visitor->builtin_functions["draw_text"] = nullptr;
	visitor->builtin_functions["draw_pixel"] = nullptr;
	visitor->builtin_functions["draw_line"] = nullptr;
	visitor->builtin_functions["draw_rect"] = nullptr;
	visitor->builtin_functions["fill_rect"] = nullptr;
	visitor->builtin_functions["draw_circle"] = nullptr;
	visitor->builtin_functions["fill_circle"] = nullptr;
	visitor->builtin_functions["create_font"] = nullptr;
	visitor->builtin_functions["load_image"] = nullptr;
	visitor->builtin_functions["draw_image"] = nullptr;
	visitor->builtin_functions["update"] = nullptr;
	visitor->builtin_functions["destroy_window"] = nullptr;
	visitor->builtin_functions["destroy_font"] = nullptr;
	visitor->builtin_functions["destroy_image"] = nullptr;
	visitor->builtin_functions["is_quit"] = nullptr;
}

void ModuleGraphics::register_functions(VirtualMachine* vm) {

	vm->builtin_functions["create_window"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("title"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("width"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("height"))->get_value()
		};

		auto title_var = std::make_shared<RuntimeVariable>("title", Type::T_STRING);
		title_var->set_value(new RuntimeValue(vals[0]));
		vm->gc.add_var_root(title_var);

		auto width_var = std::make_shared<RuntimeVariable>("width", Type::T_INT);
		width_var->set_value(new RuntimeValue(vals[1]));
		vm->gc.add_var_root(width_var);

		auto height_var = std::make_shared<RuntimeVariable>("height", Type::T_INT);
		height_var->set_value(new RuntimeValue(vals[2]));
		vm->gc.add_var_root(height_var);

		flx_struct str = flx_struct();
		str["title"] = title_var;
		str["width"] = width_var;
		str["height"] = height_var;

		// create a new window graphic engine
		auto instance_var = std::make_shared<RuntimeVariable>(INSTANCE_ID_NAME, Type::T_INT);
		instance_var->set_value(new RuntimeValue(flx_int(new Window())));
		vm->gc.add_var_root(instance_var);
		str[INSTANCE_ID_NAME] = instance_var;

		// initialize window graphic engine and return value
		auto res = ((Window*)str[INSTANCE_ID_NAME]->get_value()->get_i())->initialize(
			str["title"]->get_value()->get_s(),
			(int)str["width"]->get_value()->get_i(),
			(int)str["height"]->get_value()->get_i()
		);

		// initialize window struct values
		RuntimeValue* win = vm->allocate_value(new RuntimeValue(str, Constants::STD_NAMESPACE, "Window"));

		if (!res) {
			win->set_null();
		}

		vm->push_constant(win);

		};

	vm->builtin_functions["clear_screen"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		int r, g, b;
		r = (int)vals[1]->get_str()["r"]->get_value()->get_i();
		g = (int)vals[1]->get_str()["g"]->get_value()->get_i();
		b = (int)vals[1]->get_str()["b"]->get_value()->get_i();
		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->clear_screen(Color(r, g, b));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["get_current_width"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value();

		RuntimeValue* win = val;
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		vm->push_new_constant(new RuntimeValue(flx_int(((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->get_width())));

		};

	vm->builtin_functions["get_current_height"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value();

		RuntimeValue* win = val;
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		vm->push_new_constant(new RuntimeValue(flx_int(((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->get_height())));

		};

	vm->builtin_functions["draw_pixel"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		int x, y, r, g, b;
		x = (int)vals[1]->get_i();
		y = (int)vals[2]->get_i();
		r = (int)vals[3]->get_str()["r"]->get_value()->get_i();
		g = (int)vals[3]->get_str()["g"]->get_value()->get_i();
		b = (int)vals[3]->get_str()["b"]->get_value()->get_i();
		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->draw_pixel(x, y, Color(r, g, b));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["draw_line"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x1"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y1"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x2"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y2"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		int x1, y1, x2, y2, r, g, b;
		x1 = (int)vals[1]->get_i();
		y1 = (int)vals[2]->get_i();
		x2 = (int)vals[3]->get_i();
		y2 = (int)vals[4]->get_i();
		r = (int)vals[5]->get_str()["r"]->get_value()->get_i();
		g = (int)vals[5]->get_str()["g"]->get_value()->get_i();
		b = (int)vals[5]->get_str()["b"]->get_value()->get_i();
		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->draw_line(x1, y1, x2, y2, Color(r, g, b));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["draw_rect"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("width"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("height"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int x, y, width, height, r, g, b;
		x = (int)vals[1]->get_i();
		y = (int)vals[2]->get_i();
		width = (int)vals[3]->get_i();
		height = (int)vals[4]->get_i();
		r = (int)vals[5]->get_str()["r"]->get_value()->get_i();
		g = (int)vals[5]->get_str()["g"]->get_value()->get_i();
		b = (int)vals[5]->get_str()["b"]->get_value()->get_i();
		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->draw_rect(x, y, width, height, Color(r, g, b));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["fill_rect"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("width"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("height"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int x, y, width, height, r, g, b;
		x = (int)vals[1]->get_i();
		y = (int)vals[2]->get_i();
		width = (int)vals[3]->get_i();
		height = (int)vals[4]->get_i();
		r = (int)vals[5]->get_str()["r"]->get_value()->get_i();
		g = (int)vals[5]->get_str()["g"]->get_value()->get_i();
		b = (int)vals[5]->get_str()["b"]->get_value()->get_i();
		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->fill_rect(x, y, width, height, Color(r, g, b));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["draw_circle"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("xc"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("yc"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("radius"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int xc, yc, radius, r, g, b;
		xc = (int)vals[1]->get_i();
		yc = (int)vals[2]->get_i();
		radius = (int)vals[3]->get_i();
		r = (int)vals[4]->get_str()["r"]->get_value()->get_i();
		g = (int)vals[4]->get_str()["g"]->get_value()->get_i();
		b = (int)vals[4]->get_str()["b"]->get_value()->get_i();
		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->draw_circle(xc, yc, radius, Color(r, g, b));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["fill_circle"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("xc"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("yc"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("radius"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int xc, yc, radius, r, g, b;
		xc = (int)vals[1]->get_i();
		yc = (int)vals[2]->get_i();
		radius = (int)vals[3]->get_i();
		r = (int)vals[4]->get_str()["r"]->get_value()->get_i();
		g = (int)vals[4]->get_str()["g"]->get_value()->get_i();
		b = (int)vals[4]->get_str()["b"]->get_value()->get_i();
		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->fill_circle(xc, yc, radius, Color(r, g, b));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["create_font"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("size"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("name"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("weight"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("italic"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("underline"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("strike"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("orientation"))->get_value()
		};

		auto size_var = std::make_shared<RuntimeVariable>("size", Type::T_INT);
		size_var->set_value(new RuntimeValue(vals[0]));
		vm->gc.add_var_root(size_var);

		auto name_var = std::make_shared<RuntimeVariable>("name", Type::T_STRING);
		name_var->set_value(new RuntimeValue(vals[1]));
		vm->gc.add_var_root(name_var);

		auto weight_var = std::make_shared<RuntimeVariable>("weight", Type::T_INT);
		weight_var->set_value(new RuntimeValue(vals[2]));
		vm->gc.add_var_root(weight_var);

		auto italic_var = std::make_shared<RuntimeVariable>("italic", Type::T_BOOL);
		italic_var->set_value(new RuntimeValue(vals[3]));
		vm->gc.add_var_root(italic_var);

		auto underline_var = std::make_shared<RuntimeVariable>("underline", Type::T_BOOL);
		underline_var->set_value(new RuntimeValue(vals[4]));
		vm->gc.add_var_root(underline_var);

		auto strike_var = std::make_shared<RuntimeVariable>("strike", Type::T_BOOL);
		strike_var->set_value(new RuntimeValue(vals[5]));
		vm->gc.add_var_root(strike_var);

		auto orientation_var = std::make_shared<RuntimeVariable>("orientation", Type::T_INT);
		orientation_var->set_value(new RuntimeValue(vals[6]));
		vm->gc.add_var_root(orientation_var);

		auto str = flx_struct();
		str["size"] = size_var;
		str["name"] = name_var;
		str["weight"] = weight_var;
		str["italic"] = italic_var;
		str["underline"] = underline_var;
		str["strike"] = strike_var;
		str["orientation"] = orientation_var;

		auto font = Font::create_font(
			str["size"]->get_value()->get_i(),
			str["name"]->get_value()->get_s(),
			str["weight"]->get_value()->get_i(),
			str["italic"]->get_value()->get_b(),
			str["underline"]->get_value()->get_b(),
			str["strike"]->get_value()->get_b(),
			str["orientation"]->get_value()->get_i()
		);
		if (!font) {
			throw std::runtime_error("there was an error creating font");
		}
		auto instance_var = std::make_shared<RuntimeVariable>(INSTANCE_ID_NAME, Type::T_INT);
		instance_var->set_value(new RuntimeValue(flx_int(font)));
		vm->gc.add_var_root(instance_var);
		str[INSTANCE_ID_NAME] = instance_var;

		// initialize image struct values
		RuntimeValue* font_value = vm->allocate_value(new RuntimeValue(str, Constants::STD_NAMESPACE, "Font"));

		vm->push_constant(font_value);

		};

	vm->builtin_functions["draw_text"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("text"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("color"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("font"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int x = (int)vals[1]->get_i();
		int y = (int)vals[2]->get_i();
		std::string text = vals[3]->get_s();
		int r = (int)vals[4]->get_str()["r"]->get_value()->get_i();
		int g = (int)vals[4]->get_str()["g"]->get_value()->get_i();
		int b = (int)vals[4]->get_str()["b"]->get_value()->get_i();

		RuntimeValue* font_value = vals[5];
		if (font_value->is_void()) {
			throw std::runtime_error("font is null");
		}
		Font* font = (Font*)font_value->get_str()[INSTANCE_ID_NAME]->get_value()->get_i();
		if (!font) {
			throw std::runtime_error("there was an error handling font");
		}

		((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->draw_text(x, y, text, Color(r, g, b), font);

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["get_text_size"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("text"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("font"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("Window is null");
		}
		if (!((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		std::string text = vals[1]->get_s();
		RuntimeValue* font_value = vals[2];
		if (font_value->is_void()) {
			throw std::runtime_error("font is null");
		}
		Font* font = (Font*)font_value->get_str()[INSTANCE_ID_NAME]->get_value()->get_i();
		if (!font) {
			throw std::runtime_error("there was an error handling font");
		}

		auto point = ((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->get_text_size(text, font);

		auto width_var = std::make_shared<RuntimeVariable>("width", Type::T_INT);
		width_var->set_value(new RuntimeValue(flx_int(point.width * 2 * 0.905)));
		vm->gc.add_var_root(width_var);

		auto height_var = std::make_shared<RuntimeVariable>("height", Type::T_INT);
		height_var->set_value(new RuntimeValue(flx_int(point.height * 2 * 0.875)));
		vm->gc.add_var_root(height_var);

		flx_struct str = flx_struct();
		str["width"] = width_var;
		str["height"] = height_var;

		vm->push_new_constant(new RuntimeValue(str, Constants::STD_NAMESPACE, "Size"));

		};

	vm->builtin_functions["load_image"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		auto path_var = std::make_shared<RuntimeVariable>("path", Type::T_STRING);
		path_var->set_value(new RuntimeValue(val));
		vm->gc.add_var_root(path_var);

		auto str = flx_struct();
		str["path"] = path_var;

		// loads image
		auto image = Image::load_image(str["path"]->get_value()->get_s());
		if (!image) {
			throw std::runtime_error("there was an error loading image");
		}
		auto instance_var = std::make_shared<RuntimeVariable>(INSTANCE_ID_NAME, Type::T_INT);
		instance_var->set_value(new RuntimeValue(flx_int(image)));
		vm->gc.add_var_root(instance_var);
		str[INSTANCE_ID_NAME] = instance_var;

		auto width_var = std::make_shared<RuntimeVariable>("width", Type::T_INT);
		width_var->set_value(new RuntimeValue(flx_int(image->width)));
		vm->gc.add_var_root(width_var);

		auto height_var = std::make_shared<RuntimeVariable>("height", Type::T_INT);
		height_var->set_value(new RuntimeValue(flx_int(image->height)));
		vm->gc.add_var_root(height_var);

		str["width"] = width_var;
		str["height"] = height_var;

		// initialize image struct values
		RuntimeValue* img = vm->allocate_value(new RuntimeValue(str, Constants::STD_NAMESPACE, "Image"));

		vm->push_constant(img);

		};

	vm->builtin_functions["draw_image"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("image"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y"))->get_value()
		};

		RuntimeValue* win = vals[0];
		if (win->is_void()) {
			throw std::runtime_error("window is null");
		}
		auto window = ((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i());
		if (!window) {
			throw std::runtime_error("there was an error handling window");
		}
		RuntimeValue* img = vals[1];
		if (img->is_void()) {
			throw std::runtime_error("window is null");
		}
		auto image = ((Image*)img->get_str()[INSTANCE_ID_NAME]->get_value()->get_i());
		if (!image) {
			throw std::runtime_error("there was an error handling image");
		}
		int x = (int)vals[2]->get_i();
		int y = (int)vals[3]->get_i();
		window->draw_image(image, x, y);

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["update"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value();

		RuntimeValue* win = val;
		if (!win->is_void()) {
			if (((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
				((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->update();
			}
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["destroy_window"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value();

		auto instance_id = val->get_raw_str()->at(INSTANCE_ID_NAME);
		if (auto window = (Window*)instance_id->get_value()->get_i()) {
			delete window;
			instance_id->get_value()->set(flx_int(0));
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["destroy_font"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("font"))->get_value();

		auto instance_id = val->get_raw_str()->at(INSTANCE_ID_NAME);
		if (auto font = (Font*)instance_id->get_value()->get_i()) {
			delete font;
			instance_id->get_value()->set(flx_int(0));
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["destroy_image"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("image"))->get_value();

		auto instance_id = val->get_raw_str()->at(INSTANCE_ID_NAME);
		if (auto img = (Image*)instance_id->get_value()->get_i()) {
			delete img;
			instance_id->get_value()->set(flx_int(0));
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["is_quit"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		RuntimeValue* win = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("window"))->get_value();
		auto val = vm->allocate_value(new RuntimeValue(Type::T_BOOL));
		if (!win->is_void()) {
			if (((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())) {
				val->set(flx_bool(((Window*)win->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->is_quit()));
			}
			else {
				val->set(flx_bool(true));
			}
		}
		else {
			val->set(flx_bool(true));
		}
		vm->push_constant(val);

		};

}
