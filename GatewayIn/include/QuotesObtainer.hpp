#pragma once

#include <string_view>
#include <optional>
#include <chrono>
#include <vector>
#include <memory>
#include <optional>
#include <format>
#include <Quote.hpp>
#include <NetworkClientBase.hpp>

namespace gateway {
    class PixNetworkClient;
    class QuotesObtainer {
    public:
        explicit QuotesObtainer(std::unique_ptr<PixNetworkClient> networkClient);
        ~QuotesObtainer();

        bool connect(std::string_view host, std::string_view port);
        void disconnect();
        [[nodiscard]] std::optional<Quote> getCurrentQuote() const;
        void setUpdateInterval(std::chrono::milliseconds interval);

    private:
        std::unique_ptr<PixNetworkClient> pixClient_;
        void processPixMessage(std::string_view message);
        [[nodiscard]] std::vector<Quote> parsePixFormat(std::string_view data) const;

        mutable std::mutex quoteMutex_;
        std::optional<Quote> lastQuote_;
        std::chrono::milliseconds updateInterval_{1000};
    };
}
