
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
bool NetworkClientBase<Derived>::connect(std::string_view host, std::string_view port) {
    try {
        boost::asio::ip::tcp::resolver resolver(io_service_);
        auto endpoints = resolver.resolve(
            std::string(host), 
            std::string(port)
        );

        boost::asio::connect(socket_, endpoints);
        
        running_ = true;
        io_thread_ = std::thread([this]() { runIoService(); });
        
        startReceive();
        return true;
    }
    catch (const boost::system::system_error& e) {
        static_cast<Derived*>(this)->handleError(e.what());
        return false;
    }
}

template<typename Derived>
void NetworkClientBase<Derived>::disconnect() {
	if(running_) {
		running_ = false;
		if(socket_.is_open()) {
			boost::system::error_code ec;
			socket_.close(ec);
		}
		if(io_thread_.joinable()) {
			io_thread_.join();
		}
	}
}

	template<typename Derived>
	bool NetworkClientBase<Derived>::send(const std::string_view& message) {
		try {
			socket_.async_send(message);
			return true;
		} catch (const boost::system::system_error& e) {
			if (auto derived = static_cast<Derived*>(this)) {
				derived->handleError("Send failed: " + std::string(e.what()));
			}
			return false;
		}
	}

	template<typename Derived>
	void NetworkClientBase<Derived>::startReceive() {
		std::cout<< "Start Receive called" << std::endl;
		//TODO: not implemented
	}

	template<typename Derived>
	void NetworkClientBase<Derived>::handleReceive(const boost::system::error_code& error, std::size_t bytes) {
		std::cout<< "Handle Receive called" << std::endl;
		//TODO: not implemented
	}

	template<typename Derived>
	void NetworkClientBase<Derived>::runIoService() {
		std::cout<< "run Io service called" << std::endl;
		//TODO: not implemented
	}
} // namespace gateway
