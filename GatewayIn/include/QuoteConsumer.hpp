#pragma once

#include <vector>
#include <functional>
#include <thread>
#include <chrono>
#include "QuotesObtainer.hpp"
#include "OrderBook.hpp"


class QuoteConsumer {
public:
	struct QuoteSource {
		gateway::QuotesObtainer* source;
		std::string_view serverId;
	};

	explicit QuoteConsumer(std::vector<QuoteSource> sources)
			: sources_{std::move(sources)} {}

	void run() {
		consumerThread_ = std::thread([this]() {
			while (true) {
				for (auto& src : sources_) {
					processQueue(src.source->getAskQueue());
					processQueue(src.source->getBidQueue());
				}
				std::this_thread::sleep_for(std::chrono::microseconds(100));
			}
		});
	}

	const OrderBook& getOrderBook() const { return orderBook_; }

private:
	std::vector<QuoteSource> sources_;
	std::thread consumerThread_;

	template<typename Queue>
	void processQueue(Queue& queue) {
		gateway::Quote quote;
		while (queue.pop(quote)) {
			orderBook_.update(std::move(quote));
		}
	}

	OrderBook orderBook_;
};