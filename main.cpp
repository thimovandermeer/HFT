#if 0
#include <iostream>
#include "GatewayIn/include/QuotesObtainer.hpp"
#include "GatewayIn/include/websocket/BitVavoNetworkClient.hpp"
#include <iomanip>
#include <thread>
#include <memory>

int main() {
	using namespace std::chrono;
	std::cout << "Starting QuotesObtainer...\n";

	std::vector<std::pair<std::string, std::string>> servers = {
		{"ws.bitvavo.com", "443"}
	};

	//TODO: this should be moved out of the main function and abstracted away
	std::vector<std::pair<std::unique_ptr<gateway::QuotesObtainer>, std::string>> obtainers;

	for (const auto& [host, port] : servers) {
		gateway::AnyFeed feed;
		bool use_bitvavo = true;
		if (use_bitvavo) feed.emplace<gateway::BitvavoWebSocketClient>();
		else             feed.emplace<gateway::PixNetworkClient>();

		//TODO: fix the last argument later
		auto obtainer = std::make_unique<gateway::QuotesObtainer>(std::move(feed), host, port, "BTC");
		if (!obtainer->connect()) {
			std::cerr << "Failed to connect to " << host << ":" << port << "\n";
			continue;
		} else {
			std::cout << "Successfully connected" << host << ":" << port << std::endl;
		}

		std::string label = host + ":" + port;
		obtainers.emplace_back(std::move(obtainer), std::move(label));
	}

	std::vector<QuoteConsumer::QuoteSource> sources;
	for (auto& [obtainerPtr, label] : obtainers) {
		sources.push_back({ obtainerPtr.get(), label });
	}

	auto consumer = std::make_unique<QuoteConsumer>(std::move(sources));
	std::cout << std::fixed << std::setprecision(5);

	consumer->run();

	// make ui class and add this there
	while (true) {
		std::cout << "\033[2J\033[H";  // Clear screen
		std::cout << "=== Aggregated Level 2 Order Book ===\n\n";

		const auto& book = consumer->getOrderBook();
		const auto& bids = book.getBids();
		const auto& asks = book.getAsks();

		std::cout << std::left << std::setw(15) << "Bid Size" << std::setw(15) << "Bid Price"
				  << "||  "
				  << std::setw(15) << "Ask Price" << std::setw(15) << "Ask Size" << "\n";
		std::cout << std::string(64, '-') << "\n";

		auto bidIt = bids.begin();
		auto askIt = asks.begin();
		for (int i = 0; i < 10; ++i) {
			std::ostringstream line;

			if (bidIt != bids.end()) {
				line << std::setw(15) << bidIt->second.size
					 << std::setw(15) << bidIt->first;
				++bidIt;
			} else {
				line << std::setw(15) << "-" << std::setw(15) << "-";
			}

			line << "||  ";

			if (askIt != asks.end()) {
				line << std::setw(15) << askIt->first
					 << std::setw(15) << askIt->second.size;
				++askIt;
			} else {
				line << std::setw(15) << "-" << std::setw(15) << "-";
			}

			std::cout << line.str() << "\n";
		}

		// === Obtainer Stats ===
		const auto& obtainerStats = consumer->fetchObtainerStats();
		std::cout << "\n=== Quote Queue Stats ===\n\n";
		std::cout << std::left << std::setw(25) << "Server"
				  << std::setw(15) << "Bid Queue"
				  << std::setw(15) << "Ask Queue" << "\n";
		std::cout << std::string(55, '-') << "\n";

		for (const auto& stat : obtainerStats) {
			std::cout << std::setw(25) << stat.serverId
					  << std::setw(15) << stat.bidQueueSize
					  << std::setw(15) << stat.askQueueSize << "\n";
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}
#endif

