#ifndef TYPES_HPP
#define TYPES_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>
#include <variant>

#include "gcobject.hpp"
#include "utils.hpp"
#include "scope.hpp"

namespace core {

	class ASTExprNode;
	class ASTBlockNode;
	class ASTDeclarationNode;
	class ASTFunctionDefinitionNode;

	class Variable;
	class SemanticVariable;
	class RuntimeVariable;

	class RuntimeValue;

	enum class Type {
		T_UNDEFINED, T_VOID, T_BOOL, T_INT, T_FLOAT, T_CHAR, T_STRING, T_OBJECT, T_STRUCT, T_CLASS, T_FUNCTION, T_ANY
	};

	// boolean standardized type
	typedef bool flx_bool;

	// integer standardized type
	typedef int64_t flx_int;

	// floating point standardized type
	typedef long double flx_float;

	// character standardized type
	typedef char flx_char;

	// string standardized type
	typedef std::string flx_string;

	// array standardized type
	class flx_array {
	private:
		flx_int _size;
		std::shared_ptr<RuntimeValue*[]> _data;

	public:
		flx_array();
		flx_array(flx_int size);

		flx_int size() const;

		RuntimeValue*& operator[](flx_int index);
		const RuntimeValue* operator[](flx_int index) const;
		
		void resize(flx_int new_size) {
			if (new_size == _size) return;

			auto new_data = std::shared_ptr<RuntimeValue * []>(new RuntimeValue * [new_size]);
			flx_int min_size = __min(_size, new_size);

			for (flx_int i = 0; i < min_size; ++i) {
				new_data[i] = _data[i];
			}

			_size = new_size;
			_data = new_data;
		}

		void append(flx_array& other) {
			flx_int old_size = _size;
			resize(_size + other.size());

			for (flx_int i = 0; i < other.size(); ++i) {
				_data[old_size + i] = other[i];
			}
		}
	
	};

	// structure standardized type
	typedef std::map<std::string, std::shared_ptr<RuntimeVariable>> flx_struct;

	// function standardized type
	typedef std::pair<std::string, std::string> flx_function;

	// class standardized type
	class flx_class : public Scope {
	public:
		flx_class(std::string module_name_space, std::string module_name);
		flx_class();
	};

	class TypeDefinition {
	public:
		Type type;
		std::string type_name;
		std::string type_name_space;
		std::vector<std::shared_ptr<ASTExprNode>> expr_dim;
		std::vector<size_t> dim;

		TypeDefinition(
			Type type,
			const std::vector<std::shared_ptr<ASTExprNode>>& expr_dim,
			const std::string& type_name_space = "",
			const std::string& type_name = ""
		);

		TypeDefinition(
			Type type,
			const std::vector<size_t>& dim,
			const std::string& type_name_space = "",
			const std::string& type_name = ""
		);

		TypeDefinition(Type type, const std::string& type_name_space, const std::string& type_name);

		TypeDefinition(Type type);

		TypeDefinition();

		virtual ~TypeDefinition() = default;

		bool is_any_or_match_type_def(TypeDefinition rtype, bool strict = false, bool strict_array = false);
		bool match_type_def(TypeDefinition rtype, bool strict, bool strict_array);
		bool match_type_def_bool(TypeDefinition rtype);
		bool match_type_def_int(TypeDefinition rtype, bool strict = false);
		bool match_type_def_float(TypeDefinition rtype, bool strict = false);
		bool match_type_def_char(TypeDefinition rtype);
		bool match_type_def_string(TypeDefinition rtype, bool strict = false);
		bool match_type_def_array(TypeDefinition rtype, bool strict = false, bool strict_array = false);
		bool match_type_def_struct(TypeDefinition rtype);
		bool match_type_def_class(TypeDefinition rtype);
		bool match_type_def_function(TypeDefinition rtype);
		bool match_array_dim(TypeDefinition rtype);

		bool match_type(TypeDefinition rtype) const;
		bool is_undefined() const;
		bool is_void() const;
		bool is_bool() const;
		bool is_int() const;
		bool is_float() const;
		bool is_char() const;
		bool is_string() const;
		bool is_any() const;
		bool is_object() const;
		bool is_class() const;
		bool is_struct() const;
		bool is_function() const;
		bool is_textual() const;
		bool is_numeric() const;
		bool is_array() const;
		bool is_collection() const;
		bool is_iterable() const;

		static std::string type_str(Type);
		static std::string buid_type_str(const TypeDefinition& type);
		static std::string buid_struct_type_name(const std::string& type_name_space, const std::string& type_name);

		void normalize();

	};

	class VariableDefinition : public TypeDefinition {
		using DefaultValueVariant = std::variant<std::monostate, std::shared_ptr<ASTExprNode>, size_t>;
	public:
		std::string identifier;
		DefaultValueVariant default_value;
		bool is_rest = false;
		bool is_const = false;

		explicit VariableDefinition(
			const std::string& identifier,
			TypeDefinition type_definition,
			std::shared_ptr<ASTExprNode> default_value,
			bool is_rest = false,
			bool is_const = false
		);

		explicit VariableDefinition(
			const std::string& identifier,
			TypeDefinition type_definition,
			size_t default_value_pc,
			bool is_rest = false,
			bool is_const = false
		);

		VariableDefinition();

		bool has_expr_default() const {
			return std::holds_alternative<std::shared_ptr<ASTExprNode>>(default_value);
		}

		bool has_pc_default() const {
			return std::holds_alternative<size_t>(default_value);
		}

		std::shared_ptr<ASTExprNode> get_expr_default() const;

		size_t get_pc_default() const;

	};

	class UnpackedVariableDefinition : public TypeDefinition {
	public:
		std::vector<VariableDefinition> variables;
		std::shared_ptr<ASTExprNode> assign_value;

		UnpackedVariableDefinition(TypeDefinition type_definition, const std::vector<VariableDefinition>& variables);
	};

	class FunctionDefinition : public TypeDefinition {
	public:
		std::string identifier;
		std::vector<std::shared_ptr<TypeDefinition>> parameters;
		size_t pointer = 0;
		std::shared_ptr<ASTBlockNode> block = nullptr;

		FunctionDefinition(
			const std::string& identifier,
			TypeDefinition type_definition,
			const std::vector<std::shared_ptr<TypeDefinition>>& parameters = std::vector<std::shared_ptr<TypeDefinition>>(),
			std::shared_ptr<ASTBlockNode> block = nullptr
		);

		FunctionDefinition(const std::string& identifier);

		FunctionDefinition();

		void check_signature() const;
	};

	class StructDefinition {
	public:
		std::string identifier;
		std::map<std::string, std::shared_ptr<VariableDefinition>> variables;

		StructDefinition(
			const std::string& identifier,
			const std::map<std::string, std::shared_ptr<VariableDefinition>>& variables
		);

		StructDefinition(const std::string& identifier);

		StructDefinition();
	};

	class ClassDefinition {
	public:
		std::string identifier;
		std::vector<std::shared_ptr<ASTDeclarationNode>> declarations;
		std::vector<std::shared_ptr<ASTFunctionDefinitionNode>> functions;
		std::map<std::string, VariableDefinition> variables;
		std::shared_ptr<Scope> functions_scope = nullptr;

		ClassDefinition(
			const std::string& identifier,
			const std::vector<std::shared_ptr<ASTDeclarationNode>>& declarations,
			const std::vector<std::shared_ptr<ASTFunctionDefinitionNode>>& functions
		);

		ClassDefinition(const std::string& identifier);

		ClassDefinition();
	};

	class Variable : public TypeDefinition {
	public:
		std::string identifier;

		Variable(const std::string& identifier, TypeDefinition type_definition);

		Variable();

		virtual ~Variable() = default;

		void def_type();

	};

	class Value : public TypeDefinition {
	public:
		std::weak_ptr<Variable> ref;
		bool is_constexpr = false;

		Value(TypeDefinition type, bool is_constexpr = false);

		Value();

		virtual ~Value() = default;
	};

	class SemanticValue : public Value {
	public:
		std::shared_ptr<TypeDefinition> type_ref = nullptr;
		std::shared_ptr<SemanticVariable> ref;
		flx_bool b = false;
		flx_int i = 0;
		flx_float f = 0.;
		flx_char c = '\0';
		flx_string s;
		intmax_t hash = 0;

		SemanticValue(TypeDefinition type_definition, intmax_t hash = 0, bool is_constexpr = false);

		SemanticValue();

		void set_b(flx_bool b);
		void set_i(flx_int i);
		void set_f(flx_float f);
		void set_c(flx_char c);
		void set_s(const flx_string& s);

		void copy_from(const SemanticValue& value);
	};

	class SemanticVariable : public Variable, public std::enable_shared_from_this<SemanticVariable> {
	public:
		std::shared_ptr<SemanticValue> value;
		bool is_const = false;

		SemanticVariable(const std::string& identifier, TypeDefinition type_definition, bool is_const = false);

		SemanticVariable();

		void set_value(std::shared_ptr<SemanticValue> value);
		std::shared_ptr<SemanticValue> get_value();

	};

	class RuntimeValue : public Value, public runtime::GCObject {
	private:
		flx_bool* b = nullptr;
		flx_int* i = nullptr;
		flx_float* f = nullptr;
		flx_char* c = nullptr;
		flx_string* s = nullptr;
		std::shared_ptr<flx_array> arr = nullptr;
		std::shared_ptr<flx_struct> str = nullptr;
		std::shared_ptr<flx_class> cls = nullptr;
		flx_function* fun = nullptr;

	public:
		RuntimeValue* value_ref = nullptr;
		size_t access_index = 0;
		flx_string access_identifier = "";

		RuntimeValue(flx_bool);
		RuntimeValue(flx_int);
		RuntimeValue(flx_float);
		RuntimeValue(flx_char);
		RuntimeValue(flx_string);
		RuntimeValue(flx_array, Type array_type, std::vector<size_t> dim, std::string type_name_space = "", std::string type_name = "");
		RuntimeValue(flx_struct, std::string type_name, std::string type_name_space);
		RuntimeValue(flx_class, std::string type_name, std::string type_name_space);
		RuntimeValue(flx_function);
		RuntimeValue(RuntimeValue*);
		RuntimeValue(TypeDefinition type);
		RuntimeValue(Type type);
		RuntimeValue();
		~RuntimeValue();

		void set(flx_bool);
		void set(flx_int);
		void set(flx_float);
		void set(flx_char);
		void set(flx_string);
		void set(flx_array, Type array_type, std::vector<size_t> dim, std::string type_name_space = "", std::string type_name = "");
		void set(flx_struct, std::string type_name_space, std::string type_name);
		void set(flx_class, std::string type_name_space, std::string type_name);
		void set(flx_function);

		void set_field(std::string identifier, RuntimeValue* sub_value);
		void set_item(size_t index, RuntimeValue* sub_value);
		void set_char(size_t index, RuntimeValue* sub_value);

		flx_bool get_b() const;
		flx_int get_i() const;
		flx_float get_f() const;
		flx_char get_c() const;
		flx_string get_s() const;
		flx_array get_arr() const;
		flx_struct get_str() const;
		flx_class get_cls() const;
		flx_function get_fun() const;

		RuntimeValue* get_field(std::string identifier, bool use_holder_reference = false);
		RuntimeValue* get_item(flx_int index, bool use_holder_reference = false);
		RuntimeValue* get_char(flx_int index, bool use_holder_reference = false);

		flx_bool* get_raw_b();
		flx_int* get_raw_i();
		flx_float* get_raw_f();
		flx_char* get_raw_c();
		flx_string* get_raw_s();
		std::shared_ptr<flx_array> get_raw_arr();
		std::shared_ptr<flx_struct> get_raw_str();
		std::shared_ptr<flx_class> get_raw_cls();
		flx_function* get_raw_fun();

		void set_null();

		bool has_value();

		void copy_from(RuntimeValue* value);

		virtual std::vector<GCObject*> get_references() override;

	private:
		void unset();
	};

	class RuntimeVariable : public Variable, public runtime::GCObject, public std::enable_shared_from_this<RuntimeVariable> {
	public:
		RuntimeValue* value = nullptr;

		RuntimeVariable(const std::string& identifier, TypeDefinition value);

		RuntimeVariable();

		~RuntimeVariable();

		void set_value(RuntimeValue* value);
		RuntimeValue* get_value(bool use_variable_ref = false);

		virtual std::vector<runtime::GCObject*> get_references() override;
	};

	class SemanticOperations {
	public:
		static SemanticValue do_operation(const std::string& op, SemanticValue ltype, SemanticValue rtype);

		static void normalize_type(TypeDefinition owner, SemanticValue* value);

	};

	class RuntimeOperations {
	public:
		static flx_bool equals_value(
			const RuntimeValue* lval,
			const RuntimeValue* rval
		);

		static std::string parse_value_to_string(
			const RuntimeValue* value,
			bool print_complex_types = false,
			std::vector<uintptr_t> printed = std::vector<uintptr_t>()
		);
		static std::string parse_array_to_string(
			const RuntimeValue* value,
			bool print_complex_types,
			std::vector<uintptr_t> printed
		);
		static std::string parse_class_to_string(
			const RuntimeValue* value,
			bool print_complex_types,
			std::vector<uintptr_t> printed
		);
		static std::string parse_struct_to_string(
			const RuntimeValue* value,
			bool print_complex_types,
			std::vector<uintptr_t> printed
		);

		static RuntimeValue* do_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval);
		static flx_bool do_relational_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval);
		static flx_int do_spaceship_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval);
		static flx_int do_operation(flx_int lval, flx_int rval, const std::string& op);
		static flx_float do_operation(flx_float lval, flx_float rval, const std::string& op);
		static flx_string do_operation(flx_string lval, flx_string rval, const std::string& op);
		static flx_array do_operation(flx_array lval, flx_array rval, const std::string& op);

		static RuntimeValue* normalize_type(std::shared_ptr<TypeDefinition> owner, RuntimeValue* value, bool new_ref = false);

	};

}

#endif // !TYPES_HPP
