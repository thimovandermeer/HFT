#include <iostream>
#include "GatewayIn/include/QuotesObtainer.hpp"
#include "PixNetworkClient.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
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
		}

		std::string label = host + ":" + port;
		obtainers.emplace_back(std::move(obtainer), std::move(label));
	}

	std::cout << "✅ Connected to PIX server.\n";
	std::cout << std::fixed << std::setprecision(5);

	while (true) {
		std::cout << "\033[2J\033[H";  // Clear screen
		std::cout << "=== Market Quote Monitor ===\n";

		for (const auto& [quotesPtr, label] : obtainers) {
			const auto& quotes = *quotesPtr;
			std::cout << "\n--- Feed: " << label << " ---\n";

			auto askSize = quotes.sizeAskQueue();
			auto bidSize = quotes.sizeBidQueue();
			auto latestAsk = quotes.peekAskQuote();
			auto latestBid = quotes.peekBidQuote();

			std::cout << "Ask Queue: " << askSize;
			if (latestAsk) std::cout << ", Latest Ask: " << latestAsk->getPrice();
			std::cout << "\n";

			std::cout << "Bid Queue: " << bidSize;
			if (latestBid) std::cout << ", Latest Bid: " << latestBid->getPrice();
			std::cout << "\n";

			std::cout << "Avg Ask Interval: " << quotes.avgAskQuoteIntervalMs() << " ms\n";
			std::cout << "Avg Bid Interval: " << quotes.avgBidQuoteIntervalMs() << " ms\n";
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

