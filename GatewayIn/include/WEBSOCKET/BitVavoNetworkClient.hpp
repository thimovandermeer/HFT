// BitvavoWebSocketClient.hpp
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
				: WebSocketClientBase<BitvavoWebSocketClient>(std::move(other)) // base move-ctor
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

		// ---- CRTP hooks ----
		bool connect(std::string_view host, std::string_view port)
		{
			// Bitvavo needs target "/v2/" — pass that to connect().
			// Optionally send pings later; Beast auto-handles pongs.
			std::cout << "[Bitvavo] WS opened\n";
			return true;
		}

		void onMessage(std::string_view frame)
		{
			// Learning step 1: just print length / peek a few chars.
			// Later: parse JSON, check event == "book", route.
			if (frame.find("\"event\":\"book\"") != std::string_view::npos) {
				// For now, show minimal proof this works:
				std::cout << "[Bitvavo] book frame, bytes=" << frame.size() << "\n";
				// TODO: parse bids/asks & nonce, then hand to your OrderBook
			} else {
				// You’ll also receive system messages (subscriptions, heartbeats, etc.)
				// std::cout << "[Bitvavo] other: " << frame << "\n";
			}
		}

		void onClosed()
		{
			std::cout << "[Bitvavo] WS closed\n";
		}

		void onError(std::string_view what)
		{
			std::cerr << "[Bitvavo] ERROR: " << what << "\n";
		}

	private:
		std::unordered_set<std::string> markets_;
		MessageHandler messageHandler_;
		ErrorHandler errorHandler_;
	};
}