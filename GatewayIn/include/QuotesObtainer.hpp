#pragma once

#include <string_view>
#include <optional>
#include <chrono>
#include <vector>
#include <memory>
#include <optional>
#include <format>
#include <Quote.hpp>
#include <NetworkClientBase.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace gateway {
    class PixNetworkClient;
    class QuotesObtainer {
    public:
        explicit QuotesObtainer(std::unique_ptr<PixNetworkClient> networkClient);
        ~QuotesObtainer();

        bool connect(std::string_view host, std::string_view port);
        void disconnect();
        void setUpdateInterval(std::chrono::milliseconds interval);
		std::optional<Quote> getBidCurrentQuote();
		std::optional<Quote> getAskCurrentQuote();

	private:
        std::unique_ptr<PixNetworkClient> pixClient_;
		void parseAndStoreQuote(std::string_view fixMessage);

		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>> bidQuoteQueue_;
		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>> askQuoteQueue_;
		std::chrono::milliseconds updateInterval_{1000};
    };
}
