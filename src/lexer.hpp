#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "token.hpp"

namespace core {

	namespace parser {

		class Lexer {
		public:
			Lexer(const std::string& name, const std::string& source);
			Lexer(const std::string& name, const std::string& source, size_t start_row, size_t start_col);
			~Lexer();

			Token next_token();
			size_t get_current_token() const;
			void set_current_token(size_t token);

		private:
			std::string source;
			std::string name;
			std::vector<Token> tokens;
			char before_char = '\0';
			char current_char = '\0';
			char next_char = '\0';
			size_t current_index = 0;
			size_t current_token = 0;
			size_t current_row = 1;
			size_t current_col = 0;
			size_t start_col = 0;

			void tokenize();
			bool has_next();
			bool is_space();
			void advance();
			Token process_identifier();
			Token process_special_number();
			Token process_number();
			Token process_char();
			Token process_string();
			void process_multiline_string();
			Token process_symbol();
			Token process_comment();

			std::string build_error_message(const std::string error);

			static size_t find_mlv_closer(const std::string expr);
		};

	}

}

#endif // !LEXER_HPP
