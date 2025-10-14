#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>

#include "../../GatewayIn/include/QuotesObtainer.hpp"
#include "../../GatewayIn/include/Quote.hpp"
#include "../../OrderBook/include/OrderBook.hpp"
#include "../../OrderBook/include/QuoteConsumer.hpp"
#include "websocket/MockBitVavoClient.hpp"

using namespace testing;
using namespace gateway;
using namespace std::chrono_literals;


TEST(QuoteConsumer_EndToEnd, BitvavoBidAndAskFlow) {
    MockBitvavoClient mock;
    MockBitvavoClient::MessageHandler onMsg;
    EXPECT_CALL(mock, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsg));
    EXPECT_CALL(mock, connect(_, _)).WillOnce(Return(true));

    QuotesObtainer<MockBitvavoClient> obt(std::move(mock),
                                          "wss.bitvavo.com", "443", "BTC-EUR");
    ASSERT_TRUE(obt.connect());

    QuoteConsumer consumer{ std::tie(obt), "BTC-EUR"};
    consumer.start();

    onMsg(R"({"event":"book","bids":[["10420.00","0.75"]]})");
    onMsg(R"({"event":"book","asks":[["10425.00","1.00"]]})");

    std::this_thread::sleep_for(3ms);

    const auto& book = consumer.getOrderBook();
    EXPECT_DOUBLE_EQ(book.bestBid(), 10420.00);
    EXPECT_DOUBLE_EQ(book.bestAsk(), 10425.00);

    consumer.stop();
}

TEST(QuoteConsumer_EndToEnd, HandlesEmptyFeedsGracefully) {
	MockBitvavoClient mock;
	MockBitvavoClient::MessageHandler onMsg;
	EXPECT_CALL(mock, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, connect(_, _)).WillOnce(Return(true));

	QuotesObtainer<MockBitvavoClient> obt(std::move(mock),
										  "wss.bitvavo.com", "443", "BTC-EUR");
	ASSERT_TRUE(obt.connect());

	QuoteConsumer consumer{ std::tie(obt), "BTC-EUR" };
	consumer.start();
	std::this_thread::sleep_for(1ms);
	consumer.stop();

	EXPECT_EQ(consumer.getOrderBook().bestBid(), 0.0);
	EXPECT_EQ(consumer.getOrderBook().bestAsk(), 0.0);
}

TEST(QuoteConsumer_Stress, DuplicateQuoteLevelsAcrossFeeds) {
	MockBitvavoClient mockA, mockB;
	MockBitvavoClient::MessageHandler onMsgA, onMsgB;

	EXPECT_CALL(mockA, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsgA));
	EXPECT_CALL(mockB, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsgB));
	EXPECT_CALL(mockA, connect(_, _)).WillOnce(Return(true));
	EXPECT_CALL(mockB, connect(_, _)).WillOnce(Return(true));

	QuotesObtainer<MockBitvavoClient> feedA(std::move(mockA), "wss.bitvavo.com", "443", "BTC-EUR");
	QuotesObtainer<MockBitvavoClient> feedB(std::move(mockB), "wss.bitvavo.com", "443", "BTC-EUR");
	ASSERT_TRUE(feedA.connect());
	ASSERT_TRUE(feedB.connect());

	QuoteConsumer consumer{ std::tie(feedA, feedB), "BTC-EUR" };
	consumer.start();

	onMsgA(R"({"event":"book","bids":[["10420.00","0.75"]]})");
	onMsgB(R"({"event":"book","bids":[["10420.00","1.00"]]})");

	std::this_thread::sleep_for(3ms);

	EXPECT_DOUBLE_EQ(consumer.getOrderBook().bestBid(), 10420.00);

	consumer.stop();
}

TEST(QuoteConsumer_Stress, RepeatedZeroSizeDeletionsSafe) {
	MockBitvavoClient mock;
	MockBitvavoClient::MessageHandler onMsg;
	EXPECT_CALL(mock, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, connect(_, _)).WillOnce(Return(true));

	QuotesObtainer<MockBitvavoClient> obt(std::move(mock),
										  "wss.bitvavo.com", "443", "BTC-EUR");
	ASSERT_TRUE(obt.connect());

	QuoteConsumer consumer{ std::tie(obt), "BTC-EUR" };
	consumer.start();

	onMsg(R"({"event":"book","bids":[["10400.00","0.5"]]})");
	std::this_thread::sleep_for(1ms);
	onMsg(R"({"event":"book","bids":[["10400.00","0.0"]]})");
	onMsg(R"({"event":"book","bids":[["10400.00","0.0"]]})");
	std::this_thread::sleep_for(2ms);

	EXPECT_DOUBLE_EQ(consumer.getOrderBook().bestBid(), 0.0);

	consumer.stop();
}

TEST(QuoteConsumer_Stress, IgnoresMalformedMessages) {
	MockBitvavoClient mock;
	MockBitvavoClient::MessageHandler onMsg;
	EXPECT_CALL(mock, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, connect(_, _)).WillOnce(Return(true));

	QuotesObtainer<MockBitvavoClient> obt(std::move(mock),
										  "wss.bitvavo.com", "443", "BTC-EUR");
	ASSERT_TRUE(obt.connect());

	QuoteConsumer consumer{ std::tie(obt), "BTC-EUR" };
	consumer.start();

	onMsg(R"({"event":"book","bids":[["bad_number","0.1"]]})");
	onMsg(R"({"event":"book","asks":"oops"})");

	std::this_thread::sleep_for(2ms);
	EXPECT_EQ(consumer.getOrderBook().bestBid(), 0.0);
	EXPECT_EQ(consumer.getOrderBook().bestAsk(), 0.0);

	consumer.stop();
}

TEST(QuoteConsumer_Stress, ProcessesThousandQuotesUnderLoad) {
	MockBitvavoClient mock;
	MockBitvavoClient::MessageHandler onMsg;

	EXPECT_CALL(mock, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, connect(_, _)).WillOnce(Return(true));
	ON_CALL(mock, setErrorHandler(_)).WillByDefault([](auto){});

	QuotesObtainer<MockBitvavoClient> obt(std::move(mock),
										  "wss.bitvavo.com", "443", "BTC-EUR");
	ASSERT_TRUE(obt.connect());

	QuoteConsumer consumer{ std::tie(obt), "BTC-EUR" };
	consumer.start();

	for (int i = 0; i < 1000; ++i) {
		double price = 10000.0 + i * 0.01;
		double size = (i % 10) * 0.1 + 0.1;
		std::ostringstream msg;
		msg << R"({"event":"book","bids":[[)"
	<< std::fixed << std::setprecision(2) << price
	<< "," << std::fixed << std::setprecision(2) << size
	<< R"(]]})";


		onMsg(msg.str());
	}

	EXPECT_GT(obt.sizeBidQueue(), 0u);

	for (int i = 0; i < 50; ++i) {
		std::cout <<"checking if bidqueue is empty" << std::endl;
		if (obt.bidQueueEmpty()) {
			std::cout << "bidqueue empty" <<std::endl;
			break;
		}

		std::this_thread::sleep_for(200us);
	}

	auto book = consumer.getOrderBook();
	EXPECT_NEAR(book.bestBid(), 10000.0 + 999 * 0.01, 1e-9);
	EXPECT_GT(book.bids().size(), 500u);

	consumer.stop();
}

TEST(QuoteConsumer_Stress, StopDuringHeavyLoadIsSafe) {
	MockBitvavoClient mock;
	MockBitvavoClient::MessageHandler onMsg;
	EXPECT_CALL(mock, setMessageHandler(_)).WillOnce(SaveArg<0>(&onMsg));
	EXPECT_CALL(mock, connect(_, _)).WillOnce(Return(true));

	QuotesObtainer<MockBitvavoClient> obt(std::move(mock),
										  "wss.bitvavo.com", "443", "BTC-EUR");
	ASSERT_TRUE(obt.connect());

	QuoteConsumer consumer{ std::tie(obt), "BTC-EUR" };
	consumer.start();

	for (int i = 0; i < 500; ++i)
		onMsg(R"({"event":"book","bids":[["101.00","0.1"]]})");

	consumer.stop();
	SUCCEED();
}



