#pragma once
#include <gmock/gmock.h>
#include <functional>
#include <string>
#include <string_view>

namespace gateway {

 class PixNetworkClient {
 public:
 	using MsgHandler   = std::function<void(std::string_view)>;
 	using MessageHandler = MsgHandler; // compatibility alias for tests
 	using ErrorHandler = std::function<void(std::string_view)>;

		PixNetworkClient() { s_instance = this; }

		// gMock API
		MOCK_METHOD(bool, connect, (const std::string& host, const std::string& port), ());
		MOCK_METHOD(void, disconnect, (), ());
		MOCK_METHOD(void, setMessageHandler, (MsgHandler), ());
		MOCK_METHOD(void, setErrorHandler,   (ErrorHandler), ());
		// Pix heeft geen send() in jouw pad, maar laat staan als je het wil toevoegen:
		// MOCK_METHOD(void, send, (const std::string&), ());

		static PixNetworkClient* instance() { return s_instance; }
		static void TriggerMessage(std::string_view m) { if (s_on_msg) s_on_msg(m); }
		static void TriggerError(std::string_view e)   { if (s_on_err) s_on_err(e); }

		static inline MsgHandler   s_on_msg{};
		static inline ErrorHandler s_on_err{};

	private:
		static inline PixNetworkClient* s_instance = nullptr;
	};

} // namespace gateway
