#pragma once

#include <map>
#include <string>
#include <functional>
#include "Quote.hpp"

struct PriceLevel {
	double price{};
	double size{};
};

class OrderBook {
public:
	explicit OrderBook(std::string symbol)
		: symbol_(std::move(symbol)) {}

	void update(const gateway::Quote& quote);

	double bestBid() const;
	double bestAsk() const;

	const std::map<double, PriceLevel, std::greater<double>>& bids() const { return bids_; }
	const std::map<double, PriceLevel, std::less<double>>& asks() const { return asks_; }

	const std::string& symbol() const { return symbol_; }

private:
	std::string symbol_;

	std::map<double, PriceLevel, std::greater<double>> bids_;
	std::map<double, PriceLevel, std::less<double>> asks_;
};