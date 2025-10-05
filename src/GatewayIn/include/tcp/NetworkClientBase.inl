
#pragma once
#include <iostream>

namespace gateway {

	template<typename Derived>
	NetworkClientBase<Derived>::NetworkClientBase() : socket_(io_service_) {}

	template<typename Derived>
	NetworkClientBase<Derived>::~NetworkClientBase() {
		disconnect();
	}

	template<typename Derived>
	NetworkClientBase<Derived>::NetworkClientBase(NetworkClientBase&& other) noexcept
			: io_service_(),
			  socket_(io_service_),
			  receive_thread_(),
			  io_thread_(),
			  running_(false),
			  connect_timeout_(other.connect_timeout_),
			  receive_buffer_(other.receive_buffer_)
	{
		bool was_running = other.running_.exchange(false);

		try {
			if (was_running && other.socket_.is_open()) {
				boost::system::error_code ec;
				other.socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				other.socket_.close(ec);
			}
		} catch (...) {}

		if (other.receive_thread_.joinable()) {
			try { other.receive_thread_.join(); } catch (...) {}
		}
		if (other.io_thread_.joinable()) {
			try { other.io_thread_.join(); } catch (...) {}
		}

		other.receive_thread_ = std::thread();
		other.io_thread_      = std::thread();
	}

	template<typename Derived>
	bool NetworkClientBase<Derived>::connect(std::string_view host, std::string_view port) {
		try {
			boost::asio::ip::tcp::resolver resolver(io_service_);
			auto endpoints = resolver.resolve(host, port);

			boost::system::error_code ec;
			boost::asio::connect(socket_, endpoints, ec);

			if (ec) {
				std::cerr << "Connection failed: " << ec.message() << std::endl;
				return false;
			}

			socket_.non_blocking(true);
			running_ = true;

			receive_thread_ = std::thread([this]() {
				static_cast<Derived*>(this)->startReceive();
			});


			return true;
		} catch (const std::exception& e) {
			std::cerr << "Exception during connect: " << e.what() << std::endl;
			return false;
		}
	}

	template<typename Derived>
	void NetworkClientBase<Derived>::disconnect() {
		std::cout << "Disconnect called" << std::endl;
		running_ = false;

		if (socket_.is_open()) {
			boost::system::error_code ec;
			socket_.close(ec);
		}

		if (receive_thread_.joinable()) {
			if (std::this_thread::get_id() != receive_thread_.get_id()) {
				std::cout << "Joining receive_thread_" << std::endl;
				receive_thread_.join();
			} else {
				std::cout << "Can't join self — detaching receive_thread_" << std::endl;
				receive_thread_.detach();
			}
			receive_thread_ = std::thread();
		}
		static_cast<Derived*>(this)->onDisconnect();
	}

	template<typename Derived>
	bool NetworkClientBase<Derived>::send(const std::string_view& message) {
		try {
			std::size_t bytes_sent = boost::asio::write(
					socket_,
					boost::asio::buffer(message.data(), message.size())
			);

			return bytes_sent == message.size();
		} catch (const boost::system::system_error& e) {
			static_cast<Derived*>(this)->handleError("Send failed: " + std::string(e.what()));
			return false;
		}
	}

	template<typename Derived>
	void NetworkClientBase<Derived>::startReceive() {
		boost::system::error_code ec;

		static_cast<Derived*>(this)->onConnectionReady();

		while (running_) {
			std::size_t n = 0;
			try {
				n = socket_.read_some(boost::asio::buffer(receive_buffer_), ec);
			} catch (const std::exception& e) {
				static_cast<Derived*>(this)->handleError(std::string("Exception in read_some: ") + e.what());
				break;
			}

			if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again) {
				continue;
			}

			if (ec) {
				static_cast<Derived*>(this)->handleError("Socket read error: " + ec.message());
				break;
			}

			static_cast<Derived*>(this)->handleReceive(receive_buffer_.data(), n);
		}
	}


	template<typename Derived>
	void NetworkClientBase<Derived>::setConnectTimeout(std::chrono::milliseconds timeout) {
		connect_timeout_ = timeout;
	}

	template<typename Derived>
	std::chrono::milliseconds NetworkClientBase<Derived>::getConnectTimeout() const {
		return connect_timeout_;
	}

} // namespace gateway
