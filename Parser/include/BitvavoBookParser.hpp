// BitvavoBookParser.hpp
#pragma once
#include <string_view>
#include <vector>
#include <string>
#include <cstdlib>
#include <chrono>
#include "Quote.hpp"

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

		// price "..."
		auto q = frame.find('"', p);
		if (q == std::string_view::npos) return false;
		px = std::strtod(std::string(frame.substr(p, q - p)).c_str(), nullptr);

		// qty after ,"  "..."
		auto q2a = frame.find('"', q + 3);
		if (q2a == std::string_view::npos) return false;
		auto q2b = frame.find('"', q2a + 1);
		if (q2b == std::string_view::npos) return false;
		qty = std::strtod(std::string(frame.substr(q2a + 1, q2b - q2a - 1)).c_str(), nullptr);

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
				// TODO: Move Quote construction (agnostic piece) out if you want zero coupling here.
				return gateway::Quote(px, qty, now, market, gateway::QuoteSide::Bid);
			}
			if (parseFirstLevel(frame, "asks", px, qty)) {
				// TODO: Move Quote construction (agnostic piece) out if you want zero coupling here.
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

