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
#include <random>
#include <boost/lockfree/spsc_queue.hpp>

namespace gateway {
    class PixNetworkClient;
    class QuotesObtainer {
    public:
        explicit QuotesObtainer(
				std::unique_ptr<PixNetworkClient> networkClient,
				std::string host_,
				std::string _port
				);
        ~QuotesObtainer();

        bool connect();
        void disconnect();
        void setUpdateInterval(std::chrono::milliseconds interval);

		size_t sizeAskQueue() const { return askQuoteQueue_.read_available(); }
		size_t sizeBidQueue() const { return bidQuoteQueue_.read_available(); }

		double avgAskQuoteIntervalMs() const { return avgInterval(askTimestamps_); }
		double avgBidQuoteIntervalMs() const { return avgInterval(bidTimestamps_); }

		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>>& getBidQueue();
		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>>& getAskQueue();

	private:
        std::unique_ptr<PixNetworkClient> pixClient_;
		void parseAndStoreQuote(std::string_view fixMessage);

		std::string host_;
		std::string port_;

		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>> bidQuoteQueue_;
		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>> askQuoteQueue_;

		std::deque<std::chrono::system_clock::time_point> askTimestamps_;
		std::deque<std::chrono::system_clock::time_point> bidTimestamps_;

		std::optional<Quote> peakAskQuote_;
		std::optional<Quote> peakBidQuote_;

		double avgInterval(const std::deque<std::chrono::system_clock::time_point> timestamps) const;
		std::chrono::milliseconds updateInterval_{1000};

		// Reconnect logic
		void startReconnectLoop();
		std::atomic<bool> reconnecting_{false};
		const size_t maxReconnectAttempts_ = 10;
		const std::chrono::milliseconds baseBackoff_{100};
		const std::chrono::milliseconds maxBackoff_{3000};
		std::mt19937 rng_{std::random_device{}()};
    };
}
