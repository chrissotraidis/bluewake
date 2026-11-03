#include "dsp_adapter.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "Core/DSP/DSPTables.h"
#include "Core/DSP/Interpreter/DSPIntTables.h"

int main(int argc, char** argv)
{
  assert(argc == 3);
  std::vector<std::uint8_t> memory(0x200000);
  std::vector<std::uint8_t> aram(0x1000000);
  bool interrupted = false;
  bool task_source_read = false;
  std::uint32_t dma_address = 0;
  std::uint32_t dma_size = 0;

  bluewake::dsp::Adapter adapter;
  bluewake::dsp::HostCallbacks callbacks;
  callbacks.read_memory = [&](std::uint32_t address) {
    if (address == 0x80399420)
      task_source_read = true;
    const auto index = address >= 0x81000000
                           ? address - 0x81000000
                           : address >= 0x80399420 && address < 0x8039B140
                                 ? 0x10000 + address - 0x80399420
                                 : address;
    assert(index < memory.size());
    return memory[index];
  };
  callbacks.write_memory = [&](std::uint32_t address, std::uint8_t value) {
    const auto index = address >= 0x81000000 ? address - 0x81000000 : address;
    assert(index < memory.size());
    memory[index] = value;
  };
  callbacks.read_aram = [&](std::uint32_t address) {
    return aram[address & (aram.size() - 1)];
  };
  callbacks.write_aram = [&](std::uint32_t address, std::uint8_t value) {
    aram[address & (aram.size() - 1)] = value;
  };
  callbacks.dma_write_observer = [&](std::uint32_t address, std::uint32_t size) {
    dma_address = address;
    dma_size = size;
  };
  callbacks.interrupt_observer = [&] { interrupted = true; };

  assert(adapter.initialize(argv[1], argv[2], std::move(callbacks)));
  for (std::uint32_t value = 0; value <= UINT16_MAX; ++value)
  {
    const auto instruction = static_cast<DSP::UDSPInstruction>(value);
    const auto& decoded = DSP::Interpreter::GetDecodedOp(instruction);
    assert(decoded.main == DSP::Interpreter::GetOp(instruction));
    assert(decoded.extension == DSP::Interpreter::GetExtOp(instruction));
    assert(decoded.extended == DSP::GetOpTemplate(instruction)->extended);
  }
  for (std::size_t offset = 0; offset < 0x1000; offset += 2)
  {
    memory[offset] = 0x00;
    memory[offset + 1] = 0x21;
  }
  adapter.write_control(0);
  assert((adapter.read_control() & 0x0804) == 0);
  adapter.run_cycles(5000);
  assert(adapter.peek_dsp_mailbox() == 0x8071FEED);
  assert(adapter.read_dsp_mailbox_low() == 0xFEED);
  assert((adapter.peek_dsp_mailbox() & 0x80000000u) == 0);
  const std::uint32_t boot_words[] = {
      0x80F3A001, 0x80399420, 0x80F3C002, 0x00000000, 0x80F3A002,
      0x00001D20, 0x80F3B002, 0x00000000, 0x80F3D001, 0x00000000,
  };
  for (const auto word : boot_words)
  {
    adapter.write_cpu_mailbox(word);
    adapter.run_cycles(5000);
  }
  assert(task_source_read);
  assert(memory[0xd4] == 0x00 && memory[0xd5] == 0x21);
  adapter.write_cpu_mailbox(0x12345678);
  adapter.write_ifx(0xCD, 0x0123);
  assert(adapter.read_ifx(0xCD) == 0x0123);
  // DSBL bit 15 starts a DMA; it is not part of the byte length.
  adapter.write_ifx(0xCE, 0x0000);
  adapter.write_ifx(0xCF, 0x0000);
  adapter.write_ifx(0xCD, 0x0300);
  adapter.write_ifx(0xCB, 0x8046);
  assert(adapter.read_ifx(0xCB) == 0);
  adapter.write_ifx(0xD1, 0x0040);
  assert(adapter.read_ifx(0xD1) == 0x0040);
  assert(adapter.run_cycles(12) >= 0);
  assert((adapter.peek_dsp_mailbox() & 0x80000000u) == 0);
  assert(!interrupted || dma_size != 0 || dma_address != 0);
  adapter.shutdown();
}
