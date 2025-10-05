#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <deque>
#include <utility>
#include <chrono>
#include <vector>
#include <memory>
#include <iostream>
#include <thread>
#include <atomic>
#include <algorithm>
#include <functional>
#include <random>
#include <boost/lockfree/spsc_queue.hpp>

#include "Quote.hpp"
#include "../../Parser/include/BitvavoBookParser.hpp"
#include "../../Parser/include/FixBookParser.hpp"
#include <type_traits>

namespace gateway {


	template <typename Client>
	class QuotesObtainer {
	public:
		using Clock = std::chrono::system_clock;

		template <typename C>
		explicit QuotesObtainer(C&& client,
								std::string host,
								std::string port,
								std::string market)
				: host_(std::move(host)),
				  port_(std::move(port)),
				  market_(std::move(market))
			{
				client_ = &client;
				if constexpr (requires(Client* c, const std::string& s) { c->send(s); }) {
					// Bitvavo-like client: parse websocket JSON frames
					client_->setMessageHandler([this](std::string_view msg) { parseBitvavo(msg); });
				} else {
					// Pix-like client: parse FIX messages
					client_->setMessageHandler([this](std::string_view msg) { parseFix(msg); });
				}
				client_->setErrorHandler([this](std::string_view err) {
					std::cerr << "Error: " << err << "\n";
					disconnect();
					startReconnectLoop();
				});
			}

		~QuotesObtainer() { disconnect(); }

		bool connect() {
			if (!client_->connect(host_, port_)) return false;
			if constexpr (requires(Client* c, const std::string& s) { c->send(s); }) {
				// For Bitvavo-like websocket, send manual subscribe
    std::string sub = std::string(R"({"action":"subscribe","channels":[{"name":"book","markets":[")")
						  + market_ + R"("]}]})";
				client_->send(sub);
			}
			return true;
		}

		void disconnect() {
			client_->disconnect();
		}

		// Expose SPSC queues
		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>>& getBidQueue() { return bidQuoteQueue_; }
		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>>& getAskQueue() { return askQuoteQueue_; }

		// Stats/peaks
		std::deque<std::chrono::system_clock::time_point> askTimestamps_;
		std::deque<std::chrono::system_clock::time_point> bidTimestamps_;
		std::optional<Quote> peakAskQuote_;
		std::optional<Quote> peakBidQuote_;

		double avgInterval(const std::deque<std::chrono::system_clock::time_point> timestamps) const {
			if (timestamps.size() < 2) return 0.0;
			double total = 0.0;
			for (size_t i = 1; i < timestamps.size(); ++i) {
				total += std::chrono::duration_cast<std::chrono::milliseconds>(timestamps[i] - timestamps[i - 1]).count();
			}
			return total / (timestamps.size() - 1);
		}
		std::chrono::milliseconds updateInterval_{1000};

		void startReconnectLoop() {
			if (reconnecting_.exchange(true)) return;
			std::thread([this]() {
				std::uniform_int_distribution<int> jitterDist(0, 50);
				size_t attempts = 0;
				while (attempts < maxReconnectAttempts_) {
					std::cerr << "[Reconnect] Attempt " << (attempts + 1)
							  << " for " << host_ << ":" << port_ << "\n";
					if (connect()) {
						std::cout << "[Reconnect] Success for " << host_ << ":" << port_ << "\n";
						reconnecting_ = false;
						return;
					}
					++attempts;
					auto backoff = std::min(baseBackoff_ * (1 << attempts), maxBackoff_);
					backoff += std::chrono::milliseconds(jitterDist(rng_));
					std::this_thread::sleep_for(backoff);
				}
				std::cerr << "[Reconnect] Failed after " << attempts << " attempts for "
						  << host_ << ":" << port_ << "\n";
				reconnecting_ = false;
			}).detach();
		}

		void parseFix(std::string_view fixMessage) {
			auto quote = fix::parseAndStoreQuote(fixMessage);
			if (quote) storeQuote(*quote);
		}

		void parseBitvavo(std::string_view bitVavoMessage) {
			auto result = bitvavo::parseAndStoreQuote(bitVavoMessage, market_);
			if (result) storeQuote(*result);
		}

		void storeQuote(const Quote& quote) {
			if (quote.getSide() == QuoteSide::Bid) {
				if (!bidQuoteQueue_.push(quote)) std::cerr << "Bid queue full for host " << host_ << ":" << port_ << "\n";
				if (!peakBidQuote_ || quote.getPrice() > peakBidQuote_->getPrice()) {
					peakBidQuote_ = quote;
				}
				bidTimestamps_.push_back(quote.getTimestamp());
				if (bidTimestamps_.size() > 100) bidTimestamps_.pop_front();
			} else if (quote.getSide() == QuoteSide::Ask) {
				if (!askQuoteQueue_.push(quote)) std::cerr << "Ask queue full for host " << host_ << ":" << port_ << "\n";
				if (!peakAskQuote_ || quote.getPrice() < peakAskQuote_->getPrice()) {
					peakAskQuote_ = quote;
				}
				askTimestamps_.push_back(quote.getTimestamp());
				if (askTimestamps_.size() > 100) askTimestamps_.pop_front();
			}
		}

	private:
		Client* client_ {nullptr};
		std::string host_;
		std::string port_;
		std::string market_;

		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>> bidQuoteQueue_;
		boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>> askQuoteQueue_;

		std::atomic<bool> reconnecting_{false};
		const size_t maxReconnectAttempts_ = 10;
		const std::chrono::milliseconds baseBackoff_{100};
		const std::chrono::milliseconds maxBackoff_{3000};
		std::mt19937 rng_{std::random_device{}()};
	};
}
