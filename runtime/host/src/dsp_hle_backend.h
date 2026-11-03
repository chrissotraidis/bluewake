#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "dsp_adapter_c.h"

namespace bluewake::dsp
{
// Dolphin's high-level DSP (see dsp_hle_backend.cpp). One instance at a time.
class HleBackend
{
public:
  HleBackend();
  ~HleBackend();
  HleBackend(const HleBackend&) = delete;
  HleBackend& operator=(const HleBackend&) = delete;

  bool initialize(void* user, BluewakeDspGuestPointerFn guest_pointer, std::uint8_t* aram,
                  std::uint32_t aram_size, BluewakeDspTimebaseFn timebase,
                  BluewakeDspInterruptFn interrupt);
  int run_cycles(int cycles);
  void write_control(std::uint16_t value);
  std::uint16_t read_control();
  void write_cpu_mailbox(std::uint32_t value);
  std::uint32_t peek_cpu_mailbox();
  std::uint32_t peek_dsp_mailbox();
  std::uint16_t read_dsp_mailbox_low();
  std::uint64_t interrupts() const;
  // Save states: Dolphin's DSPHLE::DoState (control register, ucode and its
  // mixer, mail queue) through a PointerWrap, plus this backend's latch of an
  // interrupt raised but not yet delivered. load_state is false when the blob
  // does not match what DoState reads.
  std::vector<std::uint8_t> save_state();
  bool load_state(const std::uint8_t* data, std::size_t size);

private:
  struct Impl;
  Impl* m_impl;
};
}  // namespace bluewake::dsp
