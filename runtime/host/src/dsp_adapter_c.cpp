#include "dsp_adapter_c.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include "dsp_adapter.h"
#include "dsp_hle_backend.h"

struct BluewakeDspAdapter
{
  bluewake::dsp::Adapter impl;
  std::unique_ptr<bluewake::dsp::HleBackend> hle;
  void* user = nullptr;
  BluewakeDspReadMemoryFn read_memory = nullptr;
  BluewakeDspWriteMemoryFn write_memory = nullptr;
  BluewakeDspReadAramFn read_aram = nullptr;
  BluewakeDspWriteAramFn write_aram = nullptr;
  BluewakeDspDmaWriteFn dma_write = nullptr;
  BluewakeDspInterruptFn interrupt = nullptr;
};

extern "C" BluewakeDspAdapter* bluewake_dsp_adapter_create(
    const char* irom_path, const char* coef_path, void* user,
    BluewakeDspReadMemoryFn read_memory,
    BluewakeDspWriteMemoryFn write_memory,
    BluewakeDspReadAramFn read_aram,
    BluewakeDspWriteAramFn write_aram,
    BluewakeDspDmaWriteFn dma_write,
    BluewakeDspInterruptFn interrupt)
{
  if (irom_path == nullptr || coef_path == nullptr || read_memory == nullptr ||
      write_memory == nullptr || read_aram == nullptr || write_aram == nullptr)
    return nullptr;

  auto adapter = std::make_unique<BluewakeDspAdapter>();
  adapter->user = user;
  adapter->read_memory = read_memory;
  adapter->write_memory = write_memory;
  adapter->read_aram = read_aram;
  adapter->write_aram = write_aram;
  adapter->dma_write = dma_write;
  adapter->interrupt = interrupt;

  bluewake::dsp::HostCallbacks callbacks;
  callbacks.read_memory = [adapter_ptr = adapter.get()](std::uint32_t address) {
    return adapter_ptr->read_memory(adapter_ptr->user, address);
  };
  callbacks.write_memory = [adapter_ptr = adapter.get()](std::uint32_t address,
                                                          std::uint8_t value) {
    adapter_ptr->write_memory(adapter_ptr->user, address, value);
  };
  callbacks.read_aram = [adapter_ptr = adapter.get()](std::uint32_t address) {
    return adapter_ptr->read_aram(adapter_ptr->user, address);
  };
  callbacks.write_aram = [adapter_ptr = adapter.get()](std::uint32_t address,
                                                        std::uint8_t value) {
    adapter_ptr->write_aram(adapter_ptr->user, address, value);
  };
  callbacks.dma_write_observer = [adapter_ptr = adapter.get()](
                                     std::uint32_t address, std::uint32_t size) {
    if (adapter_ptr->dma_write != nullptr)
      adapter_ptr->dma_write(adapter_ptr->user, address, size);
  };
  callbacks.interrupt_observer = [adapter_ptr = adapter.get()] {
    if (adapter_ptr->interrupt != nullptr)
      adapter_ptr->interrupt(adapter_ptr->user);
  };

  if (!adapter->impl.initialize(irom_path, coef_path, std::move(callbacks)))
    return nullptr;
  return adapter.release();
}

extern "C" BluewakeDspAdapter* bluewake_dsp_adapter_create_hle(
    void* user, BluewakeDspGuestPointerFn guest_pointer, uint8_t* aram,
    uint32_t aram_size, BluewakeDspTimebaseFn timebase,
    BluewakeDspInterruptFn interrupt)
{
  auto adapter = std::make_unique<BluewakeDspAdapter>();
  adapter->user = user;
  adapter->interrupt = interrupt;
  adapter->hle = std::make_unique<bluewake::dsp::HleBackend>();
  if (!adapter->hle->initialize(user, guest_pointer, aram, aram_size, timebase,
                                interrupt))
    return nullptr;
  return adapter.release();
}

extern "C" int bluewake_dsp_adapter_is_hle(const BluewakeDspAdapter* adapter)
{
  return adapter != nullptr && adapter->hle != nullptr;
}

extern "C" void bluewake_dsp_adapter_destroy(BluewakeDspAdapter* adapter)
{
  delete adapter;
}

extern "C" int bluewake_dsp_adapter_run_cycles(BluewakeDspAdapter* adapter,
                                                  int cycles)
{
  if (adapter == nullptr)
    return 0;
  return adapter->hle ? adapter->hle->run_cycles(cycles) : adapter->impl.run_cycles(cycles);
}

extern "C" void bluewake_dsp_adapter_write_control(
    BluewakeDspAdapter* adapter, uint16_t value)
{
  if (adapter != nullptr && adapter->hle)
    adapter->hle->write_control(value);
  else if (adapter != nullptr)
    adapter->impl.write_control(value);
}

extern "C" uint16_t bluewake_dsp_adapter_read_control(
    BluewakeDspAdapter* adapter)
{
  if (adapter == nullptr)
    return 0;
  return adapter->hle ? adapter->hle->read_control() : adapter->impl.read_control();
}

extern "C" void bluewake_dsp_adapter_write_cpu_mailbox(
    BluewakeDspAdapter* adapter, uint32_t value)
{
  if (adapter != nullptr && adapter->hle)
    adapter->hle->write_cpu_mailbox(value);
  else if (adapter != nullptr)
    adapter->impl.write_cpu_mailbox(value);
}

extern "C" uint32_t bluewake_dsp_adapter_peek_cpu_mailbox(
    const BluewakeDspAdapter* adapter)
{
  if (adapter == nullptr)
    return 0;
  return adapter->hle ? adapter->hle->peek_cpu_mailbox() : adapter->impl.peek_cpu_mailbox();
}

extern "C" uint32_t bluewake_dsp_adapter_peek_dsp_mailbox(
    const BluewakeDspAdapter* adapter)
{
  if (adapter == nullptr)
    return 0;
  return adapter->hle ? adapter->hle->peek_dsp_mailbox() : adapter->impl.peek_dsp_mailbox();
}

extern "C" uint16_t bluewake_dsp_adapter_read_dsp_mailbox_low(
    BluewakeDspAdapter* adapter)
{
  if (adapter == nullptr)
    return 0;
  return adapter->hle ? adapter->hle->read_dsp_mailbox_low()
                      : adapter->impl.read_dsp_mailbox_low();
}

extern "C" void bluewake_dsp_adapter_write_ifx(
    BluewakeDspAdapter* adapter, uint16_t address, uint16_t value)
{
  if (adapter != nullptr && !adapter->hle)
    adapter->impl.write_ifx(address, value);
}

extern "C" uint16_t bluewake_dsp_adapter_read_ifx(
    BluewakeDspAdapter* adapter, uint16_t address)
{
  return adapter == nullptr || adapter->hle ? 0 : adapter->impl.read_ifx(address);
}

extern "C" size_t bluewake_dsp_adapter_save_state(BluewakeDspAdapter* adapter,
                                                  uint8_t** out)
{
  if (out == nullptr)
    return 0;
  *out = nullptr;
  if (adapter == nullptr || !adapter->hle)
    return 0;
  const std::vector<std::uint8_t> state = adapter->hle->save_state();
  if (state.empty())
    return 0;
  *out = static_cast<uint8_t*>(std::malloc(state.size()));
  if (*out == nullptr)
    return 0;
  std::memcpy(*out, state.data(), state.size());
  return state.size();
}

extern "C" int bluewake_dsp_adapter_load_state(BluewakeDspAdapter* adapter,
                                               const uint8_t* data, size_t size)
{
  if (adapter == nullptr || !adapter->hle || data == nullptr)
    return 0;
  return adapter->hle->load_state(data, size) ? 1 : 0;
}
