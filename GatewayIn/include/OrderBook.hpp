#pragma once

#include <map>
#include <chrono>
#include <iostream>
#include <Quote.hpp>

class OrderBook {
public:
	using Clock = std::chrono::system_clock;
	using Timestamp = Clock::time_point;

	struct PriceLevelData {
		double size;
		Timestamp lastUpdated;
	};

	void update(gateway::Quote&& quote) {
		const double price = quote.getPrice();
		const double size = quote.getSize();
		const gateway::QuoteSide side = quote.getSide();
		const Timestamp now = Clock::now();

		if (side == gateway::QuoteSide::Bid) {
			PriceLevelData& level = bids_[price];
			level.size += size;
			level.lastUpdated = now;
		} else {
			PriceLevelData& level = asks_[price];
			level.size += size;
			level.lastUpdated = now;
		}
	}

	using BidLevels = std::map<double, PriceLevelData, std::greater<>>;
	using AskLevels = std::map<double, PriceLevelData>;

	const BidLevels&  getBids() const { return bids_; }
	const AskLevels&  getAsks() const { return asks_; }

private:
	BidLevels bids_;
	AskLevels asks_;
};

