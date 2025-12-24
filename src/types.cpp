#include "types.hpp"

#include <cmath>

#include "utils.hpp"
#include "exception_helper.hpp"
#include "module.hpp";
#include "md_datetime.hpp"
#include "md_graphics.hpp"
#include "md_files.hpp"
#include "md_console.hpp"
#include "visitor.hpp"
#include "token.hpp"
#include "constants.hpp"

using namespace core;


flx_array::flx_array()
	: _size(0), _data(nullptr) {
}

flx_array::flx_array(flx_int size)
	: _size(size), _data(std::make_unique<RuntimeValue* []>(size)) {
}

flx_int flx_array::size() const {
	return _size;
}

RuntimeValue*& flx_array::operator[](flx_int index) {
	return _data[index];
}

const RuntimeValue* flx_array::operator[](flx_int index) const {
	return _data[index];
}

flx_class::flx_class(std::string module_name_space, std::string module_name)
	: Scope(module_name_space, module_name) {
}

flx_class::flx_class()
	: Scope() {
}

TypeDefinition::TypeDefinition(Type type, const std::vector<std::shared_ptr<ASTExprNode>>& expr_dim,
	const std::string& type_name_space, const std::string& type_name)
	: type(type), expr_dim(expr_dim), type_name(type_name), type_name_space(type_name_space) {
	normalize();
}

TypeDefinition::TypeDefinition(Type type, const std::vector<size_t>& dim,
	const std::string& type_name_space, const std::string& type_name)
	: type(type), dim(dim), type_name(type_name), type_name_space(type_name_space) {
	normalize();
}

TypeDefinition::TypeDefinition(Type type,
	const std::string& type_name_space, const std::string& type_name)
	: type(type), type_name(type_name), type_name_space(type_name_space) {
	normalize();
}

TypeDefinition::TypeDefinition(Type type)
	: type(type) {
}

TypeDefinition::TypeDefinition()
	: type(Type::T_UNDEFINED) {
}

bool TypeDefinition::is_any_or_match_type_def(TypeDefinition rtype, bool strict, bool strict_array) {
	if (is_any() && !is_array()
		|| rtype.is_any() && !rtype.is_array()
		|| is_void()
		|| rtype.is_void()) {
		return true;
	}
	return match_type_def(rtype, strict, strict_array);
}

bool TypeDefinition::match_type_def(TypeDefinition rtype, bool strict, bool strict_array) {
	if (match_type_def_array(rtype, strict, strict_array)) return true;
	if (match_type_def_bool(rtype)) return true;
	if (match_type_def_int(rtype, strict)) return true;
	if (match_type_def_float(rtype, strict)) return true;
	if (match_type_def_char(rtype)) return true;
	if (match_type_def_string(rtype, strict)) return true;
	if (match_type_def_struct(rtype)) return true;
	if (match_type_def_class(rtype)) return true;
	if (match_type_def_function(rtype)) return true;
	return false;
}

bool TypeDefinition::match_type_def_bool(TypeDefinition rtype) {
	return is_bool() && rtype.is_bool();
}

bool TypeDefinition::match_type_def_int(TypeDefinition rtype, bool strict) {
	return is_int() && (strict && rtype.is_int() || !strict && rtype.is_numeric());
}

bool TypeDefinition::match_type_def_float(TypeDefinition rtype, bool strict) {
	return is_float() && (strict && rtype.is_float() || !strict && rtype.is_numeric());
}

bool TypeDefinition::match_type_def_char(TypeDefinition rtype) {
	return is_char() && rtype.is_char();
}

bool TypeDefinition::match_type_def_string(TypeDefinition rtype, bool strict) {
	return is_string() && (strict && rtype.is_string() || !strict && rtype.is_textual());
}

bool TypeDefinition::match_type_def_array(TypeDefinition rtype, bool strict, bool strict_array) {
	if (is_array() && rtype.is_array() && match_array_dim(rtype)) {
		auto base_type_l = TypeDefinition(type, std::vector<size_t>(), type_name_space, type_name);
		auto base_type_r = TypeDefinition(rtype.type, std::vector<size_t>(), rtype.type_name_space, rtype.type_name);
		return base_type_l.is_any_or_match_type_def(base_type_r, strict, strict_array);
	}
	return false;
}

bool TypeDefinition::match_type_def_struct(TypeDefinition rtype) {
	return is_struct() && rtype.is_struct()
		&& type_name_space == rtype.type_name_space
		&& type_name == rtype.type_name;
}

bool TypeDefinition::match_type_def_class(TypeDefinition rtype) {
	return is_class() && rtype.is_class()
		&& type_name_space == rtype.type_name_space
		&& type_name == rtype.type_name;
}

bool TypeDefinition::match_type_def_function(TypeDefinition rtype) {
	return is_function() && rtype.is_function();
}

bool TypeDefinition::match_array_dim(TypeDefinition rtype) {
	std::vector<size_t> var_dim = dim;
	std::vector<size_t> expr_dim = rtype.dim;

	if ((var_dim.size() == 1 && var_dim[0] >= 0 && var_dim[0] <= 1) || (expr_dim.size() == 1 && expr_dim[0] >= 0 && expr_dim[0] <= 1)
		|| var_dim.size() == 0 || expr_dim.size() == 0) {
		return true;
	}

	if (var_dim.size() != expr_dim.size()) {
		return false;
	}

	for (size_t dc = 0; dc < var_dim.size(); ++dc) {
		if (dim.at(dc) && var_dim.at(dc) != expr_dim.at(dc)) {
			return false;
		}
	}

	return true;
}

bool TypeDefinition::match_type(TypeDefinition rtype) const {
	return type == rtype.type;
}

bool TypeDefinition::is_undefined() const {
	return match_type(TypeDefinition(Type::T_UNDEFINED));
}

bool TypeDefinition::is_void() const {
	return match_type(TypeDefinition(Type::T_VOID));
}

bool TypeDefinition::is_bool() const {
	return match_type(TypeDefinition(Type::T_BOOL));
}

bool TypeDefinition::is_int() const {
	return match_type(TypeDefinition(Type::T_INT));
}

bool TypeDefinition::is_float() const {
	return match_type(TypeDefinition(Type::T_FLOAT));
}

bool TypeDefinition::is_char() const {
	return match_type(TypeDefinition(Type::T_CHAR));
}

bool TypeDefinition::is_string() const {
	return match_type(TypeDefinition(Type::T_STRING));
}

bool TypeDefinition::is_any() const {
	return match_type(TypeDefinition(Type::T_ANY));
}

bool TypeDefinition::is_object() const {
	return match_type(TypeDefinition(Type::T_OBJECT));
}

bool TypeDefinition::is_class() const {
	return match_type(TypeDefinition(Type::T_CLASS));
}

bool TypeDefinition::is_struct() const {
	return match_type(TypeDefinition(Type::T_STRUCT));
}

bool TypeDefinition::is_function() const {
	return match_type(TypeDefinition(Type::T_FUNCTION));
}

bool TypeDefinition::is_textual() const {
	return is_string() || is_char();
}

bool TypeDefinition::is_numeric() const {
	return is_int() || is_float();
}
bool TypeDefinition::is_array() const {
	return dim.size() > 0 || expr_dim.size() > 0;
}

bool TypeDefinition::is_collection() const {
	return is_string() || is_array();
}

bool TypeDefinition::is_iterable() const {
	return is_collection() || is_struct();
}

std::string TypeDefinition::type_str(Type t) {
	switch (t) {
	case Type::T_UNDEFINED:
		return "undefined";
	case Type::T_VOID:
		return "void";
	case Type::T_BOOL:
		return "bool";
	case Type::T_INT:
		return "int";
	case Type::T_FLOAT:
		return "float";
	case Type::T_CHAR:
		return "char";
	case Type::T_STRING:
		return "string";
	case Type::T_ANY:
		return "any";
	case Type::T_OBJECT:
		return "object";
	case Type::T_STRUCT:
		return "struct";
	case Type::T_CLASS:
		return "class";
	case Type::T_FUNCTION:
		return "function";
	default:
		throw std::runtime_error("invalid type encountered");
	}
}

std::string TypeDefinition::buid_type_str(const TypeDefinition& type_def) {
	std::string ss;

	auto type = type_def.type;

	if (type_def.is_struct()) {
		ss = type_def.type_name;
	}
	else {
		ss = TypeDefinition::type_str(type);
	}

	if (type_def.dim.size() > 0) {
		auto& dim = type_def.dim;
		for (size_t i = 0; i < dim.size(); ++i) {
			ss += "[";
			if (dim[i] > 0) {
				ss += std::to_string(dim[i]);
			}
			ss += "]";
		}
	}

	if (type_def.is_struct()) {
		ss = TypeDefinition::buid_struct_type_name(type_def.type_name_space, ss);
	}

	return ss;
}

std::string TypeDefinition::buid_struct_type_name(const std::string& type_name_space, const std::string& type_name) {
	return (type_name_space == Constants::DEFAULT_NAMESPACE
		|| type_name_space.empty() ? "" : type_name_space + "::") + type_name;
}

void TypeDefinition::normalize() {
	if (is_struct()) {
		if (type_name.empty()) {
			throw std::runtime_error("struct type name cannot be empty");
		}
		if (type_name_space.empty()) {
			type_name_space = Constants::DEFAULT_NAMESPACE;
		}
	}
}

VariableDefinition::VariableDefinition(
	const std::string& identifier,
	TypeDefinition type_definition,
	std::shared_ptr<ASTExprNode> default_value,
	bool is_rest,
	bool is_const
)
	: TypeDefinition(type_definition),
	identifier(identifier),
	default_value(default_value),
	is_rest(is_rest),
	is_const(is_const) {
}

VariableDefinition::VariableDefinition(
	const std::string& identifier,
	TypeDefinition type_definition,
	size_t default_value_pc,
	bool is_rest,
	bool is_const
)
	: TypeDefinition(type_definition),
	identifier(identifier),
	default_value(default_value_pc),
	is_rest(is_rest),
	is_const(is_const) {
}

UnpackedVariableDefinition::UnpackedVariableDefinition(
	TypeDefinition type_definition,
	const std::vector<VariableDefinition>& variables
)
	: TypeDefinition(type_definition),
	variables(variables) {
}

VariableDefinition::VariableDefinition()
	: TypeDefinition() {
}
std::shared_ptr<ASTExprNode> VariableDefinition::get_expr_default() const {
	return std::get<std::shared_ptr<ASTExprNode>>(default_value);
}

size_t VariableDefinition::get_pc_default() const {
	return std::get<size_t>(default_value);
}

FunctionDefinition::FunctionDefinition(
	const std::string& identifier,
	TypeDefinition type_definition,
	const std::vector<std::shared_ptr<TypeDefinition>>& parameters,
	std::shared_ptr<ASTBlockNode> block
)
	: TypeDefinition(type_definition),
	identifier(identifier),
	parameters(parameters),
	block(block) {
	check_signature();
}

FunctionDefinition::FunctionDefinition(const std::string& identifier)
	: TypeDefinition(Type::T_ANY), identifier(identifier) {
}

FunctionDefinition::FunctionDefinition()
	: TypeDefinition() {
}

void FunctionDefinition::check_signature() const {
	bool has_default = false;
	for (size_t i = 0; i < parameters.size(); ++i) {
		if (auto parameter = std::dynamic_pointer_cast<VariableDefinition>(parameters[i])) {
			if (parameter->is_rest && parameters.size() - 1 != i) {
				throw std::runtime_error("rest '" + identifier + "' parameter must be the last parameter");
			}
			if (parameter->get_expr_default()) {
				has_default = true;
			}
			if (!parameter->get_expr_default() && has_default) {
				throw std::runtime_error("default values as '" + identifier + "' must be at end");
			}
		}
	}
}

StructDefinition::StructDefinition(
	const std::string& identifier,
	const std::map<std::string, std::shared_ptr<VariableDefinition>>& variables
)
	: identifier(identifier),
	variables(variables) {
}

StructDefinition::StructDefinition(const std::string& identifier)
	: identifier(identifier) {
}

StructDefinition::StructDefinition()
	: identifier(""), variables(std::map<std::string, std::shared_ptr<VariableDefinition>>()) {
}

ClassDefinition::ClassDefinition(
	const std::string& identifier,
	const std::vector<std::shared_ptr<ASTDeclarationNode>>& declarations,
	const std::vector<std::shared_ptr<ASTFunctionDefinitionNode>>& functions
)
	: identifier(identifier),
	declarations(declarations),
	functions(functions) {
}

ClassDefinition::ClassDefinition(const std::string& identifier)
	: identifier(identifier) {
}

ClassDefinition::ClassDefinition() {}

Variable::Variable(const std::string& identifier, TypeDefinition type_definition)
	: TypeDefinition(type_definition), identifier(identifier) {
	def_type();
}

Variable::Variable()
	: TypeDefinition() {
}

void Variable::def_type() {
	type = is_void() || is_undefined() ? Type::T_ANY : type;
}

Value::Value(TypeDefinition type_definition, bool is_constexpr)
	: TypeDefinition(type_definition), is_constexpr(is_constexpr) {
}

Value::Value()
	: TypeDefinition() {
}

SemanticValue::SemanticValue(TypeDefinition type_definition, intmax_t hash, bool is_constexpr)
	: Value(type_definition, is_constexpr),
	hash(hash) {
}

SemanticValue::SemanticValue()
	: Value() {
}

void SemanticValue::set_b(flx_bool b) {
	this->b = b;
	this->hash = static_cast<intmax_t>(b);
}

void SemanticValue::set_i(flx_int i) {
	this->i = i;
	this->hash = static_cast<intmax_t>(i);
}

void SemanticValue::set_f(flx_float f) {
	this->f = f;
	this->hash = static_cast<intmax_t>(std::floor(f));
}

void SemanticValue::set_c(flx_char c) {
	this->c = c;
	this->hash = static_cast<intmax_t>(c);
}

void SemanticValue::set_s(const flx_string& s) {
	this->s = s;
	this->hash = utils::StringUtils::hashcode(s);
}

void SemanticValue::copy_from(const SemanticValue& value) {
	type = value.type;
	dim = value.dim;
	type_name = value.type_name;
	type_name_space = value.type_name_space;
	hash = value.hash;
	is_constexpr = value.is_constexpr;
}

SemanticVariable::SemanticVariable(const std::string& identifier, TypeDefinition type_definition, bool is_const)
	: Variable(identifier, type_definition), is_const(is_const) {
}

SemanticVariable::SemanticVariable()
	: Variable() {
}

void SemanticVariable::set_value(std::shared_ptr<SemanticValue> value) {
	this->value = value;
	this->value->ref = shared_from_this();
}

std::shared_ptr<SemanticValue> SemanticVariable::get_value() {
	value->ref = shared_from_this();
	return value;
}

RuntimeValue::RuntimeValue(flx_bool rawv)
	: Value(TypeDefinition(Type::T_BOOL)) {
	set(rawv);
}

RuntimeValue::RuntimeValue(flx_int rawv)
	: Value(TypeDefinition(Type::T_INT)) {
	set(rawv);
}

RuntimeValue::RuntimeValue(flx_float rawv)
	: Value(TypeDefinition(Type::T_FLOAT)) {
	set(rawv);
}

RuntimeValue::RuntimeValue(flx_char rawv)
	: Value(TypeDefinition(Type::T_CHAR)) {
	set(rawv);
}

RuntimeValue::RuntimeValue(flx_string rawv)
	: Value(TypeDefinition(Type::T_STRING)) {
	set(rawv);
}

RuntimeValue::RuntimeValue(flx_array rawv, Type array_type, std::vector<size_t> dim, std::string type_name_space, std::string type_name)
	: Value(TypeDefinition(array_type, dim, type_name_space, type_name)) {
	set(rawv, array_type, dim, type_name_space, type_name);
}

RuntimeValue::RuntimeValue(flx_struct rawv, std::string type_name_space, std::string type_name)
	: Value(TypeDefinition(Type::T_STRUCT, type_name_space, type_name)) {
	set(rawv, type_name_space, type_name);
}

RuntimeValue::RuntimeValue(flx_class rawv, std::string type_name_space, std::string type_name)
	: Value(TypeDefinition(Type::T_CLASS, type_name_space, type_name)) {
	set(rawv, type_name_space, type_name);
}

RuntimeValue::RuntimeValue(flx_function rawv)
	: Value(TypeDefinition(Type::T_FUNCTION)) {
	set(rawv);
}

RuntimeValue::RuntimeValue(Type t)
	: Value(TypeDefinition(t)) {
}

RuntimeValue::RuntimeValue(RuntimeValue* v) {
	copy_from(v);
}

RuntimeValue::RuntimeValue(TypeDefinition v)
	: Value(v) {
}

RuntimeValue::RuntimeValue()
	: Value() {
}

RuntimeValue::~RuntimeValue() {
	unset();
};

void RuntimeValue::set(flx_bool b) {
	unset();
	this->b = new flx_bool(b);
	type = Type::T_BOOL;
}

void RuntimeValue::set(flx_int i) {
	unset();
	this->i = new flx_int(i);
	type = Type::T_INT;
}

void RuntimeValue::set(flx_float f) {
	unset();
	this->f = new flx_float(f);
	type = Type::T_FLOAT;
}

void RuntimeValue::set(flx_char c) {
	unset();
	this->c = new flx_char(c);
	type = Type::T_CHAR;
}

void RuntimeValue::set(flx_string s) {
	unset();
	this->s = new flx_string(s);
	type = Type::T_STRING;
}

void RuntimeValue::set(flx_array arr, Type array_type, std::vector<size_t> dim, std::string type_name_space, std::string type_name) {
	unset();
	this->arr = std::make_shared<flx_array>(arr);
	type = array_type;
	this->dim = dim;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void RuntimeValue::set(flx_struct str, std::string type_name_space, std::string type_name) {
	unset();
	this->str = std::make_shared<flx_struct>(str);
	type = Type::T_STRUCT;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void RuntimeValue::set(flx_class cls, std::string type_name_space, std::string type_name) {
	unset();
	this->cls = std::make_shared<flx_class>(cls);
	type = Type::T_CLASS;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void RuntimeValue::set(flx_function fun) {
	unset();
	this->fun = new flx_function(fun);
	type = Type::T_FUNCTION;
}

void RuntimeValue::set_field(std::string identifier, RuntimeValue* sub_value) {
	if (!str) return;
	sub_value->access_identifier = identifier;
	(*str)[identifier]->set_value(sub_value);
}

void RuntimeValue::set_item(size_t index, RuntimeValue* sub_value) {
	if (!arr) return;
	sub_value->access_index = index;
	(*arr)[index] = sub_value;
}

void RuntimeValue::set_char(size_t index, RuntimeValue* sub_value) {
	if (!s) return;
	sub_value->access_index = index;
	(*s)[index] = sub_value->get_c();
}

flx_bool RuntimeValue::get_b() const {
	if (!b) return flx_bool();
	return *b;
}

flx_int RuntimeValue::get_i() const {
	if (!i) return flx_int();
	return *i;
}

flx_float RuntimeValue::get_f() const {
	if (!f) return flx_float();
	return *f;
}

flx_char RuntimeValue::get_c() const {
	if (!c) return flx_char();
	return *c;
}

flx_string RuntimeValue::get_s() const {
	if (!s) return flx_string();
	return *s;
}

flx_array RuntimeValue::get_arr() const {
	if (!arr) return flx_array();
	return *arr;
}

flx_struct RuntimeValue::get_str() const {
	if (!str) return flx_struct();
	return *str;
}

flx_class RuntimeValue::get_cls() const {
	if (!cls) return flx_class();
	return *cls;
}

flx_function RuntimeValue::get_fun() const {
	if (!fun) return flx_function();
	return *fun;
}

RuntimeValue* RuntimeValue::get_field(std::string identifier, bool use_holder_reference) {
	if (!str) return nullptr;
	auto sub_value = std::dynamic_pointer_cast<RuntimeVariable>((*str)[identifier])->get_value(use_holder_reference);
	if (use_holder_reference && sub_value) {
		//value_ref = nullptr;
		//sub_value->value_ref = this;
		ref.reset();
		sub_value->access_identifier = identifier;
	}
	return sub_value;
}

RuntimeValue* RuntimeValue::get_item(flx_int index, bool use_holder_reference) {
	if (!arr) return nullptr;
	if (index < 0 || index >= (*arr).size()) {
		throw std::runtime_error("invalid array access position " + std::to_string(index) + " in a array with size " + std::to_string((*arr).size()));
	}
	auto sub_value = (*arr)[index];
	if (use_holder_reference && sub_value) {
		value_ref = nullptr;
		sub_value->value_ref = this;
		sub_value->access_index = index;
	}
	return sub_value;
}

RuntimeValue* RuntimeValue::get_char(flx_int index, bool use_holder_reference) {
	if (!s) return nullptr;
	if (index < 0 || index >= (*s).size()) {
		throw std::runtime_error("invalid string access position " + std::to_string(index) + " in a string with size " + std::to_string((*s).size()));
	}
	auto sub_value = new RuntimeValue(flx_char((*s)[index]));
	if (use_holder_reference) {
		value_ref = nullptr;
		sub_value->value_ref = this;
		sub_value->access_index = index;
	}

	return sub_value;
}

flx_bool* RuntimeValue::get_raw_b() {
	return b;
}

flx_int* RuntimeValue::get_raw_i() {
	return i;
}

flx_float* RuntimeValue::get_raw_f() {
	return f;
}

flx_char* RuntimeValue::get_raw_c() {
	return c;
}

flx_string* RuntimeValue::get_raw_s() {
	return s;
}

std::shared_ptr<flx_array> RuntimeValue::get_raw_arr() {
	return arr;
}

std::shared_ptr<flx_struct> RuntimeValue::get_raw_str() {
	return str;
}

std::shared_ptr<flx_class> RuntimeValue::get_raw_cls() {
	return cls;
}

flx_function* RuntimeValue::get_raw_fun() {
	return fun;
}

void RuntimeValue::unset() {
	access_identifier = "";
	access_index = 0;

	if (this->b) {
		delete this->b;
		this->b = nullptr;
	}
	if (this->i) {
		delete this->i;
		this->i = nullptr;
	}
	if (this->f) {
		delete this->f;
		this->f = nullptr;
	}
	if (this->c) {
		delete this->c;
		this->c = nullptr;
	}
	if (this->s) {
		delete this->s;
		this->s = nullptr;
	}
	if (this->arr) {
		this->arr = nullptr;
	}
	if (this->str) {
		this->str = nullptr;
	}
	if (this->fun) {
		delete this->fun;
		this->fun = nullptr;
	}
}

void RuntimeValue::set_null() {
	auto v = RuntimeValue(Type::T_VOID);
	copy_from(&v);
}

bool RuntimeValue::has_value() {
	return !is_undefined() && !is_void();
}

void RuntimeValue::copy_from(RuntimeValue* value) {
	type = value->type;
	type_name = value->type_name;
	type_name_space = value->type_name_space;
	dim = value->dim;
	ref = value->ref;
	unset();

	if (is_array()) {
		arr = value->get_raw_arr();
	}
	else if (is_bool()) {
		b = new flx_bool(value->get_b());
	}
	else if (is_int()) {
		i = new flx_int(value->get_i());
	}
	else if (is_float()) {
		f = new flx_float(value->get_f());
	}
	else if (is_char()) {
		c = new flx_char(value->get_c());
	}
	else if (is_string()) {
		s = new flx_string(value->get_s());
	}
	else if (is_struct()) {
		str = value->get_raw_str();
	}
	else if (is_class()) {
		cls = value->get_raw_cls();
	}
	else if (is_function()) {
		fun = new flx_function(value->get_fun());
	}

}

std::vector<runtime::GCObject*> RuntimeValue::get_references() {
	std::vector<GCObject*> references;

	if (is_array()) {
		auto arr = get_arr();
		for (size_t i = 0; i < arr.size(); ++i) {
			const auto& v = arr[i];
			references.push_back(v);
		}
	}
	else if (is_struct()) {
		for (const auto& var : get_str()) {
			auto var_ref = std::dynamic_pointer_cast<RuntimeVariable>(var.second)->get_references();
			references.insert(references.end(), var_ref.begin(), var_ref.end());
		}
	}
	else if (is_class()) {
		const auto& class_vars = get_raw_cls()->variable_symbol_table;
		for (const auto& var : class_vars) {
			auto var_ref = std::dynamic_pointer_cast<RuntimeVariable>(var.second)->get_references();
			references.insert(references.end(), var_ref.begin(), var_ref.end());
		}
	}

	return references;
}

RuntimeVariable::RuntimeVariable(const std::string& identifier, TypeDefinition v)
	: Variable(identifier, v) {
}

RuntimeVariable::RuntimeVariable()
	: Variable() {
}

RuntimeVariable::~RuntimeVariable() = default;

void RuntimeVariable::set_value(RuntimeValue* val) {
	value = val;
	value->ref = shared_from_this();
}

RuntimeValue* RuntimeVariable::get_value(bool use_variable_ref) {
	if (use_variable_ref) {
		value->ref = shared_from_this();
	}

	return value;
}

std::vector<runtime::GCObject*> RuntimeVariable::get_references() {
	std::vector<GCObject*> references;

	references.push_back(value);

	return references;
}

SemanticValue SemanticOperations::do_operation(const std::string& op, SemanticValue lval, SemanticValue rval) {
	std::shared_ptr<SemanticVariable> var_ref = lval.ref;

	// handle direct variable assign
	if (var_ref && Token::is_assignment_op(op)) {
		auto assign_value = rval;

		if (op != "=") {
			assign_value = do_operation(std::string{ op[0] }, lval, rval);
		}

		SemanticOperations::normalize_type(*var_ref, &assign_value);

		if (!var_ref->is_any_or_match_type_def(assign_value)) {
			ExceptionHelper::throw_operation_err(op, *var_ref, assign_value);
		}

		var_ref->set_value(std::make_shared<SemanticValue>(assign_value));
		return assign_value;
	}

	// if any side is null, is a null comparison
	if ((lval.is_void() || rval.is_void()) && Token::is_equality_op(op)) {
		return SemanticValue(Type::T_BOOL);
	}

	// if is a null assign
	if ((lval.is_void() || rval.is_void()) && op == "=") {
		return rval;
	}

	if (op == "in") {
		auto result = SemanticValue(TypeDefinition(Type::T_BOOL));

		if (lval.is_any() && rval.is_any()) {
			return result;
		}
		if (!rval.is_collection()) {
			throw std::runtime_error("invalid type '" + TypeDefinition::buid_type_str(rval) + "', value must be a array or string");
		}
		if (!lval.match_type(rval)
			&& lval.is_string() && !rval.is_string()
			&& lval.is_char() && !rval.is_string()) {
			throw std::runtime_error("types don't match '" + TypeDefinition::buid_type_str(lval) + "' and '" + TypeDefinition::buid_type_str(rval) + "'");
		}

		return result;
	}

	if (lval.is_any() || rval.is_any()) {
		return SemanticValue(Type::T_ANY);
	}

	// type specifics

	if (lval.is_array()) {
		if (rval.is_array() && Token::is_equality_op(op)) {
			return SemanticValue(Type::T_BOOL);
		}


		if (!rval.match_type_def_array(lval) && op != "+") {
			ExceptionHelper::throw_operation_err(op, lval, rval);
		}

		bool match_arr_t = lval.type == rval.type || lval.is_any();

		auto new_dim = lval.dim;
		for (auto d : rval.dim) {
			new_dim.push_back(d);
		}

		return SemanticValue(TypeDefinition(
			match_arr_t ? lval.type : Type::T_ANY, new_dim, lval.type_name, lval.type_name_space
		));
	}
	else if (lval.is_bool()) {
		if (!rval.is_bool()) {
			ExceptionHelper::throw_operation_err(op, lval, rval);
		}

		if (Token::is_bool_op(op) || Token::is_equality_op(op)) {
			return SemanticValue(Type::T_BOOL);
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}
	else if (lval.is_int()) {
		if (rval.is_numeric() && op == "<=>") {
			return SemanticValue(Type::T_INT);
		}

		if (rval.is_numeric() && (Token::is_relational_op(op) || Token::is_equality_op(op))) {
			return SemanticValue(Type::T_BOOL);
		}

		if (rval.is_int() || rval.is_float()) {
			if (op == "/" || op == "/%") {
				return SemanticValue(Type::T_FLOAT);
			}
			else if (Token::is_int_op(op)) {
				return SemanticValue(Type::T_INT);
			}
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}
	else if (lval.is_float()) {
		if (rval.is_numeric() && op == "<=>") {
			return SemanticValue(Type::T_INT);
		}

		if (rval.is_numeric() && (Token::is_relational_op(op) || Token::is_equality_op(op))) {
			return SemanticValue(Type::T_BOOL);
		}

		if ((rval.is_float() || rval.is_int()) && Token::is_float_op(op)) {
			return SemanticValue(Type::T_FLOAT);
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}
	else if (lval.is_char()) {
		if (rval.is_char() && Token::is_equality_op(op)) {
			return SemanticValue(Type::T_BOOL);
		}
		if ((rval.is_char() || rval.is_string()) && (op == "+" || Token::is_assignment_collection_op(op))) {
			return SemanticValue(Type::T_STRING);
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}
	else if (lval.is_string()) {
		if (rval.is_string() && Token::is_equality_op(op)) {
			return SemanticValue(Type::T_BOOL);
		}

		if ((rval.is_string() || rval.is_char()) && (op == "+" || Token::is_assignment_collection_op(op))) {
			return SemanticValue(Type::T_STRING);
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}
	else if (lval.is_struct()) {
		if (rval.is_struct() && Token::is_equality_op(op)) {
			return SemanticValue(Type::T_BOOL);
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}
	else if (lval.is_class()) {
		if (rval.is_class() && Token::is_equality_op(op)) {
			return SemanticValue(Type::T_BOOL);
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}
	else if (lval.is_function()) {
		if (rval.is_function() && Token::is_equality_op(op)) {
			return SemanticValue(Type::T_BOOL);
		}

		ExceptionHelper::throw_operation_err(op, lval, rval);

	}

	throw std::runtime_error("cannot determine type of operation");

}

void SemanticOperations::normalize_type(TypeDefinition owner, SemanticValue* value) {
	if (owner.is_array() || value->is_array()) {
		return;
	}

	if (owner.is_string() && value->is_char()) {
		value->type = owner.type;
	}
	else if (owner.is_float() && value->is_int()) {
		value->type = owner.type;
	}
	else if (owner.is_int() && value->is_float()) {
		value->type = owner.type;
	}
}

flx_bool RuntimeOperations::equals_value(const RuntimeValue* lval, const RuntimeValue* rval) {
	if (lval->is_void()) {
		return rval->is_void();
	}
	else if (lval->is_bool()) {
		return lval->get_b() == rval->get_b();
	}
	else if (lval->is_int()) {
		return lval->get_i() == rval->get_i();
	}
	else if (lval->is_float()) {
		return lval->get_f() == rval->get_f();
	}
	else if (lval->is_char()) {
		return lval->get_c() == rval->get_c();
	}
	else if (lval->is_string()) {
		return lval->get_s() == rval->get_s();
	}
	else if (lval->is_function()) {
		return lval->get_fun() == rval->get_fun();
	}
	else if (lval->is_struct() || lval->is_class() || lval->is_array()) {
		return lval == rval;
	}
	
	return false;
}

std::string RuntimeOperations::parse_value_to_string(
	const RuntimeValue* value,
	bool print_complex_types,
	std::vector<uintptr_t> printed
) {
	if (!value) {
		return "null";
	}

	std::string str = "";

	if (value->is_array()) {
		std::stringstream s;
		s << TypeDefinition::buid_type_str(*value) << "<array@0x" << std::hex << reinterpret_cast<uintptr_t>(value) << ">";
		str = s.str();

		if (print_complex_types) {
			if (std::find(printed.begin(), printed.end(), reinterpret_cast<uintptr_t>(value)) != printed.end()) {
				str += "{...}";
			}
			else {
				printed.push_back(reinterpret_cast<uintptr_t>(value));
				str += RuntimeOperations::parse_array_to_string(value, print_complex_types, printed);
			}
		}

		return str;
	}

	switch (value->type) {
	case Type::T_VOID:
		str = "null";
		break;
	case Type::T_BOOL:
		str = ((value->get_b()) ? "true" : "false");
		break;
	case Type::T_INT:
		str = std::to_string(value->get_i());
		break;
	case Type::T_FLOAT:
		str = std::to_string(value->get_f());
		str.erase(str.find_last_not_of('0') + 1, std::string::npos);
		if (!str.empty() && str.back() == '.') {
			str += "0";
		}
		break;
	case Type::T_CHAR:
		str = flx_string(std::string{ value->get_c() });
		break;
	case Type::T_STRING:
		str = value->get_s();
		break;
	case Type::T_CLASS: {
		std::stringstream s = std::stringstream();
		if (!value->type_name_space.empty()) {
			s << value->type_name_space << "::";
		}
		s << value->type_name << "<class@0x" << std::hex << reinterpret_cast<uintptr_t>(value) << ">";
		str = s.str();

		if (print_complex_types) {
			if (std::find(printed.begin(), printed.end(), reinterpret_cast<uintptr_t>(value)) != printed.end()) {
				str += "{...}";
			}
			else {
				printed.push_back(reinterpret_cast<uintptr_t>(value));
				str += RuntimeOperations::parse_class_to_string(value, print_complex_types, printed);
			}
		}

		break;
	}
	case Type::T_STRUCT: {
		std::stringstream s = std::stringstream();
		if (!value->type_name_space.empty()) {
			s << value->type_name_space << "::";
		}
		s << value->type_name << "<struct@0x" << std::hex << reinterpret_cast<uintptr_t>(value) << ">";
		str = s.str();

		if (print_complex_types) {
			if (std::find(printed.begin(), printed.end(), reinterpret_cast<uintptr_t>(value)) != printed.end()) {
				str += "{...}";
			}
			else {
				printed.push_back(reinterpret_cast<uintptr_t>(value));
				str += RuntimeOperations::parse_struct_to_string(value, print_complex_types, printed);
			}
		}

		break;
	}
	case Type::T_FUNCTION: {
		str = value->get_fun().first + (value->get_fun().first.empty() ? "" : "::") + value->get_fun().second + "(...)";
		break;
	}
	case Type::T_UNDEFINED:
		throw std::runtime_error("undefined expression");
	default:
		throw std::runtime_error("can't determine value type on parsing");
	}
	return str;
}

std::string RuntimeOperations::parse_array_to_string(
	const RuntimeValue* value,
	bool print_complex_types,
	std::vector<uintptr_t> printed
) {
	auto arr_value = value->get_arr();
	std::stringstream s = std::stringstream();
	s << "{";
	for (auto i = 0; i < arr_value.size(); ++i) {
		bool isc = arr_value[i] && arr_value[i]->is_char();
		bool iss = arr_value[i] && arr_value[i]->is_string();

		if (isc) s << "'";
		else if (iss) s << '"';

		s << parse_value_to_string(arr_value[i], print_complex_types, printed);

		if (isc) s << "'";
		else if (iss) s << '"';

		if (i < arr_value.size() - 1) {
			s << ",";
		}
	}
	s << "}";
	return s.str();
}

std::string RuntimeOperations::parse_class_to_string(
	const RuntimeValue* value,
	bool print_complex_types,
	std::vector<uintptr_t> printed
) {
	auto cls_value = value->get_cls();
	std::stringstream s = std::stringstream();
	s << "{";
	for (auto const& [key, val] : cls_value.variable_symbol_table) {
		bool isc = val && val->is_char();
		bool iss = val && val->is_string();

		s << key << ":";

		if (isc) s << "'";
		else if (iss) s << '"';

		s << parse_value_to_string(std::dynamic_pointer_cast<RuntimeVariable>(val)->get_value(), print_complex_types, printed);

		if (isc) s << "'";
		else if (iss) s << '"';

		s << ";";
	}
	for (auto const& [key, val] : cls_value.function_symbol_table) {
		s << ExceptionHelper::buid_signature(val->identifier, val->parameters) << ";";
	}
	s << "}";
	return s.str();
}

std::string RuntimeOperations::parse_struct_to_string(
	const RuntimeValue* value,
	bool print_complex_types,
	std::vector<uintptr_t> printed
) {
	auto str_value = value->get_str();
	std::stringstream s = std::stringstream();
	s << "{";
	for (auto const& [key, val] : str_value) {
		bool isc = val && val->is_char();
		bool iss = val && val->is_string();

		s << key << ":";

		if (isc) s << "'";
		else if (iss) s << '"';

		s << parse_value_to_string(std::dynamic_pointer_cast<RuntimeVariable>(val)->get_value(), print_complex_types, printed);

		if (isc) s << "'";
		else if (iss) s << '"';

		s << ";";
	}
	s << "}";
	return s.str();
}

RuntimeValue* RuntimeOperations::do_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
	std::shared_ptr<RuntimeVariable> var_ref = lval->ref.lock() ?
		std::dynamic_pointer_cast<RuntimeVariable>(lval->ref.lock()) : nullptr;
	RuntimeValue* val_ref = lval->value_ref;

	// handle subvalue assign
	if (val_ref && Token::is_assignment_op(op)) {
		lval->value_ref = nullptr;

		if (!lval->is_any_or_match_type_def(*rval)) {
			ExceptionHelper::throw_operation_err(op, *lval, *rval);
		}

		auto assign_value = rval;

		if (op != "=") {
			auto ltval = RuntimeValue(lval);
			auto rtval = RuntimeValue(rval);
			assign_value = RuntimeOperations::do_operation(std::string{ op[0] }, &ltval, &rtval);
		}

		if (val_ref->is_array()) {
			val_ref->set_item(lval->access_index, assign_value);

			return assign_value;
		}
		else if (val_ref->is_string()) {
			val_ref->set_char(lval->access_index, assign_value);

			return assign_value;
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);
	}

	// handle direct variable assign
	if (var_ref && Token::is_assignment_op(op)) {
		lval->ref.reset();
		bool new_ref = true;

		auto assign_value = rval;

		if (op != "=") {
			auto ltval = RuntimeValue(lval);
			auto rtval = RuntimeValue(rval);
			assign_value = RuntimeOperations::do_operation(std::string{ op[0] }, &ltval, &rtval);
			new_ref = false;
		}

		assign_value = RuntimeOperations::normalize_type(var_ref, assign_value, new_ref);

		if (!var_ref->is_any_or_match_type_def(*rval)) {
			ExceptionHelper::throw_operation_err(op, *var_ref, *rval);
		}

		var_ref->set_value(assign_value);

		return assign_value;
	}

	if (op == "in") {
		auto res = false;

		if (rval->is_array()) {
			flx_array expr_col = rval->get_arr();

			for (size_t i = 0; i < expr_col.size(); ++i) {
				res = RuntimeOperations::equals_value(lval, expr_col[i]);
				if (res) {
					break;
				}
			}
		}
		else {
			const auto& expr_col = rval->get_s();

			if (lval->is_char()) {
				res = rval->get_s().find(lval->get_c()) != std::string::npos;
			}
			else {
				res = rval->get_s().find(lval->get_s()) != std::string::npos;
			}
		}

		return new RuntimeValue(flx_bool(res));
	}

	// if any side is null, it's a null comparison
	if ((lval->is_void() || rval->is_void()) && Token::is_equality_op(op)) {
		return new RuntimeValue((flx_bool)((op == "==") ? lval->match_type(*rval) : !lval->match_type(*rval)));
	}

	if (Token::is_assignment_op(op)) {
		throw std::runtime_error("assigning operation can only be performed in referenced values");
	}

	// type specifics

	if (lval->is_array()) {
		if (rval->is_array() && Token::is_equality_op(op)) {
			return new RuntimeValue(flx_bool(op == "==" ? equals_value(lval, rval) : !equals_value(lval, rval)));
		}

		if (!rval->match_type_def_array(*lval) && op != "+") {
			ExceptionHelper::throw_operation_err(op, *lval, *rval);
		}

		bool match_arr_t = lval->type == rval->type || lval->is_any();

		auto new_dim = lval->dim;
		new_dim.at(new_dim.size() - 1) += rval->dim.at(rval->dim.size() - 1);

		return new RuntimeValue(
			do_operation(lval->get_arr(), rval->get_arr(), op),
			match_arr_t ? lval->type : Type::T_ANY, new_dim, lval->type_name, lval->type_name_space
		);
	}
	else if (lval->is_bool()) {
		if (!rval->is_bool()) {
			ExceptionHelper::throw_operation_err(op, *lval, *rval);
		}

		if (op == "and") {
			return new RuntimeValue((flx_bool)(lval->get_b() && rval->get_b()));
		}
		else if (op == "or") {
			return new RuntimeValue((flx_bool)(lval->get_b() || rval->get_b()));
		}
		else if (op == "==") {
			return new RuntimeValue((flx_bool)(lval->get_b() == rval->get_b()));
		}
		else if (op == "!=") {
			return new RuntimeValue((flx_bool)(lval->get_b() != rval->get_b()));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}
	else if (lval->is_int()) {
		if (rval->is_numeric() && op == "<=>") {
			return new RuntimeValue(do_spaceship_operation(op, lval, rval));
		}

		if (rval->is_numeric() && Token::is_relational_op(op)) {
			return new RuntimeValue(do_relational_operation(op, lval, rval));
		}

		if (rval->is_numeric() && Token::is_equality_op(op)) {
			flx_float l = lval->get_i();
			flx_float r = rval->is_float() ? rval->get_f() : rval->get_i();

			return new RuntimeValue(flx_bool(op == "==" ? l == r : l != r));
		}

		if (rval->is_float()) {
			return new RuntimeValue(do_operation(flx_float(lval->get_i()), rval->get_f(), op));
		}
		else if (rval->is_int()) {
			if (op == "/" || op == "/%") {
				return new RuntimeValue(do_operation(flx_float(lval->get_i()), flx_float(rval->get_i()), op));
			}
			return new RuntimeValue(do_operation(lval->get_i(), rval->get_i(), op));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}
	else if (lval->is_float()) {
		if (rval->is_numeric() && op == "<=>") {
			return new RuntimeValue(do_spaceship_operation(op, lval, rval));
		}

		if (rval->is_numeric() && Token::is_relational_op(op)) {
			return new RuntimeValue(do_relational_operation(op, lval, rval));
		}

		if (rval->is_numeric() && Token::is_equality_op(op)) {
			flx_float l = lval->get_f();
			flx_float r = rval->is_float() ? rval->get_f() : rval->get_i();

			return new RuntimeValue(flx_bool(op == "==" ? l == r : l != r));
		}

		if (rval->is_float()) {
			return new RuntimeValue(do_operation(lval->get_f(), rval->get_f(), op));
		}
		else if (rval->is_int()) {
			return new RuntimeValue(do_operation(lval->get_f(), flx_float(rval->get_i()), op));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}
	else if (lval->is_char()) {
		if (rval->is_char() && Token::is_equality_op(op)) {
			return new RuntimeValue((flx_bool)(op == "==" ?
				lval->get_c() == rval->get_c() : lval->get_c() != rval->get_c()));
		}

		if (rval->is_char()) {
			return new RuntimeValue(do_operation(std::string{ lval->get_c() }, std::string{ rval->get_c() }, op));
		}
		else if (rval->is_string()) {
			return new RuntimeValue(do_operation(std::string{ lval->get_c() }, rval->get_s(), op));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}
	else if (lval->is_string()) {
		if (rval->is_string() && Token::is_equality_op(op)) {
			return new RuntimeValue((flx_bool)(op == "==" ?
				lval->get_s() == rval->get_s() : lval->get_s() != rval->get_s()));
		}

		if (rval->is_string()) {
			return new RuntimeValue(do_operation(lval->get_s(), rval->get_s(), op));
		}
		else if (rval->is_char()) {
			return new RuntimeValue(do_operation(lval->get_s(), std::string{ rval->get_c() }, op));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}
	else if (lval->is_struct()) {
		if (rval->is_struct() && Token::is_equality_op(op)) {
			return new RuntimeValue((flx_bool)(op == "==" ? equals_value(lval, rval) : !equals_value(lval, rval)));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}
	else if (lval->is_class()) {
		if (rval->is_class() && Token::is_equality_op(op)) {
			return new RuntimeValue((flx_bool)(op == "==" ? equals_value(lval, rval) : !equals_value(lval, rval)));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}
	else if (lval->is_function()) {
		if (rval->is_function() && Token::is_equality_op(op)) {
			return new RuntimeValue((flx_bool)(op == "==" ? equals_value(lval, rval) : !equals_value(lval, rval)));
		}

		ExceptionHelper::throw_operation_err(op, *lval, *rval);

	}

	throw std::runtime_error("cannot determine type of operation");

}

flx_bool RuntimeOperations::do_relational_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
	flx_float l = lval->is_float() ? lval->get_f() : lval->get_i();
	flx_float r = rval->is_float() ? rval->get_f() : rval->get_i();

	if (op == "<") {
		return l < r;
	}
	else if (op == ">") {
		return l > r;
	}
	else if (op == "<=") {
		return l <= r;
	}
	else if (op == ">=") {
		return l >= r;
	}
	ExceptionHelper::throw_operation_err(op, *lval, *rval);
}

flx_int RuntimeOperations::do_spaceship_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
	flx_float l = lval->is_float() ? lval->get_f() : lval->get_i();
	flx_float r = rval->is_float() ? rval->get_f() : rval->get_i();

	auto res = l <=> r;
	if (res == std::strong_ordering::less) {
		return flx_int(-1);
	}
	else if (res == std::strong_ordering::equal) {
		return flx_int(0);
	}
	else if (res == std::strong_ordering::greater) {
		return flx_int(1);
	}
}

flx_int RuntimeOperations::do_operation(flx_int lval, flx_int rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	else if (op == "-=" || op == "-") {
		return lval - rval;
	}
	else if (op == "*=" || op == "*") {
		return lval * rval;
	}
	else if (op == "/=" || op == "/") {
		if (rval == 0) {
			throw std::runtime_error("division by zero encountered");
		}
		return lval / rval;
	}
	else if (op == "%=" || op == "%") {
		if (rval == 0) {
			throw std::runtime_error("remainder by zero is undefined");
		}
		return lval % rval;
	}
	else if (op == "/%=" || op == "/%") {
		if (rval == 0) {
			throw std::runtime_error("floor division by zero encountered");
		}
		return flx_int(std::floor(lval / rval));
	}
	else if (op == "**=" || op == "**") {
		return flx_int(std::pow(lval, rval));
	}
	else if (op == ">>=" || op == ">>") {
		return lval >> rval;
	}
	else if (op == "<<=" || op == "<<") {
		return lval << rval;
	}
	else if (op == "|=" || op == "|") {
		return lval | rval;
	}
	else if (op == "&=" || op == "&") {
		return lval & rval;
	}
	else if (op == "^=" || op == "^") {
		return lval ^ rval;
	}
	throw std::runtime_error("invalid '" + op + "' operator for types 'int' and 'int'");
}

flx_float RuntimeOperations::do_operation(flx_float lval, flx_float rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	else if (op == "-=" || op == "-") {
		return lval - rval;
	}
	else if (op == "*=" || op == "*") {
		return lval * rval;
	}
	else if (op == "/=" || op == "/") {
		if (int(rval) == 0) {
			throw std::runtime_error("division by zero encountered");
		}
		return lval / rval;
	}
	else if (op == "%=" || op == "%") {
		if (int(rval) == 0) {
			throw std::runtime_error("remainder by zero is undefined");
		}
		return std::fmod(lval, rval);
	}
	else if (op == "/%=" || op == "/%") {
		if (int(rval) == 0) {
			throw std::runtime_error("floor division by zero encountered");
		}
		return std::floor(lval / rval);
	}
	else if (op == "**=" || op == "**") {
		return flx_int(std::pow(lval, rval));
	}
	throw std::runtime_error("invalid '" + op + "' operator");
}

flx_string RuntimeOperations::do_operation(flx_string lval, flx_string rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	throw std::runtime_error("invalid '" + op + "' operator for types 'string' and 'string'");
}

flx_array RuntimeOperations::do_operation(flx_array lval, flx_array rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		lval.append(rval);

		return lval;
	}

	throw std::runtime_error("invalid '" + op + "' operator for types 'array' and 'array'");
}

RuntimeValue* RuntimeOperations::normalize_type(std::shared_ptr<TypeDefinition> owner, RuntimeValue* value, bool new_ref) {
	if (owner->is_array() || value->is_array()) {
		return value;
	}

	if (!new_ref) {
		if (owner->is_string() && value->is_char()) {
			value->type = owner->type;
			value->set(flx_string{ value->get_c() });
		}
		else if (owner->is_float() && value->is_int()) {
			value->type = owner->type;
			value->set(flx_float(value->get_i()));
		}
		else if (owner->is_int() && value->is_float()) {
			value->type = owner->type;
			value->set(flx_int(value->get_f()));
		}
	}
	else {
		if (owner->is_string() && value->is_char()) {
			return new RuntimeValue(flx_string{ value->get_c() });
		}
		else if (owner->is_float() && value->is_int()) {
			return new RuntimeValue(flx_float(value->get_i()));
		}
		else if (owner->is_int() && value->is_float()) {
			return new RuntimeValue(flx_int(value->get_f()));
		}
	}

	return value;
}
