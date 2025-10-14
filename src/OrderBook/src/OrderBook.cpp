#include "OrderBook.hpp"

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
