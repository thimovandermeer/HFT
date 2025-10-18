#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>


class Visualizer {
public:
    explicit Visualizer(const OrderBookView& view, std::string market)
        : view_(view), market_(std::move(market)) {}

    void run() {
        using namespace std::chrono;
        print_header();
        auto lastPrint = steady_clock::now();

        for (;;) {
            auto now = steady_clock::now();
            if (now - lastPrint >= 1s) {
                auto snap = view_.read();
                print_line(snap);
                lastPrint = now;
            }
            std::this_thread::sleep_for(5ms);
        }
    }

private:
    const OrderBookView& view_;
    std::string market_;

    static void print_header() {
        std::cout << "Time                 | Market   | Latest Bid            | Latest Ask\n";
        std::cout << "---------------------+----------+-----------------------+-----------------------\n";
    }
    static void print_line(const OrderBookSnapshot& s) {
        using namespace std::chrono;
        auto t = system_clock::to_time_t(system_clock::now());
        std::tm tm{}; localtime_r(&t, &tm);

        auto fmt = [](double px, std::optional<double> sz)->std::string{
            std::ostringstream os; os.setf(std::ios::fixed); os<<std::setprecision(2);
            if (std::isnan(px)) return std::string(8,'-') + " x " + std::string(7,'-');
            os<<std::setw(8)<<px<<" x "<<std::setw(7)<<(sz?*sz:0.0);
            return os.str();
        };

        std::optional<double> bbSize, baSize;
        if (!s.bidLevels.empty() && !std::isnan(s.bestBid) && s.bidLevels.front().first == s.bestBid) bbSize = s.bidLevels.front().second;
        if (!s.askLevels.empty() && !std::isnan(s.bestAsk) && s.askLevels.front().first == s.bestAsk) baSize = s.askLevels.front().second;

        std::ostringstream line;
        line << std::put_time(&tm, "%F %T") << " | "
             << std::setw(8) << s.symbol << " | "
             << std::setw(23) << fmt(s.bestBid, bbSize) << " | "
             << std::setw(23) << fmt(s.bestAsk, baSize);
        std::cout << line.str() << '\n';
    }
};