#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <chrono>

#include "../../GatewayIn/include/QuotesObtainer.hpp"
#include "../../GatewayIn/include/Quote.hpp"

using namespace testing;
using gateway::QuotesObtainer;
using gateway::Quote;
using gateway::QuoteSide;

static constexpr char SOH = '\x01';

template <typename Q>
static std::vector<Quote> drain(Q& q) {
	std::vector<Quote> out; Quote tmp;
	while (q.pop(tmp)) out.push_back(tmp);
	return out;
}

// ---------------- Bitvavo end-to-end via connect() ----------------

TEST(QuotesObtainer, Bitvavo_EndToEnd_BidAndAsk_Queues) {
	gateway::MockBitvavoClient mock;

	gateway::MockBitvavoClient::MessageHandler onMsg;
	gateway::MockBitvavoClient::ErrorHandler   onErr;

	EXPECT_CALL(mock, setMessageHandler(_))
			.WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, setErrorHandler(_))
			.WillOnce(SaveArg<0>(&onErr));

	EXPECT_CALL(mock, connect(StrEq("wss.bitvavo.com"), StrEq("443")))
			.WillOnce(Return(true));

	EXPECT_CALL(mock, send(R"({"action":"subscribe","channels":[{"name":"book","markets":["BTC-EUR"]}]})"))
			.Times(1);

	using TestObtainer = QuotesObtainer<gateway::MockBitvavoClient>;
	TestObtainer obt(std::move(mock), "wss.bitvavo.com", "443", "BTC-EUR");

	ASSERT_TRUE(obt.connect());
	ASSERT_TRUE(static_cast<bool>(onMsg));

	// simulate bid
	onMsg(R"({"event":"book","bids":[["101.23","0.10"]]})");
	auto bids = drain(obt.getBidQueue());
	ASSERT_EQ(bids.size(), 1u);
	EXPECT_EQ(bids[0].getSide(), QuoteSide::Bid);
	EXPECT_DOUBLE_EQ(bids[0].getPrice(), 101.23);
	EXPECT_DOUBLE_EQ(bids[0].getSize(), 0.10);

	// simulate ask
	onMsg(R"({"event":"book","asks":[["101.50","0.25"]]})");
	auto asks = drain(obt.getAskQueue());
	ASSERT_EQ(asks.size(), 1u);
	EXPECT_EQ(asks[0].getSide(), QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(asks[0].getPrice(), 101.50);
	EXPECT_DOUBLE_EQ(asks[0].getSize(), 0.25);
}

// ---------------- Pix/FIX end-to-end via connect() ----------------

static std::string make_fix_md(const std::string& sym,
							   const std::string& type, // "X" or "W"
							   const std::string& side, // "0" bid / "1" ask
							   const std::string& px,
							   const std::string& sz) {
	std::string m;
	m += "8=FIX.4.4" + std::string(1, SOH);
	m += "35=" + type + std::string(1, SOH);
	m += "55=" + sym  + std::string(1, SOH);
	m += "268=1"      + std::string(1, SOH);
	m += "269=" + side + std::string(1, SOH);
	m += "270=" + px   + std::string(1, SOH);
	m += "271=" + sz   + std::string(1, SOH);
	return m;
}

TEST(QuotesObtainer, Pix_EndToEnd_Bid_Queue) {
	gateway::MockPixClient mock;

	gateway::MockPixClient::MessageHandler onMsg;
	gateway::MockPixClient::ErrorHandler   onErr;

	EXPECT_CALL(mock, setMessageHandler(_))
			.WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, setErrorHandler(_))
			.WillOnce(SaveArg<0>(&onErr));

	EXPECT_CALL(mock, connect(StrEq("10.0.0.1"), StrEq("9000")))
			.WillOnce(Return(true));
	EXPECT_CALL(mock, disconnect()).Times(AtLeast(0));

	using TestObtainer = QuotesObtainer<gateway::MockPixClient>;
	TestObtainer obt(std::move(mock), "10.0.0.1", "9000", "ETH-EUR");

	ASSERT_TRUE(obt.connect());
	ASSERT_TRUE(static_cast<bool>(onMsg));

	onMsg(make_fix_md("ETH-EUR", "X", "0", "1999.95", "3.25"));

	auto bids = drain(obt.getBidQueue());
	ASSERT_EQ(bids.size(), 1u);
	EXPECT_EQ(bids[0].getSide(), QuoteSide::Bid);
	EXPECT_DOUBLE_EQ(bids[0].getPrice(), 1999.95);
	EXPECT_DOUBLE_EQ(bids[0].getSize(), 3.25);
}

TEST(QuotesObtainer, Pix_EndToEnd_Ask_Queue) {
	gateway::MockPixClient mock;

	gateway::MockPixClient::MessageHandler onMsg;
	gateway::MockPixClient::ErrorHandler   onErr;

	EXPECT_CALL(mock, setMessageHandler(_))
			.WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, setErrorHandler(_))
			.WillOnce(SaveArg<0>(&onErr));
	EXPECT_CALL(mock, connect(StrEq("127.0.0.1"), StrEq("9999")))
			.WillOnce(Return(true));

	using TestObtainer = QuotesObtainer<gateway::MockPixClient>;
	TestObtainer obt(std::move(mock), "127.0.0.1", "9999", "BTC-EUR");

	ASSERT_TRUE(obt.connect());
	ASSERT_TRUE(static_cast<bool>(onMsg));

	onMsg(make_fix_md("BTC-EUR", "W", "1", "60250.00", "0.75"));

	auto asks = drain(obt.getAskQueue());
	ASSERT_EQ(asks.size(), 1u);
	EXPECT_EQ(asks[0].getSide(), QuoteSide::Ask);
	EXPECT_DOUBLE_EQ(asks[0].getPrice(), 60250.00);
	EXPECT_DOUBLE_EQ(asks[0].getSize(), 0.75);
}
