#include <iostream>
#include "GatewayIn/include/QuotesObtainer.hpp"
#include "PixNetworkClient.hpp"

int main()
{
	std::cout << "Hello, World!" << std::endl;
	auto pixClient = std::make_unique<gateway::PixNetworkClient>();
	pixClient->setConnectTimeout(std::chrono::milliseconds(10000));
	auto quotes = gateway::QuotesObtainer(std::move(pixClient));
	if (quotes.connect("127.0.0.1", "1844")) {
		std::cout << "Successfully connected to PIX server\n";
		while (true) {
			auto askQuote = quotes.getAskCurrentQuote();
			if (askQuote.has_value()) {
				std::cout << "Best quote: " << askQuote->getPrice() << "\n";
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

	}

}
