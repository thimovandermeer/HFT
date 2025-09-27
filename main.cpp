#include <iostream>
#include <thread>
#include <vector>
#include <utility>
#include <iomanip>
#include <chrono>
#include "GatewayIn/include/QuotesObtainer.hpp"
#include "GatewayIn/include/websocket/BitVavoNetworkClient.hpp"

static void print_header() {
	std::cout << "Time                 | Market   | Latest Bid            | Latest Ask            | Peak Bid             | Peak Ask             | Bids/s | Asks/s\n";
	std::cout << "---------------------+----------+-----------------------+-----------------------+----------------------+----------------------+--------+-------\n";
}

int main() {
	using namespace std::chrono;
	std::cout << "Starting HFT app (demo) ...\n";

	// Configure one Bitvavo endpoint for demo
	const std::string host = "wss.bitvavo.com";
	const std::string port = "443";
	const std::string market = "BTC-EUR";

	gateway::BitvavoWebSocketClient wsClient;
	gateway::QuotesObtainer<gateway::BitvavoWebSocketClient> obt(wsClient, host, port, market);

	if (!obt.connect()) {
		std::cerr << "Failed to connect to " << host << ":" << port << "\n";
		return 1;
	}

	std::cout << "Connected. Press Ctrl+C to exit.\n";
	print_header();

	// Visualization loop
	auto lastPrint = steady_clock::now();
	std::size_t bidsThisInterval = 0;
	std::size_t asksThisInterval = 0;

	gateway::Quote q{};
	std::optional<gateway::Quote> latestBid;
	std::optional<gateway::Quote> latestAsk;

	for (;;) {
		// Drain incoming quotes quickly
		while (obt.getBidQueue().pop(q)) { latestBid = q; ++bidsThisInterval; }
		while (obt.getAskQueue().pop(q)) { latestAsk = q; ++asksThisInterval; }

		// Print every second
		auto now = steady_clock::now();
		if (now - lastPrint >= 1s) {
			// Compose timestamp
			auto sysnow = system_clock::now();
			auto t = system_clock::to_time_t(sysnow);
			std::tm tm{};

			localtime_r(&t, &tm);

			std::ostringstream line;
			line << std::put_time(&tm, "%F %T") << " | "
				 << std::setw(8) << market << " | ";

			auto fmt = [](const std::optional<gateway::Quote>& qq) {
				std::ostringstream os; os.setf(std::ios::fixed); os << std::setprecision(2);
				if (qq) os << std::setw(8) << qq->getPrice() << " x " << std::setw(7) << qq->getSize();
				else    os << std::setw(8) << "-" << " x " << std::setw(7) << "-";
				return os.str();
			};

			line << std::setw(23) << fmt(latestBid) << " | "
				 << std::setw(23) << fmt(latestAsk) << " | ";

			line << std::setw(21);
			if (obt.peakBidQuote_) line << std::fixed << std::setprecision(2) << obt.peakBidQuote_->getPrice(); else line << "-";
			line << " | ";
			line << std::setw(21);
			if (obt.peakAskQuote_) line << std::fixed << std::setprecision(2) << obt.peakAskQuote_->getPrice(); else line << "-";
			line << " | ";

			line << std::setw(6) << bidsThisInterval << " | "
				 << std::setw(5) << asksThisInterval;

			std::cout << line.str() << '\n';

			bidsThisInterval = 0;
			asksThisInterval = 0;
			lastPrint = now;
		}

		std::this_thread::sleep_for(10ms);
	}
}

