#include <gtest/gtest.h>
#include <optional>
#include <string>

#include "BitvavoBookParser.hpp"
#include "../../GatewayIn/include/Quote.hpp"

using bitvavo::extractNonce;
using bitvavo::parseFirstLevel;
using bitvavo::parseAndStoreQuote;

namespace {


	template <typename Clock = std::chrono::system_clock>
	struct TimeBounds {
		typename Clock::time_point t0;
		typename Clock::time_point t1;
	};

	template <typename Clock = std::chrono::system_clock>
	::testing::AssertionResult IsWithin(const typename Clock::time_point& tp, const TimeBounds<Clock>& bounds) {
		if (tp < bounds.t0) {
			return ::testing::AssertionFailure() << "time_point earlier than t0";
		}
		if (tp > bounds.t1) {
			return ::testing::AssertionFailure() << "time_point later than t1";
		}
		return ::testing::AssertionSuccess();
	}

} // namespace


TEST(ExtractNonce, ReturnsZeroWhenFieldMissing) {
	std::string js = R"({"event":"book","data":{}})";
	EXPECT_EQ(extractNonce(js), 0u);
}

TEST(ExtractNonce, ParsesSimpleNumber) {
	std::string js = R"({"event":"book","nonce":12345,"data":{}})";
	EXPECT_EQ(extractNonce(js), 12345u);
}

TEST(ExtractNonce, StopsAtFirstNonDigit) {
	std::string js = R"({"nonce":123x,"event":"book"})";
	EXPECT_EQ(extractNonce(js), 123u);
}

TEST(ExtractNonce, HandlesLeadingZeros) {
	std::string js = R"({"nonce":0000123})";
	EXPECT_EQ(extractNonce(js), 123u);
}

TEST(ExtractNonce, WorksWhenNonceAtEnd) {
	std::string js = R"({"event":"book","data":{}})";
	js += R"(,"nonce":987654321)";
	EXPECT_EQ(extractNonce(js), 987654321u);
}


TEST(ParseFirstLevel, ParsesFirstBidLevel) {
	std::string frame = R"({"event":"book","bids":[["123.45","0.10"]],"asks":[["200.00","1.5"]]})";
	double px = -1, qty = -1;
	ASSERT_TRUE(parseFirstLevel(frame, "bids", px, qty));
	EXPECT_DOUBLE_EQ(px, 123.45);
	EXPECT_DOUBLE_EQ(qty, 0.10);
}

TEST(ParseFirstLevel, ParsesFirstAskLevel) {
	std::string frame = R"({"event":"book","bids":[["123.45","0.10"]],"asks":[["200.00","1.5"]]})";
	double px = -1, qty = -1;
	ASSERT_TRUE(parseFirstLevel(frame, "asks", px, qty));
	EXPECT_DOUBLE_EQ(px, 200.00);
	EXPECT_DOUBLE_EQ(qty, 1.5);
}

TEST(ParseFirstLevel, ReturnsFalseWhenKeyMissing) {
	std::string frame = R"({"event":"book"})";
	double px = 0, qty = 0;
	EXPECT_FALSE(parseFirstLevel(frame, "bids", px, qty));
	EXPECT_FALSE(parseFirstLevel(frame, "asks", px, qty));
}

TEST(ParseFirstLevel, IgnoresAdditionalLevelsAndTakesFirst) {
	std::string frame = R"({"event":"book","bids":[["101.0","2.0"],["99.0","5.0"]]})";
	double px = -1, qty = -1;
	ASSERT_TRUE(parseFirstLevel(frame, "bids", px, qty));
	EXPECT_DOUBLE_EQ(px, 101.0);
	EXPECT_DOUBLE_EQ(qty, 2.0);
}

TEST(ParseFirstLevel, ReturnsFalseOnMalformedStructure) {
	std::string frame = R"({"event":"book","bids":["101.0","2.0"]})"; // wrong: not [ [ "p","q" ] ]
	double px = 0, qty = 0;
	EXPECT_FALSE(parseFirstLevel(frame, "bids", px, qty));
}

TEST(ParseFirstLevel, FailsIfWhitespaceInsertedInPatternCriticalSpots) {
	std::string frame = R"({"event":"book","bids": [["101.0","2.0"]]})"; // space after colon
	double px = 0, qty = 0;
	EXPECT_FALSE(parseFirstLevel(frame, "bids", px, qty));
}

TEST(ParseFirstLevel, ParsesZeroQuantitiesAndPrices) {
	std::string frame = R"({"event":"book","bids":[["0","0"]],"asks":[["0","0"]]})";
	double px = -1, qty = -1;
	ASSERT_TRUE(parseFirstLevel(frame, "bids", px, qty));
	EXPECT_DOUBLE_EQ(px, 0.0);
	EXPECT_DOUBLE_EQ(qty, 0.0);

	px = -1; qty = -1;
	ASSERT_TRUE(parseFirstLevel(frame, "asks", px, qty));
	EXPECT_DOUBLE_EQ(px, 0.0);
	EXPECT_DOUBLE_EQ(qty, 0.0);
}


TEST(ParseAndStoreQuote, IgnoresNonBookEvents) {
	std::string frame = R"({"event":"trade","bids":[["101.0","2.0"]]})";
	auto q = parseAndStoreQuote(frame, "BTC-EUR");
	EXPECT_FALSE(q.has_value());
}

TEST(ParseAndStoreQuote, ReturnsBidWhenBothSidesPresent) {
	std::string frame = R"({"event":"book","bids":[["101.25","3.5"]],"asks":[["102.0","1.0"]]})";
	auto t0 = std::chrono::system_clock::now();
	auto q = parseAndStoreQuote(frame, "BTC-EUR");
	auto t1 = std::chrono::system_clock::now();

	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Bid);
	EXPECT_DOUBLE_EQ(q->getPrice(), 101.25);
	EXPECT_DOUBLE_EQ(q->getSize(), 3.5);
	EXPECT_EQ(q->getSymbol(), std::string("BTC-EUR"));
	EXPECT_TRUE(IsWithin(q->getTimestamp(), TimeBounds<>{t0, t1}));
}

TEST(ParseAndStoreQuote, ReturnsAskWhenNoBids) {
	std::string frame = R"({"event":"book","asks":[["99.9","10.0"]]})";
	auto t0 = std::chrono::system_clock::now();
	auto q = parseAndStoreQuote(frame, "ETH-EUR");
	auto t1 = std::chrono::system_clock::now();

	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(q->getPrice(), 99.9);
	EXPECT_DOUBLE_EQ(q->getSize(), 10.0);
	EXPECT_EQ(q->getSymbol(), std::string("ETH-EUR"));
	EXPECT_TRUE(IsWithin(q->getTimestamp(), TimeBounds<>{t0, t1}));
}

TEST(ParseAndStoreQuote, ReturnsNulloptWhenNoLevels) {
	std::string frame = R"({"event":"book","bids":[],"asks":[]})";
	auto q = parseAndStoreQuote(frame, "BTC-EUR");
	EXPECT_FALSE(q.has_value());
}

TEST(ParseAndStoreQuote, ReturnsNulloptOnMalformedButCaught) {

	std::string frame = R"({"event":"book","bids":[["101a.0","1.0"]]})";
	auto q = parseAndStoreQuote(frame, "BTC-EUR");
	EXPECT_FALSE(q.has_value());
}

TEST(ParseAndStoreQuote, TiesToFirstBidLevelOnlyWhenMultiple) {
	std::string frame = R"({"event":"book","bids":[["111.0","1.0"],["222.0","2.0"]],"asks":[["333.0","3.0"]]})";
	auto q = parseAndStoreQuote(frame, "BTC-EUR");
	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Bid);
	EXPECT_DOUBLE_EQ(q->getPrice(), 111.0);
	EXPECT_DOUBLE_EQ(q->getSize(), 1.0);
}

TEST(ParseAndStoreQuote, HandlesZeroValues) {
	std::string frame = R"({"event":"book","asks":[["0","0"]]})";
	auto q = parseAndStoreQuote(frame, "BTC-EUR");
	ASSERT_TRUE(q.has_value());
	EXPECT_EQ(q->getSide(), gateway::QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(q->getPrice(), 0.0);
	EXPECT_DOUBLE_EQ(q->getSize(), 0.0);
}

TEST(ParseFirstLevel, RejectsMalformedPriceWithTrailingChars) {
	std::string f = R"({"event":"book","bids":[["101a.0","1.0"]]})";
	double px = 0, qty = 0;
	EXPECT_FALSE(parseFirstLevel(f, "bids", px, qty));
}

TEST(ParseFirstLevel, RejectsMalformedQtyWithLeadingChars) {
	std::string f = R"({"event":"book","asks":[["101.0","a1.0"]]})";
	double px = 0, qty = 0;
	EXPECT_FALSE(parseFirstLevel(f, "asks", px, qty));
}

