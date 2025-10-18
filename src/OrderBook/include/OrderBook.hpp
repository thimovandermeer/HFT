#pragma once

#include <map>
#include <string>
#include <functional>
#include "Quote.hpp"

struct PriceLevel {
	double price{};
	double size{};
};

struct OrderBookSnapshot {
	std::string symbol;
	double bestBid{std::numeric_limits<double>::quiet_NaN()};
	double bestAsk{std::numeric_limits<double>::quiet_NaN()};

	std::vector<std::pair<double,double>> bidLevels;
	std::vector<std::pair<double,double>> askLevels;

	std::chrono::steady_clock::time_point mono_ts{};
};

struct alignas(64) ActiveIndex {
	std::atomic<int> v{0};
	char _pad[64 - sizeof(std::atomic<int>)]{};
};

class OrderBookView {
public:
    template<class OrderBookT>
    void publish_from(const OrderBookT& ob, std::size_t maxLevels = 0) {
        const int cur  = active_.v.load(std::memory_order_relaxed);
        const int next = 1 - cur;

        OrderBookSnapshot s;
        s.symbol = ob.symbol();
        s.mono_ts = std::chrono::steady_clock::now();

        if (!ob.bids().empty()) s.bestBid = ob.bids().begin()->second.price;
        if (!ob.asks().empty()) s.bestAsk = ob.asks().begin()->second.price;

        s.bidLevels.reserve(maxLevels ? maxLevels : ob.bids().size());
        s.askLevels.reserve(maxLevels ? maxLevels : ob.asks().size());

        {
            std::size_t n = 0;
            for (auto& [p, lvl] : ob.bids()) {
                s.bidLevels.emplace_back(p, lvl.size);
                if (maxLevels && ++n >= maxLevels) break;
            }
        }
        {
            std::size_t n = 0;
            for (auto& [p, lvl] : ob.asks()) {
                s.askLevels.emplace_back(p, lvl.size);
                if (maxLevels && ++n >= maxLevels) break;
            }
        }

        buffers_[next] = std::move(s);
        active_.v.store(next, std::memory_order_release);
    }

    OrderBookSnapshot read() const {
        const int idx = active_.v.load(std::memory_order_acquire);

        return buffers_[idx];
    }

private:
    OrderBookSnapshot buffers_[2];

    ActiveIndex active_{};
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

	OrderBookSnapshot snapshot(std::size_t maxLevels = 0) const;

private:
	std::string symbol_;

	std::map<double, PriceLevel, std::greater<double>> bids_;
	std::map<double, PriceLevel, std::less<double>> asks_;
};