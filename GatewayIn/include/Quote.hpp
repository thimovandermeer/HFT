#pragma once

#include <chrono>
#include <string_view>

namespace gateway {
	enum class QuoteSide { Bid, Ask };

	class Quote {
	public:
		Quote(double price,
			  std::chrono::system_clock::time_point timestamp,
			  std::string_view symbol,
			  QuoteSide side)
				: price_{price}
				, timestamp_{timestamp}
				, symbol_{std::move(symbol)}
				, side_{side} {}

		Quote() = default;

		[[nodiscard]] double getPrice() const noexcept { return price_; }
		[[nodiscard]] std::chrono::system_clock::time_point getTimestamp() const noexcept { return timestamp_; }
		[[nodiscard]] std::string_view getSymbol() const noexcept { return symbol_; }
		[[nodiscard]] QuoteSide getSide() const noexcept { return side_; }

	private:
		double price_{};
		std::chrono::system_clock::time_point timestamp_;
		std::string_view symbol_;
		QuoteSide side_;
	};
}

