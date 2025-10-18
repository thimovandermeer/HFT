#pragma once
#include <atomic>
#include <thread>
#include <chrono>
#include <limits>
#include "OrderBook.hpp"
#include "QuotesObtainer.hpp"

template<class GatewayT>
class QuoteConsumer {
public:
    QuoteConsumer(gateway::QuotesObtainer<GatewayT>& obt, std::string symbol)
        : obt_(obt), symbol_(std::move(symbol)), book_(symbol_) {}

    void start() {
        running_.store(true);
        obt_.connect();
        worker_ = std::thread(&QuoteConsumer::runLoop, this);
    }

    void stop() {
        running_.store(false);
        obt_.disconnect();
        if (worker_.joinable()) worker_.join();
    }

    const OrderBook& orderBook() const { return book_; }
    const std::string& symbol() const { return symbol_; }

    void attachView(OrderBookView* v) { view_ = v; }
    void setPublishLevels(std::size_t n) { maxLevels_ = n; }
    void setPublishPeriod(std::chrono::milliseconds p) { publishPeriod_ = p; }

private:
    void runLoop() {
        using namespace std::chrono;
        auto nextPublish = steady_clock::now();
        std::size_t sinceLastPublish = 0;

        double lastBestBid = std::numeric_limits<double>::quiet_NaN();
        double lastBestAsk = std::numeric_limits<double>::quiet_NaN();

        gateway::Quote q{};
        while (running_.load(std::memory_order_relaxed)) {
            bool didWork = false;

            while (obt_.getBidQueue().pop(q)) { book_.update(q); didWork = true; ++sinceLastPublish; }
            while (obt_.getAskQueue().pop(q)) { book_.update(q); didWork = true; ++sinceLastPublish; }

            const auto now = steady_clock::now();
            const double bb = book_.bestBid();
            const double ba = book_.bestAsk();
            const bool tobChanged =
                (!std::isnan(bb) && bb != lastBestBid) ||
                (!std::isnan(ba) && ba != lastBestAsk);

            const bool timeToPublish = now >= nextPublish;
            const bool haveNewData   = sinceLastPublish > 0;

            if (view_ && ((timeToPublish && haveNewData) || tobChanged)) {
                view_->publish_from(book_, maxLevels_);
                sinceLastPublish = 0;
                nextPublish = now + publishPeriod_;
                lastBestBid = bb; lastBestAsk = ba;
            }

            if (!didWork) std::this_thread::sleep_for(100us);
        }
    }

private:
    gateway::QuotesObtainer<GatewayT>& obt_;
    std::string symbol_;
    OrderBook book_;

    std::atomic<bool> running_{false};
    std::thread worker_;

    OrderBookView* view_{nullptr};
    std::size_t maxLevels_{80};
    std::chrono::milliseconds publishPeriod_{20};
};