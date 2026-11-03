#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace bluewake::dsp
{
struct HostCallbacks
{
  std::function<std::uint8_t(std::uint32_t)> read_memory;
  std::function<void(std::uint32_t, std::uint8_t)> write_memory;
  std::function<std::uint8_t(std::uint32_t)> read_aram;
  std::function<void(std::uint32_t, std::uint8_t)> write_aram;
  std::function<void(std::uint32_t, std::uint32_t)> dma_write_observer;
  std::function<void()> interrupt_observer;
};

class Adapter
{
public:
  Adapter();
  ~Adapter();

  Adapter(const Adapter&) = delete;
  Adapter& operator=(const Adapter&) = delete;

  bool initialize(const std::string& irom_path, const std::string& coef_path,
                  HostCallbacks callbacks);
  void shutdown();

  int run_cycles(int cycles);
  void write_control(std::uint16_t value);
  std::uint16_t read_control();
  void write_cpu_mailbox(std::uint32_t value);
  std::uint32_t peek_cpu_mailbox() const;
  std::uint32_t peek_dsp_mailbox() const;
  std::uint16_t read_dsp_mailbox_low();
  void write_ifx(std::uint16_t address, std::uint16_t value);
  std::uint16_t read_ifx(std::uint16_t address);

private:
  struct Impl;
  Impl* m_impl;
};
}  // namespace bluewake::dsp
