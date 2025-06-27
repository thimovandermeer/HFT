#pragma once

#include "NetworkClientBase.hpp"
#include "QuotesObtainer.hpp"
#include <iostream>

namespace gateway {

	class PixNetworkClient : public NetworkClientBase<PixNetworkClient> {
	public:
		using MessageHandler = std::function<void(std::string_view)>;
		using ErrorHandler = std::function<void(std::string_view)>;

		PixNetworkClient() = default;

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

		void handleError(std::string_view error) {
			if (errorHandler_) {
				errorHandler_(error);
			} else {
				std::cerr << "PIX error: " << error << '\n';
			}
		}

	private:
		MessageHandler messageHandler_;
		ErrorHandler errorHandler_;
	};

} // namespace gateway
