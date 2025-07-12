#pragma once

#include <boost/asio.hpp>
#include <string_view>
#include <thread>
#include <atomic>
#include <array>

namespace gateway {

template<typename Derived>
class NetworkClientBase {
public:
    NetworkClientBase();
    ~NetworkClientBase();

    NetworkClientBase(const NetworkClientBase&) = delete;
    NetworkClientBase& operator=(const NetworkClientBase&) = delete;
    NetworkClientBase(NetworkClientBase&&) noexcept = default;
    NetworkClientBase& operator=(NetworkClientBase&&) noexcept = default;

	void setConnectTimeout(std::chrono::milliseconds timeout);
	[[nodiscard]] std::chrono::milliseconds getConnectTimeout() const;
	bool connect(std::string_view host, std::string_view port);
    void disconnect();
    bool send(const std::string_view& message);

protected:
    void startReceive();
    void handleReceive(const char* data, std::size_t size);
    void runIoService();

    boost::asio::io_context io_service_;
    boost::asio::ip::tcp::socket socket_;
	std::thread receive_thread_;
    std::thread io_thread_;
    std::atomic<bool> running_{false};
	std::chrono::milliseconds connect_timeout_{5000};
    
    static constexpr std::size_t max_buffer_size = 8192;
    std::array<char, max_buffer_size> receive_buffer_{};
};

} // namespace gateway

#include "NetworkClientBase.inl"
