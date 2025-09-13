#pragma once
#include <string_view>
#include <vector>
#include <string>
#include <cstdlib>
#include <chrono>
#include "../../GatewayIn/include/Quote.hpp"

namespace bitvavo {

	inline uint64_t extractNonce(std::string_view js) {
		auto p = js.find("\"nonce\":");
		if (p == std::string_view::npos) return 0;
		p += 8;
		uint64_t v = 0;
		while (p < js.size() && js[p] >= '0' && js[p] <= '9') { v = v*10 + (js[p]-'0'); ++p; }
		return v;
	}

	inline bool parseFirstLevel(std::string_view frame,
								std::string_view key,
								double& px,
								double& qty)
	{
		std::string pat = std::string("\"") + std::string(key) + "\":[[\"";
		auto p = frame.find(pat);
		if (p == std::string_view::npos) return false;
		p += pat.size();

		auto q = frame.find('"', p);
		if (q == std::string_view::npos || q == p) return false; // empty price invalid

		std::string price_str(frame.substr(p, q - p));
		char* endp = nullptr;
		const char* cprice = price_str.c_str();
		px = std::strtod(cprice, &endp);
		if (endp != cprice + price_str.size()) {
			return false;
		}

		auto q2a = frame.find('"', q + 2);
		if (q2a == std::string_view::npos) return false;
		auto q2b = frame.find('"', q2a + 1);
		if (q2b == std::string_view::npos || q2b == q2a + 1) return false; // empty qty invalid

		std::string qty_str(frame.substr(q2a + 1, q2b - q2a - 1));
		endp = nullptr;
		const char* cqty = qty_str.c_str();
		qty = std::strtod(cqty, &endp);
		if (endp != cqty + qty_str.size()) {
			return false;
		}

		return true;
	}

	inline std::optional<gateway::Quote>
	parseAndStoreQuote(std::string_view frame, std::string_view market)
	{
		try {
			if (frame.find("\"event\":\"book\"") == std::string_view::npos) {
				return std::nullopt;
			}

			const auto now = std::chrono::system_clock::now();

			double px = 0.0, qty = 0.0;

			if (parseFirstLevel(frame, "bids", px, qty)) {
				return gateway::Quote(px, qty, now, market, gateway::QuoteSide::Bid);
			}
			if (parseFirstLevel(frame, "asks", px, qty)) {
				return gateway::Quote(px, qty, now, market, gateway::QuoteSide::Ask);
			}

			return std::nullopt;
		} catch (const std::exception& e) {
			std::cerr << "Exception while parsing Bitvavo book frame: " << e.what() << "\n";
			std::cerr << "Frame (truncated): "
					  << std::string(frame.substr(0, std::min<size_t>(frame.size(), 256))) << "...\n";
			return std::nullopt;
		}
	}

} // namespace bitvavo

