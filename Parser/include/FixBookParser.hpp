//
// Created by Thimo Van der meer on 30/08/2025.
//

#ifndef HFT_FIXBOOKPARSER_HPP
#define HFT_FIXBOOKPARSER_HPP
#include <iostream>
#include "../../GatewayIn/include/Quote.hpp"


namespace fix {

	inline std::optional<gateway::Quote> parseAndStoreQuote(std::string_view fixMessage) {
		try {
			std::unordered_map<std::string_view, std::string_view> fields;
			size_t pos = 0;
			while (pos < fixMessage.size()) {
				size_t eq = fixMessage.find('=', pos);
				if (eq == std::string_view::npos) break;

				size_t soh = fixMessage.find('\x01', eq);
				if (soh == std::string_view::npos) break;

				auto tag = fixMessage.substr(pos, eq - pos);
				auto value = fixMessage.substr(eq + 1, soh - eq - 1);
				fields[tag] = value;

				pos = soh + 1;
			}

			std::string_view msgType = fields["35"];
			if (msgType != "X" && msgType != "W") return std::nullopt;

			std::string_view symbol = fields["55"];
			int numEntries = std::stoi(std::string(fields["268"]));
			size_t current = fixMessage.find("268=");

			for (int i = 0; i < numEntries; ++i) {
				size_t entryStart = fixMessage.find("269=", current);
				if (entryStart == std::string_view::npos) break;

				std::string_view side = fixMessage.substr(entryStart + 4, 1);

				size_t pxPos = fixMessage.find("270=", entryStart);
				size_t pxEnd = fixMessage.find('\x01', pxPos);
				double price = std::stod(std::string(fixMessage.substr(pxPos + 4, pxEnd - pxPos - 4)));

				size_t szPos = fixMessage.find("271=", entryStart);
				size_t szEnd = fixMessage.find('\x01', szPos);
				double size = std::stod(std::string(fixMessage.substr(szPos + 4, szEnd - szPos - 4)));

				return (gateway::Quote(price, size, std::chrono::system_clock::now(), symbol, side == "0" ? gateway::QuoteSide::Bid : gateway::QuoteSide::Ask));


				current = pxEnd;
			}
		} catch (const std::exception& e) {
			std::cerr << "Exception thrown while parsing FIX message: " << e.what() << "\n";
			std::cerr << "Message: " << fixMessage << "\n";
		}
		return std::nullopt;
	}
}
#endif //HFT_FIXBOOKPARSER_HPP
