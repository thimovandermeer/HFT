#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <optional>

#include "../../GatewayIn/include/Quote.hpp"

#include "FixBookParser.hpp"

using fix::parseAndStoreQuote;

namespace {

	constexpr char SOH = '\x01';

	std::string fix_msg(std::initializer_list<std::pair<std::string, std::string>> fields) {
		std::string m;
		m.reserve(256);
		for (auto& kv : fields) {
			m.append(kv.first);
			m.push_back('=');
			m.append(kv.second);
			m.push_back(SOH);
		}
		return m;
	}

	template <typename Clock = std::chrono::system_clock>
	struct TimeBounds { typename Clock::time_point t0, t1; };

	template <typename Clock = std::chrono::system_clock>
	::testing::AssertionResult IsWithin(const typename Clock::time_point& tp, const TimeBounds<Clock>& b) {
		if (tp < b.t0) return ::testing::AssertionFailure() << "timestamp < t0";
		if (tp > b.t1) return ::testing::AssertionFailure() << "timestamp > t1";
		return ::testing::AssertionSuccess();
	}

} // namespace

// ========================= Happy paths (lenient) =========================

TEST(FIX_LenientParser, Parses_W_Snapshot_FirstEntry_Ask) {
	const auto header = fix_msg({ {"8","FIX.4.4"}, {"55","BTC-EUR"}, {"35","W"}, {"268","1"} });
	const auto body =
			std::string("269=1") + SOH + "270=25000.25" + SOH + "271=0.75" + SOH;

	const auto msg = header + body;

	auto t0 = std::chrono::system_clock::now();
	auto q  = parseAndStoreQuote(msg);
	auto t1 = std::chrono::system_clock::now();

	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(q->getPrice(), 25000.25);
	EXPECT_DOUBLE_EQ(q->getSize(), 0.75);
	EXPECT_EQ(q->getSymbol(), std::string_view("BTC-EUR"));
	EXPECT_TRUE(IsWithin(q->getTimestamp(), TimeBounds<>{t0,t1}));
}

TEST(FIX_LenientParser, Parses_X_Incremental_FirstEntry_Bid) {
	const auto msg = fix_msg({ {"8","FIX.4.4"}, {"35","X"}, {"55","ETH-EUR"}, {"268","2"} })
					 // Entry 1 (Bid) — parser returned hier al
					 + "269=0" + std::string(1,SOH) + "270=1999.95" + std::string(1,SOH) + "271=3.25" + std::string(1,SOH)
					 // Entry 2 (maakt niet uit, wordt niet bereikt)
					 + "269=1" + std::string(1,SOH) + "270=2000.50" + std::string(1,SOH) + "271=1.00" + std::string(1,SOH);

	auto q = parseAndStoreQuote(msg);
	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Bid);
	EXPECT_DOUBLE_EQ(q->getPrice(), 1999.95);
	EXPECT_DOUBLE_EQ(q->getSize(), 3.25);
	EXPECT_EQ(q->getSymbol(), std::string_view("ETH-EUR"));
}

TEST(FIX_LenientParser, NonBookType_YieldsNullopt) {
	const auto msg = fix_msg({ {"8","FIX.4.4"}, {"35","D"}, {"55","BTC-EUR"}, {"268","1"} })
					 + "269=0" + std::string(1,SOH) + "270=1" + std::string(1,SOH) + "271=1" + std::string(1,SOH);

	auto q = parseAndStoreQuote(msg);
	EXPECT_FALSE(q.has_value());
}

// ========================= Lenient gedrag expliciet vastgelegd =========================

TEST(FIX_LenientParser, SideOtherThanZeroMapsToAsk_ByContract) {
	// 269=2 → Ask volgens huidige implementatie (alles != "0")
	const auto msg = fix_msg({ {"8","FIX.4.4"}, {"35","X"}, {"55","BTC-EUR"}, {"268","1"} })
					 + "269=2" + std::string(1,SOH) + "270=101.0" + std::string(1,SOH) + "271=1.0" + std::string(1,SOH);

	auto q = parseAndStoreQuote(msg);
	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(q->getPrice(), 101.0);
	EXPECT_DOUBLE_EQ(q->getSize(), 1.0);
}

TEST(FIX_LenientParser, StodAcceptsTrailingGarbageInPriceAndSize) {
	// std::stod parse’t de numerieke prefix en negeert trailing garbage (geen throw)
	const auto msg = fix_msg({ {"8","FIX.4.4"}, {"35","W"}, {"55","BTC-EUR"}, {"268","1"} })
					 + "269=0" + std::string(1,SOH) + "270=101a.0" + std::string(1,SOH) + "271=1.0zzz" + std::string(1,SOH);

	auto q = parseAndStoreQuote(msg);
	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Bid);
	EXPECT_DOUBLE_EQ(q->getPrice(), 101.0);   // ← prefix parsed
	EXPECT_DOUBLE_EQ(q->getSize(), 1.0);      // ← prefix parsed
}


TEST(FIX_LenientParser, HeaderOrderIrrelevantAndExtraFieldsIgnored) {
	// Volgorde gehusseld, extra velden toegevoegd; parser bouwt map en pakt alleen 35/55/268
	const auto header = fix_msg({
										{"8","FIX.4.4"},
										{"112","foo"},   // extra
										{"268","1"},
										{"9","123"},     // extra
										{"55","XRP-EUR"},
										{"35","X"}
								});
	const auto body =
			std::string("269=1") + SOH + "270=0.555" + SOH + "271=1000" + SOH + "10=999" + SOH; // checksum extra

	const auto msg = header + body;

	auto q = parseAndStoreQuote(msg);
	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(q->getPrice(), 0.555);
	EXPECT_DOUBLE_EQ(q->getSize(), 1000.0);
	EXPECT_EQ(q->getSymbol(), std::string_view("XRP-EUR"));
}


TEST(FIX_LenientParser, AlwaysReturnsFirstEntryEvenIfSecondIsBetter) {
	const auto msg = fix_msg({ {"8","FIX.4.4"}, {"35","X"}, {"55","BTC-EUR"}, {"268","2"} })
					 + "269=1" + std::string(1,SOH) + "270=30000.0" + std::string(1,SOH) + "271=0.10" + std::string(1,SOH)  // ← returned
					 + "269=0" + std::string(1,SOH) + "270=29999.0" + std::string(1,SOH) + "271=1.00" + std::string(1,SOH);

	auto q = parseAndStoreQuote(msg);
	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(q->getPrice(), 30000.0);
	EXPECT_DOUBLE_EQ(q->getSize(), 0.10);
}


TEST(FIX_LenientParser, TimestampIsNowInterval) {
	const auto msg = fix_msg({ {"8","FIX.4.4"}, {"35","W"}, {"55","BTC-EUR"}, {"268","1"} })
					 + "269=0" + std::string(1,SOH) + "270=1" + std::string(1,SOH) + "271=1" + std::string(1,SOH);

	auto t0 = std::chrono::system_clock::now();
	auto q  = parseAndStoreQuote(msg);
	auto t1 = std::chrono::system_clock::now();

	ASSERT_TRUE(q.has_value());
	EXPECT_TRUE(IsWithin(q->getTimestamp(), TimeBounds<>{t0,t1}));
}
