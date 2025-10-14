#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <string_view>
#include "QuotesObtainer.hpp"
#include "OrderBook.hpp"


struct ObtainerStats {
	std::string_view serverId;
	size_t askQueueSize;
	size_t bidQueueSize;
};

template<typename... ObtainerTs>
class QuoteConsumer {
public:

	explicit QuoteConsumer(std::tuple<ObtainerTs&...> feeds, std::string market):
		orderBook_(std::move(market))
	{
		std::apply([this](auto&... feed) {
			(this->sources_.push_back(this->makeSource(feed)), ...);
		}, feeds);
	}

	~QuoteConsumer(){stop();};

	void start() {
		if (running_.exchange(true)) return;
		consumerThread_ = std::thread(&QuoteConsumer::run, this);
	}

	void stop() {
		if (!running_.exchange(false)) return;
		if (consumerThread_.joinable())
			consumerThread_.join();
	}

	std::vector<ObtainerStats> fetchObtainerStats() const;
	const OrderBook& getOrderBook() const { return orderBook_; }

private:
	void run() {
		using namespace std::chrono_literals;
		while (running_) {
			for (auto& src : sources_) {
				src.process();
			}
			std::this_thread::sleep_for(100us);
		}
	}

	template<typename Queue>
	void processQueue(Queue& queue) {
		gateway::Quote q;
		while (queue.pop(q)) {
			orderBook_.update(q);
		}
	}


	struct SourceAdapter {
		std::function<void()> process;
		std::string_view id;
		std::string_view market;
	};

	template<typename T>
	SourceAdapter makeSource(T& feed)
	{
		return SourceAdapter{
			[&feed, this]() {
				processQueue(feed.getAskQueue());
				processQueue(feed.getBidQueue());
			},
			feed.getHost(),
			feed.getMarket()
		};
	}

	std::vector<SourceAdapter> sources_;
	mutable std::thread consumerThread_;
	std::atomic<bool> running_{false};

	OrderBook orderBook_;
};