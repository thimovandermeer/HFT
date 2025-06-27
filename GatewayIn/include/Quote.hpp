#pragma once

#include <chrono>
#include <string_view>

namespace gateway {
	class Quote {
	public:
		Quote(double price,
			  std::chrono::system_clock::time_point timestamp,
			  std::string_view symbol)
				: price_{price}
				, timestamp_{timestamp}
				, symbol_{symbol} {}

		[[nodiscard]] double getPrice() const noexcept { return price_; }
		[[nodiscard]] std::chrono::system_clock::time_point getTimestamp() const noexcept { return timestamp_; }
		[[nodiscard]] std::string_view getSymbol() const noexcept { return symbol_; }

	private:
		double price_;
		std::chrono::system_clock::time_point timestamp_;
		std::string symbol_;
	};
}
