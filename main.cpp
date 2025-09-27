#include <iostream>
#include <thread>
#include <vector>
#include <utility>
#include <iomanip>
#include <chrono>
#include "GatewayIn/include/QuotesObtainer.hpp"
#include "Vizualizer/Visualizer.hpp"

static void print_header() {
	std::cout << "Time                 | Market   | Latest Bid            | Latest Ask            | Peak Bid             | Peak Ask             | Bids/s | Asks/s\n";
	std::cout << "---------------------+----------+-----------------------+-----------------------+----------------------+----------------------+--------+-------\n";
}

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
