//
// Created by Thimo Van der meer on 27/06/2025.
//

#include "../include/QuotesObtainer.hpp"
#include "../include/PixNetworkClient.hpp"
namespace gateway
{
	QuotesObtainer::QuotesObtainer(std::unique_ptr<PixNetworkClient> networkClient)
			: pixClient_(std::move(networkClient))
	{
		pixClient_->setMessageHandler([this](std::string_view message) {
			this->parseAndStoreQuote(message);
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
		if(pixClient_->connect(host, port)) {
			std::cout << "We are connected and trying to login and subscribe" << std::endl;
			pixClient_->loginAndSubscribe("EUR/USD");
			return true;
		} else {
			return false;
		}
	}

	void QuotesObtainer::disconnect()
	{
		if (pixClient_) {
			pixClient_->disconnect();
		}
	}

	void QuotesObtainer::setUpdateInterval(std::chrono::milliseconds interval)
	{
		updateInterval_ = interval;
	}

	inline void QuotesObtainer::parseAndStoreQuote(std::string_view fixMessage) {
		std::unordered_map<std::string_view, std::string_view> fields;
		size_t pos = 0;
		std::cout << "FIX MESSAGE " << fixMessage << std::endl;
		while (pos < fixMessage.size()) {
			size_t eq = fixMessage.find('=', pos);
			if (eq == std::string_view::npos) break;

			size_t soh = fixMessage.find('\x01', eq);
			if (soh == std::string_view::npos) break;

			auto tag = fixMessage.substr(pos, eq - pos);
			auto value = fixMessage.substr(eq + 1, soh - eq - 1);
			fields[tag] = value;

			pos = soh + 1;
		}

		std::string_view symbol = fields["55"];
		std::string_view reqID = fields["262"];
		for (const auto& elem : fields) {
			std::cout << "key: " << elem.first << " value: " << elem.second  << '\n';
		}
		int numEntries = std::stoi(std::string(fields["268"]));
		size_t current = fixMessage.find("268=");

		for (int i = 0; i < numEntries; ++i) {
			size_t entryStart = fixMessage.find("269=", current);
			if (entryStart == std::string_view::npos) break;

			std::string_view side = fixMessage.substr(entryStart + 4, 1); // 0=bid, 1=ask

			size_t pxPos = fixMessage.find("270=", entryStart);
			size_t pxEnd = fixMessage.find('\x01', pxPos);
			double price = std::stod(std::string(fixMessage.substr(pxPos + 4, pxEnd - pxPos - 4)));

			Quote quote(price, std::chrono::system_clock::now(), symbol, QuoteSide::Bid);

			if (side == "0") {
				if (!bidQuoteQueue_.push(quote)) std::cerr << "Bid queue full\n";
			} else if (side == "1") {
				if (!askQuoteQueue_.push(quote)) std::cerr << "Ask queue full\n";
			}

			current = pxEnd;
		}
	}

	std::optional<Quote> QuotesObtainer::getBidCurrentQuote() {
		Quote quote;
		if (bidQuoteQueue_.pop(quote)) {
			return quote;
		}
		return std::nullopt;
	}

	std::optional<Quote> QuotesObtainer::getAskCurrentQuote() {
		Quote quote;
		if (askQuoteQueue_.pop(quote)) {
			return quote;
		}
		return std::nullopt;
	}

}
