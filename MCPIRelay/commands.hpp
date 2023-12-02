#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace MCPIRelay {
	class Client;

	class CommandParser {
	public:
		void parse_command(MCPIRelay::Client *client, std::vector<std::string>& arguments);
		void help(MCPIRelay::Client *client, std::vector<std::string>& arguments);

		using command_ptr = std::function<void(MCPIRelay::Client *client, std::vector<std::string>& arguments)>;
		std::unordered_map<std::string, command_ptr> commands;

		CommandParser();
		~CommandParser();
	};
}
