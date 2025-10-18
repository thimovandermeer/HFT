#pragma once
#include "WebSocketClientBase.hpp"
#include <iostream>
#include <unordered_set>

namespace gateway
{
	class BitvavoWebSocketClient final : public WebSocketClientBase<BitvavoWebSocketClient>
	{
	public:
		using MessageHandler = std::function<void(std::string_view)>;
		using ErrorHandler = std::function<void(std::string_view)>;

		BitvavoWebSocketClient() = default;
		BitvavoWebSocketClient(const BitvavoWebSocketClient&) = delete;
		BitvavoWebSocketClient& operator=(const BitvavoWebSocketClient&) = delete;
		BitvavoWebSocketClient(BitvavoWebSocketClient&& other) noexcept
				: WebSocketClientBase<BitvavoWebSocketClient>(std::move(other))
				, markets_(std::move(other.markets_))
				, messageHandler_(std::move(other.messageHandler_))
				, errorHandler_(std::move(other.errorHandler_))
		{}

		BitvavoWebSocketClient& operator=(BitvavoWebSocketClient&&) noexcept = delete;
		void setMessageHandler(MessageHandler handler) {
			messageHandler_ = handler;
		}

		void setErrorHandler(ErrorHandler handler) {
			errorHandler_ = handler;
		}


		void subscribeBook(const std::string &market)
		{
			markets_.insert(market);
			std::string sub = R"({"action":"subscribe","channels":[{"name":"book","markets":[")"
							  + market + R"("]}]})";
			send(sub);
		}

		bool connect(std::string_view host, std::string_view port)
		{
			return WebSocketClientBase<BitvavoWebSocketClient>::connect(host, port);
		}

		void onOpen()
		{
			std::cout << "[Bitvavo] WS opened" << std::endl;
		}

		void onMessage(std::string_view frame)
		{
			if (messageHandler_) {
				messageHandler_(frame);
				return;
			}
			if (frame.find("\"event\":\"book\"") != std::string_view::npos) {
				std::cout << "[Bitvavo] book frame, bytes=" << frame.size() << "\n";
			}
		}

		void onClosed()
		{
			std::cout << "[Bitvavo] WS closed\n";
		}

		void onError(std::string_view what)
		{
			if (what.find("Operation canceled") != std::string_view::npos ||
				what.find("operation canceled") != std::string_view::npos) {
				return;
			}
			if (errorHandler_) {
				errorHandler_(what);
				return;
			}
			std::cerr << "[Bitvavo] ERROR: " << what << "\n";
		}

	private:
		std::unordered_set<std::string> markets_;
		MessageHandler messageHandler_;
		ErrorHandler errorHandler_;
	};
}