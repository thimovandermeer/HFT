#include <iostream>
#include "GatewayIn/include/QuotesObtainer.hpp"
#include "PixNetworkClient.hpp"

int main()
{
	std::cout << "Hello, World!" << std::endl;
	auto pixClient = std::make_unique<gateway::PixNetworkClient>();
	auto quotes = gateway::QuotesObtainer(std::move(pixClient));

	if (quotes.connect("localhost", "12345")) {
		std::cout << "Successfully connected to PIX server\n";

		// Check for quotes periodically
		for (int i = 0; i < 10; ++i) {
			if (auto quote = quotes.getCurrentQuote()) {
				std::cout << "Current quote: "
						  << quote->getSymbol()
						  << " "
						  << quote->getPrice()
						  << '\n';
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

}
