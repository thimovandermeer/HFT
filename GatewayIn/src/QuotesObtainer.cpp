#if 0
#include "../include/QuotesObtainer.hpp"
#include "../../Parser/include/BitvavoBookParser.hpp"
#include "../../Parser/include/FixBookParser.hpp"
#include <iostream>
#include <variant>
#include <thread>
#include <atomic>
#include <deque>

using std::chrono::system_clock;
using std::chrono::time_point;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

namespace gateway
{

	QuotesObtainer::QuotesObtainer(
			Client client,
			AnyFeed feed,
			std::string host,
			std::string port,
			std::string market
			)
			:
			client_(std::move(client))
			feed_(std::move(feed)),
			host_(std::move(host)),
			port_(std::move(port)),
			market_(std::move(market))
	{
		
		std::visit(Overloaded{
				[this](PixNetworkClient& c) {
					c.setMessageHandler([this](std::string_view msg){
						this->parseFix(msg);
					});
					c.setErrorHandler([this](std::string_view err){
						std::cerr << "PIX Error: " << err << std::endl;
						disconnect();
						std::cout << "Reconnecting...";
						startReconnectLoop();
					});
				},
				[this](BitvavoWebSocketClient& c) {
					c.setMessageHandler([this](std::string_view frame){
						this->parseBitvavo(frame);
					});
					c.setErrorHandler([this](std::string_view err){
						std::cerr << "Bitvavo Error: " << err << std::endl;
						disconnect();
						std::cout << "Reconnecting...";
						startReconnectLoop();
					});
				}
		}, feed_);
	}

	QuotesObtainer::~QuotesObtainer()
	{
		disconnect();
	}

	bool QuotesObtainer::connect()
	{
		bool ok = std::visit(Overloaded{
				[this](PixNetworkClient& c) {
					return c.connect(host_, port_);
				},
				[this](BitvavoWebSocketClient& c) {
					if (!c.connect(host_, port_)) return false;
					std::string sub = std::string(R"({"action":"subscribe","channels":[{"name":"book","markets":[")")
									  + market_ + R"("]}]})";
					c.send(sub);
					return true;
				}
		}, feed_);
		return ok;
	}

	void QuotesObtainer::disconnect() {
		if (feed_.valueless_by_exception()) return;
		std::visit([](auto& c) { c.disconnect(); }, feed_);
	}

	void QuotesObtainer::parseFix(std::string_view fixMessage)
	{
		auto quote = fix::parseAndStoreQuote(fixMessage);
		if(quote.has_value()) {
			storeQuote(quote.value());
		} else {
			std::cerr << "No quote created from message" << std::endl;
		}
	}

	void QuotesObtainer::parseBitvavo(std::string_view bitVavoMessage)
	{
		auto result = bitvavo::parseAndStoreQuote(bitVavoMessage, market_);
		if(result.has_value()) {
			storeQuote(result.value());
		} else {
			std::cerr << "Unable to obtain quote from bitvavomessage";
		}
	}

	void QuotesObtainer::storeQuote(const Quote& quote) {
		if (quote.getSide() == QuoteSide::Bid) {
			if (!bidQuoteQueue_.push(quote)) std::cerr << "Bid queue full for host " << host_ << ":" << port_ << "\n";

			if (!peakBidQuote_ || quote.getPrice() > peakBidQuote_->getPrice()) {
				peakBidQuote_ = quote;
			}

			bidTimestamps_.push_back(quote.getTimestamp());
			if (bidTimestamps_.size() > 100) {
				bidTimestamps_.pop_front();
			}

		} else if (quote.getSide() == QuoteSide::Ask) {
			if (!askQuoteQueue_.push(quote)) std::cerr << "Ask queue full for host " << host_ << ":" << port_ << "\n";

			if (!peakAskQuote_ || quote.getPrice() < peakAskQuote_->getPrice()) {
				peakAskQuote_ = quote;
			}

			askTimestamps_.push_back(quote.getTimestamp());
			if (askTimestamps_.size() > 100) {
				askTimestamps_.pop_front();
			}
		}
	}

	void QuotesObtainer::setUpdateInterval(std::chrono::milliseconds interval)
	{
		updateInterval_ = interval;
	}

	void QuotesObtainer::startReconnectLoop()
	{
		if (reconnecting_.exchange(true)) return;
		std::thread([this]() {
			std::uniform_int_distribution<int> jitterDist(0, 50);
			size_t attempts = 0;

			while (attempts < maxReconnectAttempts_) {
				std::cerr << "[Reconnect] Attempt " << (attempts + 1)
						  << " for " << host_ << ":" << port_ << "\n";

				if (connect()) {
					std::cout << "[Reconnect] Success for " << host_ << ":" << port_ << "\n";
					reconnecting_ = false;
					return;
				}

				++attempts;
				auto backoff = std::min(baseBackoff_ * (1 << attempts), maxBackoff_);
				backoff += std::chrono::milliseconds(jitterDist(rng_));
				std::this_thread::sleep_for(backoff);
			}

			std::cerr << "[Reconnect] Failed after " << attempts << " attempts for "
					  << host_ << ":" << port_ << "\n";
			reconnecting_ = false;
		}).detach();
	}

	double QuotesObtainer::avgInterval(const std::deque<std::chrono::system_clock::time_point> timestamps) const {
		if (timestamps.size() < 2) return 0.0;
		double total = 0.0;
		for (size_t i = 1; i < timestamps.size(); ++i) {
			total += duration_cast<milliseconds>(timestamps[i] - timestamps[i - 1]).count();
		}
		return total / (timestamps.size() - 1);
	}


	boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>>& QuotesObtainer::getBidQueue() {
		return bidQuoteQueue_;
	}

	boost::lockfree::spsc_queue<gateway::Quote, boost::lockfree::capacity<1024>>& QuotesObtainer::getAskQueue() {
		return askQuoteQueue_;
	}
}
#endif
