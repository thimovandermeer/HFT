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

	auto pixClient = std::make_unique<gateway::PixNetworkClient>();
	pixClient->setConnectTimeout(milliseconds(10000));

	gateway::QuotesObtainer quotes(std::move(pixClient), "127.0.0.1", "1844");
	if (!quotes.connect()) {
		std::cerr << "❌ Failed to connect to PIX server.\n";
		return 1;
	}

	std::cout << "✅ Connected to PIX server.\n";
	std::cout << std::fixed << std::setprecision(5);

	while (true) {
		std::cout << "\033[2J\033[H";  // Clear screen

		std::cout << "=== Market Quote Monitor ===\n";

		// Queue sizes
		auto askSize = quotes.sizeAskQueue();
		auto bidSize = quotes.sizeBidQueue();

		// Most recent quotes
		auto latestAsk = quotes.peekAskQuote();
		auto latestBid = quotes.peekBidQuote();

		std::cout << "\n--- Ask Queue ---\n";
		std::cout << "Size: " << askSize << "\n";
		if (latestAsk.has_value()) {
			std::cout << "Latest: " << latestAsk->getPrice() << " @ "
					  << duration_cast<milliseconds>(
							  latestAsk->getTimestamp().time_since_epoch()).count()
					  << " ms\n";
		}

		std::cout << "\n--- Bid Queue ---\n";
		std::cout << "Size: " << bidSize << "\n";
		if (latestBid.has_value()) {
			std::cout << "Latest: " << latestBid->getPrice() << " @ "
					  << duration_cast<milliseconds>(
							  latestBid->getTimestamp().time_since_epoch()).count()
					  << " ms\n";
		}

		std::cout << "\n--- Metrics ---\n";
		std::cout << "Avg Ask Quote Interval: " << quotes.avgAskQuoteIntervalMs() << " ms\n";
		std::cout << "Avg Bid Quote Interval: " << quotes.avgBidQuoteIntervalMs() << " ms\n";

		std::this_thread::sleep_for(milliseconds(500));
	}
}

