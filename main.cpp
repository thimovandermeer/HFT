#include <iostream>
#include "GatewayIn/include/QuotesObtainer.hpp"
#include "PixNetworkClient.hpp"
#include "QuoteConsumer.hpp"
#include <iomanip>
#include <thread>
#include <memory>

int main() {
	using namespace std::chrono;
	std::cout << "Starting QuotesObtainer...\n";

	std::vector<std::pair<std::string, std::string>> servers = {
			{"127.0.0.1", "1844"},
			{"127.0.0.1", "1845"},
			{"127.0.0.1", "1846"},
			{"127.0.0.1", "1847"},
			{"127.0.0.1", "1848"},
			{"127.0.0.1", "1849"},
			{"127.0.0.1", "1850"},
			{"127.0.0.1", "1851"},



	};

	std::vector<std::pair<std::unique_ptr<gateway::QuotesObtainer>, std::string>> obtainers;

	for (const auto& [host, port] : servers) {
		auto client = std::make_unique<gateway::PixNetworkClient>();
		client->setConnectTimeout(milliseconds(10000));

		auto obtainer = std::make_unique<gateway::QuotesObtainer>(std::move(client), host, port);
		if (!obtainer->connect()) {
			std::cerr << "❌ Failed to connect to " << host << ":" << port << "\n";
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

