#include "OrderBook.hpp"
#include <iostream>

using namespace std;


void OrderBook::update(const gateway::Quote& quote) {
	const double price = quote.getPrice();
	const double size  = quote.getSize();

	if (quote.getSide() == gateway::QuoteSide::Bid) {
		if (size == 0.0)
			bids_.erase(price);
		else
			bids_[price] = PriceLevel{price, size};
	} else {
		if (size == 0.0)
			asks_.erase(price);
		else
			asks_[price] = PriceLevel{price, size};
	}
}
double OrderBook::bestBid() const {
	return bids_.empty() ? 0.0 : bids_.begin()->first;
}

double OrderBook::bestAsk() const {
	return asks_.empty() ? 0.0 : asks_.begin()->first;
}

static inline double nanq() {
	return std::numeric_limits<double>::quiet_NaN();
}


OrderBookSnapshot OrderBook::snapshot(std::size_t maxLevels) const {

	OrderBookSnapshot s;
	s.symbol = symbol_;
	s.mono_ts = std::chrono::steady_clock::now();

	if (!bids_.empty()) s.bestBid = bids_.begin()->second.price; else s.bestBid = nanq();
	if (!asks_.empty()) s.bestAsk = asks_.begin()->second.price; else s.bestAsk = nanq();

	s.bidLevels.reserve(maxLevels ? maxLevels : bids_.size());
	s.askLevels.reserve(maxLevels ? maxLevels : asks_.size());

	{
		std::size_t n = 0;
		for (const auto& [price, lvl] : bids_) {
			s.bidLevels.emplace_back(price, lvl.size);
			if (maxLevels && ++n >= maxLevels) break;
		}
	}

	{
		std::size_t n = 0;
		for (const auto& [price, lvl] : asks_) {
			s.askLevels.emplace_back(price, lvl.size);
			if (maxLevels && ++n >= maxLevels) break;
		}
	}

	return s;
}
