#pragma once

#include <chrono>
#include <string_view>

namespace gateway {
	enum class QuoteSide { Bid, Ask };

	class Quote {
	public:
		Quote(double price,
			  double size,
			  std::chrono::system_clock::time_point timestamp,
			  std::string_view symbol,
			  QuoteSide side)
				: price_{price}
				, size_{size}
				, timestamp_{timestamp}
				, symbol_{std::move(symbol)}
				, side_{side} {}

		Quote() = default;

		[[nodiscard]] double getPrice() const noexcept { return price_; }
		[[nodiscard]] std::chrono::system_clock::time_point getTimestamp() const noexcept { return timestamp_; }
		[[nodiscard]] std::string_view getSymbol() const noexcept { return symbol_; }
		[[nodiscard]] QuoteSide getSide() const noexcept { return side_; }
		[[nodiscard]] double getSize() const noexcept { return size_; }

	private:
		double price_{};
		double size_{};
		std::chrono::system_clock::time_point timestamp_;
		std::string_view symbol_;
		QuoteSide side_;
	};
}

