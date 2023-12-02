#include <config.hpp>
#include <relay.hpp>

#include <iostream>

//using RoadRunner::Server;
using MCPIRelay::Relay;

int main(int argc, char* argv[]) {
	bool server = false;
	for (int i = 0; i < argc; i++) {
		std::string arg = argv[i];
		if(arg == "-s") {
			server = true;
		}
	}

	if(server) {
		//Server *server = new Server(SERVER_PORT, MAX_CLIENTS);
	} else {
		Relay *relay = new Relay(19132, 18132, 100);
		delete relay; // Destroy when stopped
	}
    return 0;
}
