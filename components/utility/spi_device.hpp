#ifndef SPI_DEVICE_HPP
#define SPI_DEVICE_HPP

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "utility.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <ranges>
#include <utility>

namespace Utility {

    struct SPIDevice {
    public:
        SPIDevice() noexcept = default;
        SPIDevice(spi_device_handle_t const spi_device,
                  gpio_num_t const chip_select,
                  std::size_t const rx_dma_buffer_size = 1024UL,
                  std::size_t const tx_dma_buffer_size = 1024UL) noexcept;

        SPIDevice(SPIDevice const& other) = delete;
        SPIDevice(SPIDevice&& other) noexcept = default;

        SPIDevice& operator=(SPIDevice const& other) = delete;
        SPIDevice& operator=(SPIDevice&& other) noexcept = default;

        ~SPIDevice() noexcept = default;

        template <std::size_t SIZE>
        void transmit_bytes_dma(std::array<std::uint8_t, SIZE> const& bytes) const noexcept;
        void transmit_byte_dma(std::uint8_t const byte) const noexcept;

        template <std::size_t SIZE>
        std::array<std::uint8_t, SIZE> receive_bytes_dma() const noexcept;
        std::uint8_t receive_byte_dma() const noexcept;

        template <std::size_t SIZE>
        void transmit_bytes(std::array<std::uint8_t, SIZE> const& bytes) const noexcept;
        void transmit_byte(std::uint8_t const byte) const noexcept;

        template <std::size_t SIZE>
        std::array<std::uint8_t, SIZE> receive_bytes() const noexcept;
        std::uint8_t receive_byte() const noexcept;

        template <std::size_t SIZE>
        std::array<std::uint8_t, SIZE> read_bytes_dma(std::uint8_t const reg_address) const noexcept;
        std::uint8_t read_byte_dma(std::uint8_t const reg_address) const noexcept;

        template <std::size_t SIZE>
        void write_bytes_dma(std::uint8_t const reg_address,
                             std::array<std::uint8_t, SIZE> const& bytes) const noexcept;
        void write_byte_dma(std::uint8_t const reg_address, std::uint8_t const byte) const noexcept;

        template <std::size_t SIZE>
        std::array<std::uint8_t, SIZE> read_bytes(std::uint8_t const reg_address) const noexcept;
        std::uint8_t read_byte(std::uint8_t const reg_address) const noexcept;

        template <std::size_t SIZE>
        void write_bytes(std::uint8_t const reg_address, std::array<std::uint8_t, SIZE> const& bytes) const noexcept;
        void write_byte(std::uint8_t const reg_address, std::uint8_t const byte) const noexcept;

    private:
        static std::uint8_t reg_address_to_read_command(std::uint8_t const reg_address) noexcept;
        static std::uint8_t reg_address_to_write_command(std::uint8_t const reg_address) noexcept;

        static constexpr std::uint32_t TIMEOUT{100U};

        void initialize() noexcept;
        void deinitialize() noexcept;

        bool initialized_{false};

        std::uint8_t* rx_dma_buffer_{nullptr};
        std::size_t rx_dma_buffer_size_{};

        std::uint8_t* tx_dma_buffer_{nullptr};
        std::size_t tx_dma_buffer_size_{};

        spi_device_handle_t spi_device_{nullptr};
        gpio_num_t chip_select_{};
    };

    template <std::size_t SIZE>
    inline void SPIDevice::transmit_bytes_dma(std::array<std::uint8_t, SIZE> const& bytes) const noexcept
    {
        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = SIZE * 8;
            transaction.rxlength = 0;
            transaction.flags = 0;
            transaction.tx_buffer = this->tx_dma_buffer_;

            std::memcpy(this->tx_dma_buffer_, bytes.data(), bytes.size());

            spi_transaction_t* result;
            gpio_set_level(this->chip_select_, 0);
            spi_device_queue_trans(this->spi_device_, &transaction, TIMEOUT);
            spi_device_get_trans_result(this->spi_device_, &result, TIMEOUT);
            gpio_set_level(this->chip_select_, 1);
        }
    }

    template <std::size_t SIZE>
    inline std::array<std::uint8_t, SIZE> SPIDevice::receive_bytes_dma() const noexcept
    {
        std::array<std::uint8_t, SIZE> bytes{};

        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = 0;
            transaction.rxlength = SIZE * 8;
            transaction.flags = 0;
            transaction.rx_buffer = this->rx_dma_buffer_;

            spi_transaction_t* result;
            gpio_set_level(this->chip_select_, 0);
            spi_device_queue_trans(this->spi_device_, &transaction, TIMEOUT);
            spi_device_get_trans_result(this->spi_device_, &result, TIMEOUT);
            gpio_set_level(this->chip_select_, 1);

            std::memcpy(bytes.data(), transaction.rx_buffer, bytes.size());
        }

        return bytes;
    }

    template <std::size_t SIZE>
    void SPIDevice::transmit_bytes(std::array<std::uint8_t, SIZE> const& bytes) const noexcept
    {
        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = 8 * SIZE;
            transaction.rxlength = 0;
            transaction.flags = SPI_TRANS_USE_TXDATA;

            std::memcpy(transaction.tx_data, bytes.data(), bytes.size());

            gpio_set_level(this->chip_select_, 0);
            spi_device_polling_transmit(this->spi_device_, &transaction);
            gpio_set_level(this->chip_select_, 1);
        }
    }

    template <std::size_t SIZE>
    std::array<std::uint8_t, SIZE> SPIDevice::receive_bytes() const noexcept
    {
        std::array<std::uint8_t, SIZE> bytes{};

        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = 0;
            transaction.rxlength = 8 * SIZE;
            transaction.flags = SPI_TRANS_USE_RXDATA;

            gpio_set_level(this->chip_select_, 0);
            spi_device_polling_transmit(this->spi_device_, &transaction);
            gpio_set_level(this->chip_select_, 1);

            std::memcpy(bytes.data(), transaction.rx_data, bytes.size());
        }

        return bytes;
    }

    template <std::size_t SIZE>
    inline std::array<std::uint8_t, SIZE> SPIDevice::read_bytes_dma(std::uint8_t const reg_address) const noexcept
    {
        std::array<std::uint8_t, SIZE> bytes{};

        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = 0;
            transaction.rxlength = 8 * SIZE;
            transaction.flags = 0;
            transaction.addr = reg_address_to_read_command(reg_address);
            transaction.rx_buffer = this->rx_dma_buffer_;

            gpio_set_level(this->chip_select_, 0);
            spi_device_polling_transmit(this->spi_device_, &transaction);
            gpio_set_level(this->chip_select_, 1);

            std::memcpy(bytes.data(), transaction.rx_buffer, bytes.size());
        }

        return bytes;
    }

    template <std::size_t SIZE>
    inline void SPIDevice::write_bytes_dma(std::uint8_t const reg_address,
                                           std::array<std::uint8_t, SIZE> const& bytes) const noexcept
    {
        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = 8 * SIZE;
            transaction.rxlength = 0;
            transaction.flags = 0;
            transaction.addr = reg_address_to_write_command(reg_address);
            transaction.tx_buffer = this->tx_dma_buffer_;

            std::memcpy(this->tx_dma_buffer_, bytes.data(), bytes.size());

            gpio_set_level(this->chip_select_, 0);
            spi_device_polling_transmit(this->spi_device_, &transaction);
            gpio_set_level(this->chip_select_, 1);
        }
    }

    template <std::size_t SIZE>
    std::array<std::uint8_t, SIZE> SPIDevice::read_bytes(std::uint8_t const reg_address) const noexcept
    {
        std::array<std::uint8_t, SIZE> bytes{};

        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = 0;
            transaction.rxlength = 8 * SIZE;
            transaction.addr = reg_address_to_read_command(reg_address);
            transaction.flags = SPI_TRANS_USE_RXDATA;

            gpio_set_level(this->chip_select_, 0);
            spi_device_polling_transmit(this->spi_device_, &transaction);
            gpio_set_level(this->chip_select_, 1);

            std::memcpy(bytes.data(), transaction.rx_data, bytes.size());
        }

        return bytes;
    }

    template <std::size_t SIZE>
    void SPIDevice::write_bytes(std::uint8_t const reg_address,
                                std::array<std::uint8_t, SIZE> const& bytes) const noexcept
    {
        if (this->initialized_) {
            spi_transaction_t transaction{};
            transaction.length = 8 * SIZE;
            transaction.rxlength = 0;
            transaction.addr = reg_address_to_write_command(reg_address);
            transaction.flags = SPI_TRANS_USE_TXDATA;

            std::memcpy(transaction.tx_data, bytes.data(), bytes.size());

            gpio_set_level(this->chip_select_, 0);
            spi_device_polling_transmit(this->spi_device_, &transaction);
            gpio_set_level(this->chip_select_, 1);
        }
    }

}; // namespace Utility

#endif // SPI_DEVICE_HPP