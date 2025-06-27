//
// Created by Thimo Van der meer on 27/06/2025.
//

#include "../include/QuotesObtainer.hpp"
#include "../include/Quote.hpp"
#include "../include/PixNetworkClient.hpp"
namespace gateway
{
	QuotesObtainer::QuotesObtainer(std::unique_ptr<PixNetworkClient> networkClient)
			: pixClient_(std::move(networkClient))
	{
		pixClient_->setMessageHandler([this](std::string_view message) {
			this->processPixMessage(message);
		});

		pixClient_->setErrorHandler([](std::string_view error) {
			std::cerr << "PIX Error: " << error << std::endl;
		});
	}

	QuotesObtainer::~QuotesObtainer()
	{
		disconnect();
	}

	bool QuotesObtainer::connect(std::string_view host, std::string_view port)
	{
		return pixClient_->connect(host, port);
	}

	void QuotesObtainer::disconnect()
	{
		if (pixClient_) {
			pixClient_->disconnect();
		}
	}

	std::optional<Quote> QuotesObtainer::getCurrentQuote() const
	{
		std::lock_guard<std::mutex> lock(quoteMutex_);
		return lastQuote_;
	}

	void QuotesObtainer::setUpdateInterval(std::chrono::milliseconds interval)
	{
		updateInterval_ = interval;
	}

	void QuotesObtainer::processPixMessage(std::string_view message)
	{
		auto quotes = parsePixFormat(message);
		if (!quotes.empty()) {
			std::lock_guard<std::mutex> lock(quoteMutex_);
			lastQuote_ = quotes.front();
		}
	}

	std::vector<Quote> QuotesObtainer::parsePixFormat(std::string_view data) const
	{
		std::vector<Quote> quotes;
		std::istringstream stream{std::string(data)};
		std::string line;

		while (std::getline(stream, line)) {
			// Expected format: SYMBOL,PRICE,TIMESTAMP
			std::istringstream lineStream(line);
			std::string symbol, priceStr, timestampStr;

			if (std::getline(lineStream, symbol, ',') &&
				std::getline(lineStream, priceStr, ',') &&
				std::getline(lineStream, timestampStr)) {

				try {
					double price = std::stod(priceStr);
					auto timestamp = std::chrono::system_clock::from_time_t(
							std::stoull(timestampStr)
					);

					quotes.emplace_back(price, timestamp, symbol);
				} catch (const std::exception &e) {
					std::cerr << "Failed to parse quote: " << e.what() << std::endl;
					continue;
				}
			}
		}

		return quotes;
	}
	}
