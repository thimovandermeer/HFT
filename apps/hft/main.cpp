#include <iostream>
#include <chrono>

#include "QuoteConsumer.hpp"
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

	std::cout << "Connecting to " << host << ":" << port << " ...\n";
	QuoteConsumer consumer{obt, "BTC-EUR"};
	std::cout << "Connected\n";
	OrderBookView view;
	consumer.attachView(&view);
	std::cout << "Attached view\n";
	consumer.setPublishLevels(80);
	consumer.setPublishPeriod(milliseconds(20));

	consumer.start();
	VisualizerImGui visualizer(view);
	std::cout << "Vizualizer class created\n";
	visualizer.run();
	std::cout << "GUI thread running\n";

}
