#include <iostream>
#include <chrono>

#include "QuotesObtainer.hpp"
#include "websocket/BitVavoNetworkClient.hpp"
#include "Visualizer.hpp"

int main() {
	using namespace std::chrono;
	std::cout << "Starting HFT app (demo) ...\n";

	const std::string host = "ws.bitvavo.com";
	const std::string port = "443";
	const std::string market = "BTC-EUR";

	gateway::BitvavoWebSocketClient wsClient;
	gateway::QuotesObtainer<gateway::BitvavoWebSocketClient> obt(wsClient, host, port, market);

	if (!obt.connect()) {
		std::cerr << "Failed to connect to " << host << ":" << port << "\n";
		return 1;
	}

	std::cout << "Connected. Press Ctrl+C to exit.\n";

	demo::Visualizer vis(obt, market);
	vis.run();
}
