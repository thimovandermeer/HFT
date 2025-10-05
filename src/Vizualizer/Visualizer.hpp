#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>
#include <optional>
#include <thread>
#include <chrono>
#include "src/GatewayIn/include/QuotesObtainer.hpp"
#include "src/GatewayIn/include/websocket/BitVavoNetworkClient.hpp"

namespace demo {

class Visualizer {
public:
    Visualizer(gateway::QuotesObtainer<gateway::BitvavoWebSocketClient>& obt,
               std::string market)
        : obt_(obt), market_(std::move(market)) {}

    void run() {
        using namespace std::chrono;

        print_header();

        auto lastPrint = steady_clock::now();
        std::size_t bidsThisInterval = 0;
        std::size_t asksThisInterval = 0;

        gateway::Quote q{};
        std::optional<gateway::Quote> latestBid;
        std::optional<gateway::Quote> latestAsk;

        for (;;) {
            // Drain queues
            while (obt_.getBidQueue().pop(q)) { latestBid = q; ++bidsThisInterval; }
            while (obt_.getAskQueue().pop(q)) { latestAsk = q; ++asksThisInterval; }

            // Print once per second
            auto now = steady_clock::now();
            if (now - lastPrint >= 1s) {
                print_line(latestBid, latestAsk, bidsThisInterval, asksThisInterval);
                bidsThisInterval = asksThisInterval = 0;
                lastPrint = now;
            }

            std::this_thread::sleep_for(10ms);
        }
    }

private:
    gateway::QuotesObtainer<gateway::BitvavoWebSocketClient>& obt_;
    std::string market_;

    static void print_header() {
        std::cout << "Time                 | Market   | Latest Bid            | Latest Ask            "
                     "| Peak Bid             | Peak Ask             | Bids/s | Asks/s\n";
        std::cout << "---------------------+----------+-----------------------+-----------------------"
                     "+----------------------+----------------------+--------+-------\n";
    }

    void print_line(const std::optional<gateway::Quote>& latestBid,
                    const std::optional<gateway::Quote>& latestAsk,
                    std::size_t bidsThisInterval,
                    std::size_t asksThisInterval) {
        using namespace std::chrono;

        auto sysnow = system_clock::now();
        auto t = system_clock::to_time_t(sysnow);
        std::tm tm{};
        localtime_r(&t, &tm);

        std::ostringstream line;
        line << std::put_time(&tm, "%F %T") << " | "
             << std::setw(8) << market_ << " | ";

        auto fmt = [](const std::optional<gateway::Quote>& qq) {
            std::ostringstream os; os.setf(std::ios::fixed); os << std::setprecision(2);
            if (qq) os << std::setw(8) << qq->getPrice() << " x " << std::setw(7) << qq->getSize();
            else    os << std::setw(8) << "-" << " x " << std::setw(7) << "-";
            return os.str();
        };

        line << std::setw(23) << fmt(latestBid) << " | "
             << std::setw(23) << fmt(latestAsk) << " | ";

        line << std::setw(21);
        if (obt_.peakBidQuote_) line << std::fixed << std::setprecision(2) << obt_.peakBidQuote_->getPrice(); else line << "-";
        line << " | ";

        line << std::setw(21);
        if (obt_.peakAskQuote_) line << std::fixed << std::setprecision(2) << obt_.peakAskQuote_->getPrice(); else line << "-";
        line << " | ";

        line << std::setw(6) << bidsThisInterval << " | "
             << std::setw(5) << asksThisInterval;

        std::cout << line.str() << '\n';
    }
};

} // namespace demo
