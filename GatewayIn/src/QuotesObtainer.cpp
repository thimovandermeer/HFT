#include "../include/QuotesObtainer.hpp"
#include "../include/PixNetworkClient.hpp"
#include <deque>
#include <chrono>

using std::chrono::system_clock;
using std::chrono::time_point;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

namespace gateway
{
	QuotesObtainer::QuotesObtainer(
			std::unique_ptr<PixNetworkClient> networkClient,
			std::string host,
			std::string port
			)
			:
			pixClient_(std::move(networkClient)),
			host_(std::move(host)),
			port_(std::move(port))
	{
		pixClient_->setMessageHandler([this](std::string_view message) {
			this->parseAndStoreQuote(message);
		});

		pixClient_->setErrorHandler([this](std::string_view error) {
			std::cerr << "PIX Error: " << error << std::endl;
			disconnect();
			std::cout << "Reconnecting...";
			startReconnectLoop();
		});
	}

	QuotesObtainer::~QuotesObtainer()
	{
		disconnect();
	}

	bool QuotesObtainer::connect()
	{
		if(pixClient_->connect(host_, port_)) {
			return true;
		} else {
			return false;
		}
	}

	void QuotesObtainer::disconnect()
	{
		if (pixClient_) {
			pixClient_->disconnect();
		}
	}

	void QuotesObtainer::setUpdateInterval(std::chrono::milliseconds interval)
	{
		updateInterval_ = interval;
	}

	inline void QuotesObtainer::parseAndStoreQuote(std::string_view fixMessage) {
		try {
			std::unordered_map<std::string_view, std::string_view> fields;
			size_t pos = 0;
			while (pos < fixMessage.size()) {
				size_t eq = fixMessage.find('=', pos);
				if (eq == std::string_view::npos) break;

				size_t soh = fixMessage.find('\x01', eq);
				if (soh == std::string_view::npos) break;

				auto tag = fixMessage.substr(pos, eq - pos);
				auto value = fixMessage.substr(eq + 1, soh - eq - 1);
				fields[tag] = value;

				pos = soh + 1;
			}

			std::string_view msgType = fields["35"];
			if (msgType != "X" && msgType != "W") return;

			std::string_view symbol = fields["55"];
			int numEntries = std::stoi(std::string(fields["268"]));
			size_t current = fixMessage.find("268=");

			for (int i = 0; i < numEntries; ++i) {
				size_t entryStart = fixMessage.find("269=", current);
				if (entryStart == std::string_view::npos) break;

				std::string_view side = fixMessage.substr(entryStart + 4, 1);

				size_t pxPos = fixMessage.find("270=", entryStart);
				size_t pxEnd = fixMessage.find('\x01', pxPos);
				double price = std::stod(std::string(fixMessage.substr(pxPos + 4, pxEnd - pxPos - 4)));

				size_t szPos = fixMessage.find("271=", entryStart);
				size_t szEnd = fixMessage.find('\x01', szPos);
				double size = std::stod(std::string(fixMessage.substr(szPos + 4, szEnd - szPos - 4)));

				Quote quote(price, size, std::chrono::system_clock::now(), symbol, side == "0" ? QuoteSide::Bid : QuoteSide::Ask);

				if (side == "0") {
					if (!bidQuoteQueue_.push(quote)) std::cerr << "Bid queue full for host " << host_ << ":" << port_ << "\n";

					if (!peakBidQuote_ || quote.getPrice() > peakBidQuote_->getPrice()) {
						peakBidQuote_ = quote;
					}

					bidTimestamps_.push_back(quote.getTimestamp());
					if (bidTimestamps_.size() > 100) {
						bidTimestamps_.pop_front();
					}

				} else if (side == "1") {
					if (!askQuoteQueue_.push(quote)) std::cerr << "Ask queue full for host " << host_ << ":" << port_ << "\n";

					if (!peakAskQuote_ || quote.getPrice() < peakAskQuote_->getPrice()) {
						peakAskQuote_ = quote;
					}

					askTimestamps_.push_back(quote.getTimestamp());
					if (askTimestamps_.size() > 100) {
						askTimestamps_.pop_front();
					}
				}

				current = pxEnd;
			}
		} catch (const std::exception& e) {
			std::cerr << "Exception thrown while parsing FIX message: " << e.what() << "\n";
			std::cerr << "Message: " << fixMessage << "\n";
		}

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
