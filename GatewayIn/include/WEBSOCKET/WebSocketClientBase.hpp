// WebSocketClientBase.hpp
#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <atomic>
#include <array>
#include <string>
#include <string_view>
#include <thread>
#include <iostream>

template<typename Derived>
class WebSocketClientBase {
public:
	WebSocketClientBase()
			: ssl_ctx_(boost::asio::ssl::context::tls_client),
			  ws_(ioc_, ssl_ctx_) {
		ssl_ctx_.set_default_verify_paths();
		ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
	}

	~WebSocketClientBase() { disconnect(); }

	WebSocketClientBase(const WebSocketClientBase&) = delete;
	WebSocketClientBase& operator=(const WebSocketClientBase&) = delete;

	void setConnectTimeout(std::chrono::milliseconds t) { connect_timeout_ = t; }
	std::chrono::milliseconds getConnectTimeout() const { return connect_timeout_; }

	// For Bitvavo target must be "/v2/"
	bool connect(std::string_view host, std::string_view port, std::string_view target) {
		host_ = std::string(host);
		target_ = std::string(target);

		try {
			boost::asio::ip::tcp::resolver resolver(ioc_);
			auto endpoints = resolver.resolve(host_, std::string(port));
			boost::asio::connect(ws_.next_layer().next_layer(), endpoints);

			// 2) TLS (SNI)
			if(!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
				std::cerr << "TLS ERROR" << std::endl;
				static_cast<Derived*>(this)->onError("SSL_set_tlsext_host_name failed");
			}
			ws_.next_layer().handshake(boost::asio::ssl::stream_base::client);

			// 3) WebSocket handshake
			ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
			ws_.handshake(host_, target_);

			running_.store(true);
			// Notify derived
			derived().onOpen();

			// Start IO/receive thread (blocking reads)
			receive_thread_ = std::thread([this] { this->receiveLoop(); });
			return true;
		} catch (const std::exception& e) {
			derived().onError(e.what());
			return false;
		}
	}

	void disconnect() {
		if (!running_.exchange(false)) return;
		try {
			ws_.close(boost::beast::websocket::close_code::normal);
		} catch (...) {}
		if (receive_thread_.joinable()) receive_thread_.join();
	}

	bool send(std::string_view text) {
		try {
			ws_.write(boost::asio::buffer(text.data(), text.size()));
			return true;
		} catch (const std::exception& e) {
			derived().onError(e.what());
			return false;
		}
	}

protected:
	void receiveLoop() {
		try {
			while (running_.load()) {
				boost::beast::flat_buffer buffer;
				ws_.read(buffer);
				auto data = buffer.data();
				std::string_view frame(static_cast<const char*>(data.data()), data.size());
				derived().onMessage(frame);
			}
		} catch (const std::exception& e) {
			if (running_.load()) derived().onError(e.what());
		}
		derived().onClosed();
	}

	Derived& derived() { return static_cast<Derived&>(*this); }

protected:
	// Expose if a Derived wants to access IO or ws directly (advanced)
	boost::asio::io_context ioc_;
	boost::asio::ssl::context ssl_ctx_;
	boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_;

	std::thread receive_thread_;
	std::atomic<bool> running_{false};
	std::chrono::milliseconds connect_timeout_{5000};

	std::string host_;
	std::string target_;
};
