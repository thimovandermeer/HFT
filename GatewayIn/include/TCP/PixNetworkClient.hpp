#pragma once

#include "NetworkClientBase.hpp"
#include "QuotesObtainer.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace gateway {

	class PixNetworkClient : public NetworkClientBase<PixNetworkClient> {
	public:
		using MessageHandler = std::function<void(std::string_view)>;
		using ErrorHandler = std::function<void(std::string_view)>;

		PixNetworkClient() = default;
		PixNetworkClient(const PixNetworkClient&) = delete;
		PixNetworkClient& operator=(const PixNetworkClient&) = delete;
		PixNetworkClient(PixNetworkClient&&) noexcept = default;
		PixNetworkClient& operator=(PixNetworkClient&&) noexcept = default;
		void setMessageHandler(MessageHandler handler) {
			messageHandler_ = std::move(handler);
		}

		void setErrorHandler(ErrorHandler handler) {
			errorHandler_ = std::move(handler);
		}

		void handleMessage(std::string_view message) {
			if (messageHandler_) {
				messageHandler_(message);
			}
		}

		bool isLogonAck(const std::string_view& fix_msg) const {
			return fix_msg.find("35=A") != std::string::npos &&
				   fix_msg.find("49=" + targetCompId_) != std::string::npos &&
				   fix_msg.find("56=" + senderCompId_) != std::string::npos;
		}

		void handleReceive(const char* data, std::size_t size) {
			sliding_buffer_.append(data, size);

			while (true) {
				std::size_t msg_end = findFixMessageEnd(sliding_buffer_);
				if (msg_end == std::string::npos)
					break;

				std::string_view fix_msg(sliding_buffer_.data(), msg_end);

				if (isLogonAck(fix_msg) && !loggedOn_.exchange(true)) {
//					std::cout << "Logged in let's go" << std::endl;
					loggedOn_.store(true);
					sendMarketDataRequest("EUR/USD");
				} else {
					handleMessage(fix_msg);
				}
				sliding_buffer_.erase(0, msg_end);
			}
		}

		void loginAndSubscribe(std::string_view symbol) {
			sendFixLogon();
		}

		void sendFixLogon() {
			std::cout << "sending logon" << std::endl;
			std::ostringstream fields;
			fields << "98=0" << '\x01';
			fields << "108=30" << '\x01';

			std::string fix = buildFixMessage("A", fields.str());
			send(fix);
		}

		void sendMarketDataRequest(std::string_view symbol) {
			static int reqCounter = 1;

			std::ostringstream fixBody;

			fixBody << "262=req-" << reqCounter++ << '\x01';
			fixBody << "263=1" << '\x01';
			fixBody << "264=1" << '\x01';
			fixBody << "265=0" << '\x01';
			fixBody << "267=2" << '\x01';
			fixBody << "269=0" << '\x01';
			fixBody << "269=1" << '\x01';


			fixBody << "146=1" << '\x01';
			fixBody << "55=" << symbol << '\x01';
			fixBody << "460=4" << '\x01';

			std::string fix = buildFixMessage("V", fixBody.str());

			for (char c : fix) std::cout << (c == '\x01' ? '|' : c);
			std::cout << std::endl;

			send(fix);
		}

		void onConnectionReady() {
			loggedOn_.store(false);
			loginAndSubscribe("EUR/USD");
		}

		void onDisconnect() {
			outgoingSeqNum_ = 1;
			loggedOn_.store(false);
		}

		std::string formattedUtcNow() {
			using namespace std::chrono;

			auto now = system_clock::now();
			auto now_time_t = system_clock::to_time_t(now);
			auto now_ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

			std::tm utc_tm;

			gmtime_r(&now_time_t, &utc_tm);


			std::ostringstream oss;
			oss << std::put_time(&utc_tm, "%Y%m%d-%H:%M:%S");
			oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

			return oss.str();
		}

		std::string computeChecksum(const std::string& message) {
			unsigned int sum = 0;
			for (unsigned char c : message) {
				sum += c;
			}
			unsigned int checksum = sum % 256;

			std::ostringstream oss;
			oss << std::setw(3) << std::setfill('0') << checksum;
			return oss.str();
		}

		std::string buildFixMessage(
				const std::string& msgType,
				const std::string& bodyFields)
		{
			std::ostringstream body;

			body << "35=" << msgType << '\x01';
			body << "49=" << senderCompId_ << '\x01';
			body << "56=" << targetCompId_ << '\x01';
			body << "34=" << outgoingSeqNum_++ << '\x01';
			body << "52=" << formattedUtcNow() << '\x01';

			body << bodyFields;

			const std::string bodyStr = body.str();
			const std::size_t bodyLength = bodyStr.size();

			std::ostringstream full;
			full << "8=FIX.4.4" << '\x01';
			full << "9=" << bodyLength << '\x01';
			full << bodyStr;

			const std::string msgWithoutChecksum = full.str();
			const std::string checksum = computeChecksum(msgWithoutChecksum);

			full << "10=" << checksum << '\x01';

			return full.str();
		}

		void handleError(std::string_view error) {
			if (errorHandler_) {
				errorHandler_(error);
			} else {
				std::cerr << "error handler not set, unable to handle error: " << error << '\n';
			}
		}

		std::size_t findFixMessageEnd(const std::string& buffer) {
			std::size_t pos = buffer.find("10=");
			while (pos != std::string::npos) {
				if (pos + 6 < buffer.size() && buffer[pos + 6] == '\x01') {
					return pos + 7;
				}
				pos = buffer.find("10=", pos + 1);
			}
			return std::string::npos;
		}

	private:
		MessageHandler messageHandler_;
		ErrorHandler errorHandler_;

		std::string sliding_buffer_;
		int outgoingSeqNum_ = 1;
		std::atomic<bool> loggedOn_{false};
		std::string senderCompId_ = "FIXSIM-CLIENT-MKD";
		std::string targetCompId_ = "FIXSIM-SERVER-MKD";
	};

} // namespace gateway
