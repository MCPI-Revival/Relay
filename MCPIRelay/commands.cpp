#include <commands.hpp>
#include <client.hpp>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include <RakSleep.h>

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

using MCPIRelay::CommandParser;

void reload_chunks(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		client->post_to_chat("Reloading chunks. Please wait.");
		client->reload_chunks();
	} else {
		client->post_to_chat("Help for /reloadchunks");
		client->post_to_chat("Alias: /rc");
		client->post_to_chat("Usage: /reloadchunks");
		client->post_to_chat("Requests all chunks from the server at once.");
		client->post_to_chat("Useful for bad connections or fixing the builder's glitch.");
		client->post_to_chat("May lag your game. If that happens, just wait a bit.");
	}
}

void global_chat(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		std::string msg = "(" + client->username + ")";
		for(int i = 1; i < arguments.size(); i++) {
			msg += " " + arguments[i];
		}

		client->relay->post_to_chat(msg);
		client->log() << "global " << msg << std::endl;
	} else {
		client->post_to_chat("Help for /global");
		client->post_to_chat("Alias: /g");
		client->post_to_chat("Usage: /global <message>");
		client->post_to_chat("Sends a message to the global chat.");
		client->post_to_chat("Sent for all clients connected to the relay.");
	}
}

void test(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		std::string msg = "<" + client->username + ">";
		for(int i = 1; i < arguments.size(); i++) {
			msg += " " + arguments[i];
		}
		client->post_to_chat("\n" + msg);
		client->post_to_chat(msg);
	} else {
		client->post_to_chat("Help for /test");
		client->post_to_chat("Usage: /test <message>");
		client->post_to_chat("Chat shenanigans.");
	}
}

void connect_to(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		if (client->rank < 3) {
			client->post_to_chat("Permission error, access denied.");
			return;
		}
		std::string host = "localhost";
		uint16_t port = 19132;
		if(std::atoi(arguments[1].c_str()) == 0) {
			host = arguments[1];
		} else {
			port = stoi(arguments[1]);
		}

		if(arguments.size() >= 3) {
			if(std::atoi(arguments[2].c_str()) == 0) {
				host = arguments[2];
			} else {
				port = stoi(arguments[2]);
			}
		}
		client->post_to_chat("Connecting to " + host + ":" + std::to_string(port));

		RakNet::RakPeerInterface *targetPeer = RakNet::RakPeerInterface::GetInstance();
		RakNet::SocketDescriptor tsd(0, 0);
		targetPeer->Startup(1, &tsd, 1);
		targetPeer->SetMaximumIncomingConnections(1);
		targetPeer->Connect(host.c_str(), port, nullptr, 0);

		bool awaiting_answer = true;
		RakNet::RakNetGUID targetGuid;
		RakNet::Packet *temp_packet;
		while (awaiting_answer) {
			temp_packet = targetPeer->Receive();
			if (!temp_packet) continue;
			uint8_t temp_packet_id;
			targetGuid = temp_packet->guid;
			RakNet::BitStream temp_receive_stream(temp_packet->data, (int)temp_packet->bitSize, false);
			temp_receive_stream.Read<uint8_t>(temp_packet_id);
			if (temp_packet_id == ID_CONNECTION_REQUEST_ACCEPTED) {
				awaiting_answer = false;
			} else {
				//std::cout << (int)packet_ida << "\n";
				if ((int)temp_packet_id == ID_CONNECTION_ATTEMPT_FAILED) {
					awaiting_answer = false;
					client->post_to_chat("Failed to connect to server");
					return;
				}
			}
			RakSleep(1);
		}

		client->post_to_chat("Connected to new server");
		client->switch_connection(targetPeer, targetGuid);
	} else {
		client->post_to_chat("Help for /connect");
		client->post_to_chat("Alias: /c");
		client->post_to_chat("Usage: /connect <host> <port>");
		client->post_to_chat("Connects the client to a host:port pair.");
		client->post_to_chat("Warning! Use only if you know what you are doing. This is a debug command.");
	}
}

void connect_to_server(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		if (client->rank < 3) {
			client->post_to_chat("Permission error, access denied.");
			return;
		}
		if(client->relay->servers.count(arguments[1]) > 0) {
			client->relay->servers[arguments[1]]->connect_client(client);
		} else {
			client->post_to_chat("Server not found!");
		}
	} else {
		client->post_to_chat("Help for /goto");
		client->post_to_chat("Usage: /goto <server name>");
		client->post_to_chat("Connects the client to a server connected to the relay.");
		client->post_to_chat("Warning! Use only if you know what you are doing. This is a debug command.");
	}
}

void debug(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		client->debug = !client->debug;
	} else {
		client->post_to_chat("Help for /debug");
		client->post_to_chat("Usage: /debug");
		client->post_to_chat("Toggles debug.");
		client->post_to_chat("This is pretty useless if you aren't NikZapp.");
	}
}

void godmode(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		if (client->rank < 3) {
			client->post_to_chat("Permission error, access denied.");
			return;
		}
		client->god_mode = !client->god_mode;
	} else {
		client->post_to_chat("Help for /godmode");
		client->post_to_chat("Usage: /godmode");
		client->post_to_chat("Removes damage, but does not work for some reason.");
	}
}

void list_players(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments[0] != "/help") {
		client->post_to_chat("Player list:");
		std::string line;
		for(const auto& pair : client->relay->clients) {
			if(line.length() + pair.second->username.length() + 1 >= 50) {
				line += ",";
				client->post_to_chat(line);
				line = "";
			} else if(line != "") {
				line += ", ";
			}
			line += pair.second->username;
		}
		client->post_to_chat(line);
	} else {
		client->post_to_chat("Help for /list");
		client->post_to_chat("Usage: /list");
		client->post_to_chat("Shows the list of online players.");
	}
}

void help_stub(MCPIRelay::Client *client, std::vector<std::string>& arguments) {};

CommandParser::CommandParser() {
	commands["/help"] = &help_stub;
	commands["/rc"] = &reload_chunks;
	commands["/reloadchunks"] = &reload_chunks;
	commands["/global"] = &global_chat;
	commands["/g"] = &global_chat;
	commands["/connect"] = &connect_to;
	commands["/c"] = &connect_to;
	commands["/test"] = &test;
	commands["/goto"] = &connect_to_server;
	commands["/debug"] = &debug;
	commands["/godmode"] = &godmode;
	commands["/list"] = &list_players;
}

CommandParser::~CommandParser() {
	return;
}

void CommandParser::parse_command(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	std::cout << "[COMMAND PARSER]: " << arguments[0] << " from " << client->username << "\n";
	if(arguments[0] == "/help") {
		help(client, arguments);
	} else if(commands.count(arguments[0]) > 0) {
		try {
			commands[arguments[0]](client, arguments);
		} catch(const std::exception& e) {
			std::cout << e.what() << "\n";
			client->post_to_chat("Error executing command: " + *e.what());
		}
	} else {
		client->post_to_chat("Unknown command. See /help for a list of commands.");
	}
}

void CommandParser::help(MCPIRelay::Client *client, std::vector<std::string>& arguments) {
	if(arguments.size() > 1) {
		if(arguments[1][0] != '/') arguments[1] = "/" + arguments[1];
		if(commands.count(arguments[1]) > 0) {
			commands[arguments[1]](client, arguments);
		} else {
			client->post_to_chat("Unknown command. See /help for a list of commands.");
		}
	} else {
		client->post_to_chat("Command list:");
		std::string line;
		for(const auto& pair : commands) {
			if(line.length() + pair.first.length() + 1 >= 50) {
				line += ",";
				client->post_to_chat(line);
				line = "";
			} else if(line != "") {
				line += ", ";
			}
			line += pair.first;
		}
		client->post_to_chat(line);
	}
}
