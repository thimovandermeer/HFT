#pragma once
#include <string_view>
#include <string>
#include <optional>
#include <chrono>
#include <iostream>
#include "Quote.hpp"

namespace bitvavo {

	inline uint64_t extractNonce(std::string_view js) {
		auto p = js.find("\"nonce\":");
		if (p == std::string_view::npos) return 0;
		p += 8;
		uint64_t v = 0;
		while (p < js.size() && js[p] >= '0' && js[p] <= '9') {
			v = v * 10 + (js[p] - '0');
			++p;
		}
		return v;
	}

	inline bool parseFirstLevel(std::string_view frame,
							std::string_view key,
							double& px,
							double& qty)
	{
		auto keyPos = frame.find(key);
		if (keyPos == std::string_view::npos)
			return false;

		auto start = frame.find('[', keyPos);
		if (start == std::string_view::npos)
			return false;

		auto nextChar = frame.find_first_not_of(" []", start + 1);
		if (nextChar == std::string_view::npos || frame[nextChar] == ']')
			return false;

		auto end = frame.find(']', start);
		if (end == std::string_view::npos)
			return false;

		std::string inner(frame.substr(start + 1, end - start - 1));

		inner.erase(std::remove_if(inner.begin(), inner.end(), [](unsigned char c) {
			return c == '"' || c == '[' || c == ']' || std::isspace(c);
		}), inner.end());

		if (inner.empty()) return false;

		auto comma = inner.find(',');
		if (comma == std::string::npos)
			return false;

		std::string priceStr = inner.substr(0, comma);
		std::string qtyStr   = inner.substr(comma + 1);

		char* endp = nullptr;
		px  = std::strtod(priceStr.c_str(), &endp);
		if (endp == priceStr.c_str() || *endp != '\0')
			return false;

		endp = nullptr;
		qty = std::strtod(qtyStr.c_str(), &endp);
		if (endp == qtyStr.c_str() || *endp != '\0')
			return false;

		return true;
	}


	inline std::optional<gateway::Quote>
	parseAndStoreQuote(std::string_view frame, std::string_view market)
	{
		try {
			if (frame.find(R"("event":"book")") == std::string_view::npos)
				return std::nullopt;

			const auto now = std::chrono::system_clock::now();
			double px = 0.0, qty = 0.0;

			if (parseFirstLevel(frame, "bids", px, qty)) {
				return gateway::Quote(px, qty, now, market, gateway::QuoteSide::Bid);
			}

			if (parseFirstLevel(frame, "asks", px, qty)) {
				return gateway::Quote(px, qty, now, market, gateway::QuoteSide::Ask);
			}

			std::cerr << "[Bitvavo parser] No bid/ask found in frame: " << frame.substr(0, 200) << "\n";

			return std::nullopt;

		} catch (const std::exception& e) {
			std::cerr << "Exception while parsing Bitvavo book frame: " << e.what() << "\n";
			std::cerr << "Frame (truncated): "
			          << std::string(frame.substr(0, std::min<size_t>(frame.size(), 256))) << "...\n";
			return std::nullopt;
		}
	}

} // namespace bitvavo
