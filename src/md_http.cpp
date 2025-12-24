#include "md_http.hpp"

#ifdef linux

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#elif defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#endif // linux

#include "vm.hpp"
#include "semantic_analysis.hpp"
#include "utils.hpp"
#include "constants.hpp"

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleHTTP::ModuleHTTP() {}

ModuleHTTP::~ModuleHTTP() = default;

void ModuleHTTP::register_functions(SemanticAnalyser* visitor) {
	visitor->builtin_functions["request"] = nullptr;
}

void ModuleHTTP::register_functions(VirtualMachine* vm) {

	vm->builtin_functions["request"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("req"))->get_value();

		RuntimeValue* config_value = val;
		if (config_value->is_void()) {
			throw std::runtime_error("'req' is null");
		}
		flx_struct config_str = config_value->get_str();
		std::string hostname = config_str["hostname"]->get_value()->get_s();
		std::string path = config_str["path"]->get_value()->get_s();
		std::string method = config_str["method"]->get_value()->get_s();
		std::string port = "80";
		std::string headers = "";
		std::string parameters = "";
		std::string data = config_str["data"]->get_value()->get_s();

		// check mandatory parameters
		if (hostname.empty()) {
			throw std::runtime_error("Hostname must be informed.");
		}
		else if (method.empty()) {
			throw std::runtime_error("Method must be informed.");
		}

		// check if has path
		if (path.empty()) {
			path = "/";
		}

		// get port
		int param_port = config_str["port"]->get_value()->get_i();
		if (param_port != 0) {
			port = std::to_string(param_port);
		}

		// build parameters
		flx_struct str_parameters = config_str["parameters"]->get_value()->get_str();
		for (const auto& parameter : str_parameters) {
			if (parameters.empty()) {
				parameters = "?";
			}
			else {
				parameters += "&";
			}
			parameters += parameter.first + "=" + parameter.second->get_value()->get_s();
		}

		// build headers
		flx_struct str_headers = config_str["headers"]->get_value()->get_str();
		for (const auto& header : str_headers) {
			headers += header.first + ": " + header.second->get_value()->get_s() + "\r\n";
		}

#ifdef linux

		int sock;
		struct addrinfo hints;
		struct addrinfo* result = nullptr;

		// set hints to DNS resolution
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET; // IPv4
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// resolve DNS
		if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result) != 0) {
			throw std::runtime_error("Failed to resolve hostname.");
		}

		// create socket
		sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock < 0) {
			freeaddrinfo(result);
			throw std::runtime_error("Socket creation failed.");
		}

		// connect to server
		if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
			close(sock);
			freeaddrinfo(result);
			throw std::runtime_error("Connection failed.");
		}

#elif defined(_WIN32)

		WSADATA wsa;
		SOCKET sock;
		struct sockaddr_in server;
		struct addrinfo* result = NULL;
		struct addrinfo hints;

		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			throw std::runtime_error("Failed. Error Code: " + WSAGetLastError());
		}

		// set hints to DNS resolution
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET; // IPv4
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// resolve DNS
		if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result) != 0) {
			WSACleanup();
			throw std::runtime_error("Failed to resolve hostname.");
		}

		// create socket
		sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET) {
			freeaddrinfo(result);
			WSACleanup();
			throw std::runtime_error("Socket creation failed. Error: " + WSAGetLastError());
		}

		// connect to server
		if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
			closesocket(sock);
			freeaddrinfo(result);
			WSACleanup();
			throw std::runtime_error("Connection failed. Error: " + WSAGetLastError());
		}

#endif // linux

		// prepare HTTP request
		std::string request = method + " " + path + parameters + " HTTP/1.1\r\n";
		request += "Host: " + hostname + "\r\n";

		// adds headers to request
		if (!headers.empty()) {
			request += headers;
		}
		else {
			request += "\r\n";
		}

		// adds body to request
		if (!data.empty()) {
			request += "Content-Length: " + std::to_string(data.length()) + "\r\n";
			request += "\r\n";
			request += data;
		}
		else {
			request += "\r\n";
		}

		// send HTTP request
		send(sock, request.c_str(), request.length(), 0);

		// receive responce
		char buffer[8192];
		int bytes_received = recv(sock, buffer, sizeof(buffer), 0);

#ifdef linux

		close(sock);

#elif defined(_WIN32)

		closesocket(sock);
		freeaddrinfo(result);
		WSACleanup();

#endif // linux

		std::string raw_response(buffer, bytes_received);
		std::vector<std::string> response_lines = utils::StringUtils::split(raw_response, "\r\n");
		bool is_body = false;
		std::string res_body;

		// dictionary temporary vector
		std::vector<std::pair<flx_string, flx_string>> res_headers;

		for (size_t i = 1; i < response_lines.size(); ++i) {
			auto& line = response_lines[i];
			if (is_body) {
				res_body += line;
			}
			else {
				if (line == "") {
					is_body = true;
					continue;
				}

				auto header = utils::StringUtils::split(line, ": ");
				res_headers.push_back(std::make_pair(header[0], header[1]));

			}
		}

		auto flx_arr = flx_array(res_headers.size());
		for (size_t i = 0; i < res_headers.size(); ++i) {
			const auto& header = res_headers[i];
			// create header struct
			auto key_var = std::make_shared<RuntimeVariable>("key", Type::T_STRING);
			key_var->set_value(vm->allocate_value(new RuntimeValue(flx_string(header.first))));
			vm->gc.add_var_root(key_var);

			auto value_var = std::make_shared<RuntimeVariable>("value", Type::T_STRING);
			value_var->set_value(vm->allocate_value(new RuntimeValue(flx_string(header.second))));
			vm->gc.add_var_root(value_var);

			flx_struct header_str;
			header_str["key"] = key_var;
			header_str["value"] = value_var;

			// create header value
			auto header_value = vm->allocate_value(new RuntimeValue(
				header_str,
				Constants::DEFAULT_NAMESPACE,
				Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_ENTRY]
			));

			// push header to headers array
			flx_arr[i] = header_value;
		}
		auto headers_value = vm->allocate_value(new RuntimeValue(
			flx_arr,
			Type::T_STRUCT,
			std::vector<size_t>{res_headers.size()},
			Constants::DEFAULT_NAMESPACE,
			Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_ENTRY]
		));

		// create response struct
		auto status = utils::StringUtils::split(response_lines[0], ' ');

		auto http_version_var = std::make_shared<RuntimeVariable>("http_version", Type::T_STRING);
		http_version_var->set_value(vm->allocate_value(new RuntimeValue(flx_string(status[0]))));
		vm->gc.add_var_root(http_version_var);

		auto status_var = std::make_shared<RuntimeVariable>("status", Type::T_INT);
		status_var->set_value(vm->allocate_value(new RuntimeValue(flx_int(stoll(status[1])))));
		vm->gc.add_var_root(status_var);

		auto status_description_var = std::make_shared<RuntimeVariable>("status_description", Type::T_STRING);
		status_description_var->set_value(vm->allocate_value(new RuntimeValue(flx_string(status[2]))));
		vm->gc.add_var_root(status_description_var);

		auto headers_var = std::make_shared<RuntimeVariable>("headers",
			TypeDefinition(Type::T_STRUCT,
				std::vector<size_t>{res_headers.size()},
				Constants::DEFAULT_NAMESPACE,
				Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_ENTRY]
			)
		);
		headers_var->set_value(headers_value);
		vm->gc.add_var_root(headers_var);

		auto data_var = std::make_shared<RuntimeVariable>("data", Type::T_STRING);
		data_var->set_value(vm->allocate_value(new RuntimeValue(flx_string(res_body))));
		vm->gc.add_var_root(data_var);

		auto raw_var = std::make_shared<RuntimeVariable>("raw", Type::T_STRING);
		raw_var->set_value(vm->allocate_value(new RuntimeValue(flx_string(raw_response))));
		vm->gc.add_var_root(raw_var);

		flx_struct res_str;
		res_str["http_version"] = http_version_var;
		res_str["status"] = status_var;
		res_str["status_description"] = status_description_var;
		res_str["headers"] = headers_var;
		res_str["data"] = data_var;
		res_str["raw"] = raw_var;

		vm->push_new_constant(new RuntimeValue(res_str, Constants::STD_NAMESPACE, "HttpResponse"));

		};

}
