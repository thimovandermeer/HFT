#pragma once
#include <gmock/gmock.h>
#include <functional>
#include <string_view>

namespace gateway {

	class MockBitvavoClient {
	public:
		using MessageHandler = std::function<void(std::string_view)>;
		using ErrorHandler   = std::function<void(std::string_view)>;

		MOCK_METHOD(bool, connect, (const std::string& host, const std::string& port), ());
		MOCK_METHOD(void, disconnect, (), ());
		MOCK_METHOD(void, send, (const std::string& payload), ());
		MOCK_METHOD(void, subscribeBook, (const std::string& market), ());
		MOCK_METHOD(void, setMessageHandler, (MessageHandler), ());
		MOCK_METHOD(void, setErrorHandler, (ErrorHandler), ());
	};

} // namespace gateway