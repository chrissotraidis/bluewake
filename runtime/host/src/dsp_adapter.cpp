#include "dsp_adapter.h"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <utility>
#include <vector>

#include "Core/DSP/DSPCore.h"
#include "Core/DSP/DSPDisassembler.h"
#include "Core/DSP/DSPHost.h"
#include "Core/DSP/DSPTables.h"
#include "Core/DSP/Interpreter/DSPInterpreter.h"

namespace
{
struct ActiveHost
{
  bluewake::dsp::HostCallbacks callbacks;
};

ActiveHost* g_active_host = nullptr;
unsigned g_trace_reports = 0;
unsigned g_run_reports = 0;
unsigned g_mailbox_reports = 0;
unsigned g_mailbox_state_reports = 0;
unsigned g_external_reports = 0;
unsigned g_mail_read_reports = 0;
unsigned g_task_pc_reports = 0;
unsigned g_dma_ifx_reports = 0;
unsigned g_dsp_dma_state_reports = 0;
unsigned g_frame_trace_reports = 0;
unsigned g_task_dma_reports = 0;
unsigned g_frame_state_reports = 0;
unsigned g_task_loop_reports = 0;
unsigned g_frame_task_loop_reports = 0;
unsigned g_queue_cursor_reports = 0;
unsigned g_task_disasm_reports = 0;
bool g_task_message_trace = false;
bool g_trace_enabled = false;
std::uint16_t g_last_queue_write = 0;
std::uint16_t g_last_queue_read = 0;
std::uint16_t g_last_queue_count = 0;
unsigned g_rom_pc_reports = 0;
std::uint16_t g_last_rom_pc = 0;

// BLUEWAKE_DSP_BATCH. Stepping the DSP eight cycles at a time through
// Interpreter::Step() (run_exact_eight_cycles below) never skips a cycle and so
// never reaches the donor's idle skip. That is why the LLE slice costs about
// 13% of a play-scene frame while the microcode has only ~800 samples to
// produce per retrace out of 1.35M cycles: the rest is its wait loop. Routing a
// larger batch through core.RunCycles lets the donor's idle check engage, and
// when it fires the rest of the batch is abandoned but still counted as
// elapsed - which is the whole gain, so it scales with the batch.
//
// 128 is the shipping value under decision D2 (scripts/route_digest.py). On the
// full 14,100-retrace route it moves wall/guest from 254.45 s / 235.00 s (92.4%
// of real speed) to 186.86 s / 235.00 s (125.8%), with the boot phase at 1.76x.
// Four settings that engage the skip (16, 32, 128, 256) produce one identical
// route digest once the host's turn-boundary quantities are gated as bounds
// rather than equalities; 128 is the conservative choice of those and 256 is
// reachable by export for 155.6%. Export 8 to restore the exact, no-skip cadence
// - every guest-state record is the same under both, and the only measured
// difference is a one-cycle delivery drift on 125 of 1,024 deliveries and two
// cycles of route clock out of 114,210,000,002.
int g_batch_limit = 128;

bool trace_enabled()
{
  return g_trace_enabled;
}

void trace(const char* format, ...)
{
  if (!trace_enabled() || g_trace_reports >= 256)
    return;
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
  ++g_trace_reports;
}

std::vector<std::uint8_t> read_file(const std::string& path)
{
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    return {};

  const auto size = file.tellg();
  if (size < 0)
    return {};

  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  file.seekg(0);
  file.read(reinterpret_cast<char*>(bytes.data()), size);
  return file ? bytes : std::vector<std::uint8_t>{};
}

std::uint16_t read_be16(const ActiveHost& host, std::uint32_t address)
{
  return static_cast<std::uint16_t>(host.callbacks.read_memory(address) << 8) |
         host.callbacks.read_memory(address + 1);
}

void write_be16(const ActiveHost& host, std::uint32_t address, std::uint16_t value)
{
  host.callbacks.write_memory(address, static_cast<std::uint8_t>(value >> 8));
  host.callbacks.write_memory(address + 1, static_cast<std::uint8_t>(value));
}
}  // namespace

namespace DSP::Host
{
u8 ReadHostMemory(u32 address)
{
  return g_active_host->callbacks.read_aram(address);
}

void WriteHostMemory(u8 value, u32 address)
{
  g_active_host->callbacks.write_aram(address, value);
}

void DMAToDSP(u16* dst, u32 address, u32 size)
{
  if (trace_enabled() && address == 0x80399420u && g_task_dma_reports < 4) {
    std::fprintf(stderr,
                 "[dsp-lle-trace] task-dma-to-iram address=0x%08X size=%u "
                 "words=%04X,%04X,%04X,%04X\n",
                 address, size, read_be16(*g_active_host, address),
                 read_be16(*g_active_host, address + 2),
                 read_be16(*g_active_host, address + 4),
                 read_be16(*g_active_host, address + 6));
    ++g_task_dma_reports;
  }
  for (u32 offset = 0; offset < size; offset += 2)
    dst[offset / 2] = read_be16(*g_active_host, address + offset);
}

void DMAFromDSP(const u16* src, u32 address, u32 size)
{
  for (u32 offset = 0; offset < size; offset += 2)
    write_be16(*g_active_host, address + offset, src[offset / 2]);

  if (g_active_host->callbacks.dma_write_observer)
    g_active_host->callbacks.dma_write_observer(address, size);
}

void OSD_AddMessage(std::string, u32)
{
}

bool OnThread()
{
  return false;
}

bool IsWiiHost()
{
  return false;
}

void InterruptRequest()
{
  if (g_active_host->callbacks.interrupt_observer)
    g_active_host->callbacks.interrupt_observer();
}

void CodeLoaded(DSPCore& dsp, u32, size_t)
{
  // Reset analyzes the bootstrap ROM while IRAM is still blank. Retail
  // DspBoot then replaces IRAM with jdsp through DMA; refresh loop metadata
  // so authentic BLOOP instructions receive interpreter loop handling.
  dsp.DSPState().GetAnalyzer().Analyze(dsp.DSPState());
}

void CodeLoaded(DSPCore& dsp, const u8*, size_t)
{
  dsp.DSPState().GetAnalyzer().Analyze(dsp.DSPState());
}
}  // namespace DSP::Host

namespace bluewake::dsp
{
struct Adapter::Impl
{
  DSP::DSPCore core;
  ActiveHost host;
  bool initialized = false;
  bool program_loaded = false;

  void run_exact_eight_cycles()
  {
    auto& state = core.DSPState();
    auto& interpreter = core.GetInterpreter();
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
    for (int cycle = 0; cycle < 8; ++cycle) {
      if ((state.control_reg & DSP::CR_HALT) != 0)
        return;
      interpreter.Step();
    }
  }

  void deliver_pending_external_interrupt()
  {
    if ((core.DSPState().control_reg & DSP::CR_EXTERNAL_INT) == 0)
      return;

    // The donor clears CR_EXTERNAL_INT only after the DSP-side mask accepts
    // the request. Retry the hardware-visible pending bit before execution.
    core.CheckExternalInterrupt();
    core.CheckExceptions();
  }
};

Adapter::Adapter() : m_impl(new Impl)
{
}

Adapter::~Adapter()
{
  shutdown();
  delete m_impl;
}

bool Adapter::initialize(const std::string& irom_path, const std::string& coef_path,
                         HostCallbacks callbacks)
{
  shutdown();
  g_trace_enabled = std::getenv("BLUEWAKE_TRACE_DSP_LLE") != nullptr;
  {
    const char* batch_env = std::getenv("BLUEWAKE_DSP_BATCH");
    if (batch_env != nullptr) {
      char* batch_end = nullptr;
      const long requested = std::strtol(batch_env, &batch_end, 10);
      if (batch_end != batch_env && *batch_end == '\0' && requested > 0 &&
          requested <= 4096) {
        g_batch_limit = (int)requested;
        std::fprintf(stderr, "[dsp-lle] batch limit=%d\n", g_batch_limit);
      } else {
        std::fprintf(stderr, "[dsp-lle] ignoring BLUEWAKE_DSP_BATCH=\"%s\"\n",
                     batch_env);
      }
    }
  }
  g_trace_reports = 0;
  g_run_reports = 0;
  g_mailbox_reports = 0;
  g_mailbox_state_reports = 0;
  g_external_reports = 0;
  g_mail_read_reports = 0;
  g_task_pc_reports = 0;
  g_dsp_dma_state_reports = 0;
  g_frame_trace_reports = 0;
  g_task_dma_reports = 0;
  g_frame_state_reports = 0;
  g_task_loop_reports = 0;
  g_frame_task_loop_reports = 0;
  g_queue_cursor_reports = 0;
  g_task_disasm_reports = 0;
  g_task_message_trace = false;
  g_last_queue_write = 0;
  g_last_queue_read = 0;
  g_last_queue_count = 0;
  g_rom_pc_reports = 0;
  g_last_rom_pc = 0;
  m_impl->program_loaded = false;
  m_impl->host.callbacks = std::move(callbacks);
  if (!m_impl->host.callbacks.read_memory || !m_impl->host.callbacks.write_memory ||
      !m_impl->host.callbacks.read_aram || !m_impl->host.callbacks.write_aram)
    return false;

  const auto irom = read_file(irom_path);
  const auto coef = read_file(coef_path);
  if (irom.size() != DSP::DSP_IROM_BYTE_SIZE || coef.size() != DSP::DSP_COEF_BYTE_SIZE)
    return false;

  DSP::DSPInitOptions options;
  for (std::size_t i = 0; i < options.irom_contents.size(); ++i)
    options.irom_contents[i] = static_cast<std::uint16_t>(irom[i * 2] << 8 | irom[i * 2 + 1]);
  for (std::size_t i = 0; i < options.coef_contents.size(); ++i)
    options.coef_contents[i] = static_cast<std::uint16_t>(coef[i * 2] << 8 | coef[i * 2 + 1]);
  options.core_type = DSP::DSPInitOptions::CoreType::Interpreter;

  DSP::InitInstructionTable();
  g_active_host = &m_impl->host;
  m_impl->initialized = m_impl->core.Initialize(options);
  if (m_impl->initialized)
    m_impl->core.Reset();
  else
    g_active_host = nullptr;
  return m_impl->initialized;
}

void Adapter::shutdown()
{
  if (!m_impl->initialized)
    return;
  m_impl->core.Shutdown();
  m_impl->initialized = false;
  m_impl->program_loaded = false;
  g_active_host = nullptr;
}

int Adapter::run_cycles(int cycles)
{
  if (!m_impl->initialized || !m_impl->program_loaded)
    return 0;
  if (trace_enabled() && g_run_reports < 8) {
    trace("[dsp-lle-trace] run cycles=%d pc=0x%04X control=0x%04X\n", cycles,
          m_impl->core.DSPState().pc,
          m_impl->core.GetInterpreter().ReadControlRegister());
    ++g_run_reports;
  }
  int remaining = cycles;
  while (remaining > 0) {
    m_impl->deliver_pending_external_interrupt();
    if (!trace_enabled()) {
      const auto opcode = m_impl->core.DSPState().PeekInstruction();
      if (DSP::GetOpTemplate(opcode) == nullptr) {
        std::fprintf(stderr,
                     "[dsp-lle] invalid DSP opcode pc=0x%04X opcode=0x%04X\n",
                     m_impl->core.DSPState().pc, opcode);
        m_impl->program_loaded = false;
        break;
      }
      // Dolphin executes the first eight cycles without idle skipping. Keep
      // batches within that prefix so batching cannot change DSP cadence.
      const int batch = std::min(remaining, g_batch_limit);
      if (g_batch_limit == 8 && batch == 8)
        m_impl->run_exact_eight_cycles();
      else
        m_impl->core.RunCycles(batch);
      remaining -= batch;
      continue;
    }
    const auto pc_before = m_impl->core.DSPState().pc;
    const auto cpu_mailbox_before = m_impl->core.PeekMailbox(DSP::Mailbox::CPU);
    const auto dsp_mailbox_before = m_impl->core.PeekMailbox(DSP::Mailbox::DSP);
    const auto opcode = m_impl->core.DSPState().PeekInstruction();
    if (trace_enabled() && g_task_disasm_reports == 0 &&
        m_impl->core.DSPState().pc == 0x06C5) {
      std::vector<u16> imem(0x0710);
      for (u16 address = 0; address < imem.size(); ++address)
        imem[address] = m_impl->core.DSPState().ReadIMEM(address);
      DSP::AssemblerSettings settings;
      settings.show_hex = true;
      settings.show_pc = true;
      settings.decode_names = true;
      settings.decode_registers = true;
      settings.lower_case_ops = false;
      DSP::DSPDisassembler disassembler(settings);
      std::string listing;
      u16 address = 0x06C5;
      while (address < 0x0710)
        disassembler.DisassembleOpcode(imem, &address, listing);
      std::fprintf(stderr, "[dsp-lle-trace] task-disasm\n%s", listing.c_str());
      ++g_task_disasm_reports;
    }
    const auto queue_write = m_impl->core.DSPState().ReadDMEM(0x0350);
    const auto queue_read = m_impl->core.DSPState().ReadDMEM(0x0351);
    const auto queue_count = m_impl->core.DSPState().ReadDMEM(0x0352);
    if (m_impl->core.DSPState().pc == 0x06C5 && queue_count != 0)
      g_task_message_trace = true;
    if (trace_enabled() && g_queue_cursor_reports < 32 &&
        (queue_write != g_last_queue_write || queue_read != g_last_queue_read ||
         queue_count != g_last_queue_count)) {
      std::fprintf(stderr,
                   "[dsp-lle-trace] queue-cursor pc=0x%04X "
                   "write=0x%04X read=0x%04X count=0x%04X "
                   "near=0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X\n",
                   m_impl->core.DSPState().pc, queue_write, queue_read,
                   queue_count,
                   m_impl->core.DSPState().ReadDMEM(0x0287),
                   m_impl->core.DSPState().ReadDMEM(0x0288),
                   m_impl->core.DSPState().ReadDMEM(0x0289),
                   m_impl->core.DSPState().ReadDMEM(0x028A),
                   m_impl->core.DSPState().ReadDMEM(0x028B),
                   m_impl->core.DSPState().ReadDMEM(0x028C));
      ++g_queue_cursor_reports;
    }
    g_last_queue_write = queue_write;
    g_last_queue_read = queue_read;
    g_last_queue_count = queue_count;
    if (trace_enabled() && g_task_loop_reports < 192 && g_task_message_trace &&
        m_impl->core.DSPState().pc >= 0x06C5 &&
        m_impl->core.DSPState().pc <= 0x06F3) {
      auto& state = m_impl->core.DSPState();
      std::fprintf(stderr,
                   "[dsp-lle-trace] task-loop pc=0x%04X op=0x%04X "
                   "next=0x%04X arg=0x%04X ar=0x%04X,0x%04X,0x%04X,0x%04X "
                   "ax0=0x%04X%04X ax1=0x%04X%04X "
                   "ac0=0x%04X%04X%04X ac1=0x%04X%04X%04X "
                   "st0=0x%04X st2=0x%04X st3=0x%04X sr=0x%04X\n",
                   state.pc, opcode, state.ReadIMEM(static_cast<u16>(state.pc + 1)),
                   state.ReadIMEM(static_cast<u16>(state.pc + 2)),
                   state.r.ar[0], state.r.ar[1], state.r.ar[2], state.r.ar[3],
                   state.r.ax[0].h, state.r.ax[0].l, state.r.ax[1].h,
                   state.r.ax[1].l, state.r.ac[0].h, state.r.ac[0].m,
                   state.r.ac[0].l, state.r.ac[1].h, state.r.ac[1].m,
                   state.r.ac[1].l, state.r.st[0], state.r.st[2], state.r.st[3],
                   state.r.sr);
      ++g_task_loop_reports;
    }
    if (trace_enabled() && g_frame_trace_reports > 0 &&
        g_frame_task_loop_reports < 48 &&
        ((m_impl->core.DSPState().pc >= 0x06C5 &&
          m_impl->core.DSPState().pc <= 0x06CC) ||
         m_impl->core.DSPState().pc == 0x06E9 ||
         m_impl->core.DSPState().pc == 0x06F1 ||
         m_impl->core.DSPState().pc == 0x06F3)) {
      auto& state = m_impl->core.DSPState();
      std::fprintf(stderr,
                   "[dsp-lle-trace] frame-task-loop pc=0x%04X op=0x%04X "
                   "next=0x%04X arg=0x%04X ar0=0x%04X wr0=0x%04X wr1=0x%04X "
                   "wr2=0x%04X wr3=0x%04X ax0=0x%04X%04X sr=0x%04X "
                   "queue-read=0x%04X count=0x%04X\n",
                   state.pc, opcode,
                   state.ReadIMEM(static_cast<u16>(state.pc + 1)),
                   state.ReadIMEM(static_cast<u16>(state.pc + 2)),
                   state.r.ar[0], state.r.wr[0], state.r.wr[1], state.r.wr[2],
                   state.r.wr[3], state.r.ax[0].h, state.r.ax[0].l,
                   state.r.sr, state.ReadDMEM(0x0351),
                   state.ReadDMEM(0x0352));
      ++g_frame_task_loop_reports;
    }
    const auto dscr_before = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSCR);
    const auto dspa_before = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSPA);
    const auto dsmah_before = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSMAH);
    const auto dsmal_before = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSMAL);
    if (DSP::GetOpTemplate(opcode) == nullptr) {
      std::fprintf(stderr,
                   "[dsp-lle] invalid DSP opcode pc=0x%04X opcode=0x%04X\n",
                   m_impl->core.DSPState().pc, opcode);
      m_impl->program_loaded = false;
      break;
    }
    const bool frame_trace_start =
        (cpu_mailbox_before & 0xFFFF0000u) == 0x82070000u;
    m_impl->core.RunCycles(1);
    if (frame_trace_start && g_frame_trace_reports == 0)
      g_frame_trace_reports = 1;
    if (trace_enabled() && g_frame_trace_reports > 0 &&
        g_frame_trace_reports < 257) {
      const auto* op_info = DSP::FindOpInfoByOpcode(opcode);
      std::fprintf(stderr,
                   "[dsp-lle-trace] frame-step #%u pc=0x%04X->0x%04X "
                   "op=0x%04X(%s) control=0x%04X cpu=0x%08X dsp=0x%08X\n",
                   g_frame_trace_reports, pc_before,
                   m_impl->core.DSPState().pc, opcode,
                   op_info == nullptr ? "?" : op_info->name,
                   m_impl->core.GetInterpreter().ReadControlRegister(),
                   m_impl->core.PeekMailbox(DSP::Mailbox::CPU),
                   m_impl->core.PeekMailbox(DSP::Mailbox::DSP));
      ++g_frame_trace_reports;
    }
    const auto dscr_after = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSCR);
    const auto dspa_after = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSPA);
    const auto dsmah_after = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSMAH);
    const auto dsmal_after = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSMAL);
    if (trace_enabled() && g_dsp_dma_state_reports < 48 &&
        (dscr_before != dscr_after || dspa_before != dspa_after ||
         dsmah_before != dsmah_after || dsmal_before != dsmal_after)) {
      std::fprintf(stderr,
                   "[dsp-lle-trace] dma-state pc=0x%04X->0x%04X "
                   "dscr=0x%04X->0x%04X dspa=0x%04X->0x%04X "
                   "ext=0x%04X%04X->0x%04X%04X\n",
                   pc_before, m_impl->core.DSPState().pc, dscr_before,
                   dscr_after, dspa_before, dspa_after, dsmah_before,
                   dsmal_before, dsmah_after, dsmal_after);
      ++g_dsp_dma_state_reports;
    }
    if (trace_enabled() && g_rom_pc_reports < 96 &&
        pc_before != g_last_rom_pc &&
        (pc_before == 0x00B2 || pc_before == 0x00B9 ||
         pc_before == 0x0523 || pc_before == 0x0525 ||
         pc_before == 0x052C || pc_before == 0x0532 ||
         pc_before == 0x0600 || pc_before == 0x0618 ||
         pc_before == 0x0627 || pc_before == 0x062C ||
         pc_before == 0x06C5)) {
      std::fprintf(stderr,
                   "[dsp-lle-trace] rom-pc=0x%04X->0x%04X "
                   "cpu-mailbox=0x%08X dsp-mailbox=0x%08X\n",
                   pc_before, m_impl->core.DSPState().pc,
                   m_impl->core.PeekMailbox(DSP::Mailbox::CPU),
                   m_impl->core.PeekMailbox(DSP::Mailbox::DSP));
      ++g_rom_pc_reports;
      g_last_rom_pc = pc_before;
    }
    if (trace_enabled() && g_task_pc_reports < 24 &&
        (pc_before == 0x0682 || pc_before == 0x0683 ||
         m_impl->core.DSPState().pc == 0x0682 ||
         m_impl->core.DSPState().pc == 0x0683)) {
      std::fprintf(stderr,
                   "[dsp-lle-trace] task-pc 0x%04X->0x%04X "
                   "dsp-mailbox=0x%08X\n",
                   pc_before, m_impl->core.DSPState().pc,
                   m_impl->core.PeekMailbox(DSP::Mailbox::DSP));
      ++g_task_pc_reports;
    }
    const auto cpu_mailbox_after = m_impl->core.PeekMailbox(DSP::Mailbox::CPU);
    const auto dsp_mailbox_after = m_impl->core.PeekMailbox(DSP::Mailbox::DSP);
    if (trace_enabled() && g_frame_state_reports < 2 &&
        g_frame_trace_reports >= 32 &&
        (m_impl->core.DSPState().pc == 0x06C5 ||
         m_impl->core.DSPState().pc == 0x06C7)) {
      std::fprintf(stderr,
                   "[dsp-lle-trace] frame-state pc=0x%04X "
                   "dmem350=0x%04X dmem351=0x%04X dmem352=0x%04X "
                   "dmem343=0x%04X dmem344=0x%04X dmem345=0x%04X "
                   "dmem346=0x%04X dmem347=0x%04X dmem348=0x%04X "
                   "dmem349=0x%04X dmem386=0x%04X dmem387=0x%04X "
                   "queue=0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X "
                   "read=0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X "
                   "tail=0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X "
                   "dscr=0x%04X dspa=0x%04X\n",
                   m_impl->core.DSPState().pc,
                   m_impl->core.DSPState().ReadDMEM(0x0350),
                   m_impl->core.DSPState().ReadDMEM(0x0351),
                   m_impl->core.DSPState().ReadDMEM(0x0352),
                   m_impl->core.DSPState().ReadDMEM(0x0343),
                   m_impl->core.DSPState().ReadDMEM(0x0344),
                   m_impl->core.DSPState().ReadDMEM(0x0345),
                   m_impl->core.DSPState().ReadDMEM(0x0346),
                   m_impl->core.DSPState().ReadDMEM(0x0347),
                   m_impl->core.DSPState().ReadDMEM(0x0348),
                   m_impl->core.DSPState().ReadDMEM(0x0349),
                   m_impl->core.DSPState().ReadDMEM(0x0386),
                   m_impl->core.DSPState().ReadDMEM(0x0387),
                   m_impl->core.DSPState().ReadDMEM(0x0280),
                   m_impl->core.DSPState().ReadDMEM(0x0281),
                   m_impl->core.DSPState().ReadDMEM(0x0282),
                   m_impl->core.DSPState().ReadDMEM(0x0283),
                   m_impl->core.DSPState().ReadDMEM(0x0284),
                   m_impl->core.DSPState().ReadDMEM(0x0285),
                   m_impl->core.DSPState().ReadDMEM(0x0288),
                   m_impl->core.DSPState().ReadDMEM(0x0289),
                   m_impl->core.DSPState().ReadDMEM(0x028A),
                   m_impl->core.DSPState().ReadDMEM(0x028B),
                   m_impl->core.DSPState().ReadDMEM(0x028C),
                   m_impl->core.DSPState().ReadDMEM(0x028D),
                   m_impl->core.DSPState().ReadDMEM(0x0290),
                   m_impl->core.DSPState().ReadDMEM(0x0291),
                   m_impl->core.DSPState().ReadDMEM(0x0292),
                   m_impl->core.DSPState().ReadDMEM(0x0293),
                   m_impl->core.DSPState().ReadDMEM(0x0294),
                   m_impl->core.DSPState().ReadDMEM(0x0295),
                   dscr_after, dspa_after);
      ++g_frame_state_reports;
    }
    if (trace_enabled() && g_mailbox_state_reports < 128 &&
        (cpu_mailbox_before != cpu_mailbox_after ||
         dsp_mailbox_before != dsp_mailbox_after)) {
      std::fprintf(stderr,
                   "[dsp-lle-trace] mailbox-state cpu=0x%08X->0x%08X "
                   "dsp=0x%08X->0x%08X pc=0x%04X\n",
                   cpu_mailbox_before, cpu_mailbox_after, dsp_mailbox_before,
                   dsp_mailbox_after, m_impl->core.DSPState().pc);
      ++g_mailbox_state_reports;
    }
    --remaining;
    if (g_task_message_trace && m_impl->core.DSPState().pc == 0x06F3)
      g_task_message_trace = false;
  }
  return remaining;
}

void Adapter::write_control(std::uint16_t value)
{
  if (m_impl->initialized) {
    const auto previous = m_impl->core.GetInterpreter().ReadControlRegister();
    trace("[dsp-lle-trace] control old=0x%04X new=0x%04X pc=0x%04X\n", previous,
          value, m_impl->core.DSPState().pc);
    if ((previous & DSP::CR_INIT) != 0 && (value & DSP::CR_INIT) == 0) {
      // The retail route releases the ROM reset vector here. Wind Waker's
      // DspBoot later loads jdsp through the task DMA path; the donor's
      // generic 0x81000000 INIT copy is not the title's contract.
      m_impl->core.DSPState().control_reg = previous & ~DSP::CR_INIT;
      m_impl->program_loaded = true;
    }
    m_impl->core.GetInterpreter().WriteControlRegister(value);
    if ((value & DSP::CR_EXTERNAL_INT) != 0) {
      // Attempt immediate delivery like donor non-thread mode. If jdsp has
      // masked external interrupts, CR_EXTERNAL_INT remains set and the next
      // interpreter steps retry it after the mask is restored.
      m_impl->deliver_pending_external_interrupt();
      if (trace_enabled() && g_external_reports < 8) {
        std::fprintf(stderr,
                     "[dsp-lle-trace] external-int value=0x%04X "
                     "cpu=0x%08X dsp=0x%08X pc=0x%04X\n",
                     value, m_impl->core.PeekMailbox(DSP::Mailbox::CPU),
                     m_impl->core.PeekMailbox(DSP::Mailbox::DSP),
                     m_impl->core.DSPState().pc);
        ++g_external_reports;
      }
    }
  }
}

std::uint16_t Adapter::read_control()
{
  return m_impl->initialized ? m_impl->core.GetInterpreter().ReadControlRegister() : 0;
}

void Adapter::write_cpu_mailbox(std::uint32_t value)
{
  if (!m_impl->initialized)
    return;
  if (trace_enabled() && g_mailbox_reports < 128) {
    std::fprintf(stderr,
                 "[dsp-lle-trace] cpu-mailbox value=0x%08X pc=0x%04X "
                 "control=0x%04X\n",
                 value, m_impl->core.DSPState().pc,
                 m_impl->core.GetInterpreter().ReadControlRegister());
    ++g_mailbox_reports;
  }
  trace("[dsp-lle-trace] cpu-mailbox value=0x%08X pc=0x%04X\n", value,
        m_impl->core.DSPState().pc);
  m_impl->core.WriteMailboxHigh(DSP::Mailbox::CPU, static_cast<std::uint16_t>(value >> 16));
  m_impl->core.WriteMailboxLow(DSP::Mailbox::CPU, static_cast<std::uint16_t>(value));
}

std::uint32_t Adapter::peek_cpu_mailbox() const
{
  return m_impl->initialized ? m_impl->core.PeekMailbox(DSP::Mailbox::CPU) : 0;
}

std::uint32_t Adapter::peek_dsp_mailbox() const
{
  return m_impl->initialized ? m_impl->core.PeekMailbox(DSP::Mailbox::DSP) : 0;
}

std::uint16_t Adapter::read_dsp_mailbox_low()
{
  if (!m_impl->initialized)
    return 0;
  const auto before = m_impl->core.PeekMailbox(DSP::Mailbox::DSP);
  const auto value = m_impl->core.ReadMailboxLow(DSP::Mailbox::DSP);
  if (trace_enabled() && (before & 0x80000000u) != 0 &&
      g_mail_read_reports < 16) {
    std::fprintf(stderr,
                 "[dsp-lle-trace] dsp-mail-read-low value=0x%04X "
                 "mailbox=0x%08X->0x%08X pc=0x%04X\n",
                 value, before, m_impl->core.PeekMailbox(DSP::Mailbox::DSP),
                 m_impl->core.DSPState().pc);
    ++g_mail_read_reports;
  }
  return value;
}

void Adapter::write_ifx(std::uint16_t address, std::uint16_t value)
{
  if (m_impl->initialized) {
    // DSBL bit 15 is the hardware DMA-start flag. The donor's DoDMA()
    // consumes only the byte count and rejects the flag as an oversized
    // transfer, so translate the register at this boundary.
    const auto donor_value =
        address == DSP::DSP_DSBL ? static_cast<std::uint16_t>(value & 0x7FFFu)
                                 : value;
    const auto external =
        (static_cast<std::uint32_t>(m_impl->core.DSPState().ReadIFX(DSP::DSP_DSMAH))
         << 16) |
        m_impl->core.DSPState().ReadIFX(DSP::DSP_DSMAL);
    const auto control = m_impl->core.DSPState().ReadIFX(DSP::DSP_DSCR);
    if (trace_enabled() &&
        (address == DSP::DSP_DSBL || address == DSP::DSP_DSCR ||
         address == DSP::DSP_DSPA || address == DSP::DSP_DSMAH ||
         address == DSP::DSP_DSMAL) &&
        g_dma_ifx_reports < 64) {
      std::fprintf(stderr,
                   "[dsp-lle-trace] dma-ifx address=0x%02X value=0x%04X "
                   "ext=0x%08X dsp=0x%04X ctl=0x%04X pc=0x%04X "
                   "ar0=0x%04X ar1=0x%04X ac0=0x%04X%04X ac1=0x%04X%04X "
                   "dmem356=0x%04X dmem357=0x%04X donor=0x%04X\n",
                   address, value, external,
                   m_impl->core.DSPState().ReadIFX(DSP::DSP_DSPA), control,
                   m_impl->core.DSPState().pc,
                   m_impl->core.DSPState().r.ar[0],
                   m_impl->core.DSPState().r.ar[1],
                   m_impl->core.DSPState().r.ac[0].m,
                   m_impl->core.DSPState().r.ac[0].l,
                   m_impl->core.DSPState().r.ac[1].m,
                   m_impl->core.DSPState().r.ac[1].l,
                   m_impl->core.DSPState().ReadDMEM(0x0356),
                   m_impl->core.DSPState().ReadDMEM(0x0357), donor_value);
      ++g_dma_ifx_reports;
    }
    if (address == DSP::DSP_DSBL || address == DSP::DSP_DSCR ||
        address == DSP::DSP_DSPA || address == DSP::DSP_DSMAH ||
        address == DSP::DSP_DSMAL) {
      trace("[dsp-lle-trace] ifx=0x%02X value=0x%04X ext=0x%08X dsp=0x%04X "
            "ctl=0x%04X pc=0x%04X\n",
            address, value, external,
            m_impl->core.DSPState().ReadIFX(DSP::DSP_DSPA), control,
            m_impl->core.DSPState().pc);
    }
    m_impl->core.DSPState().WriteIFX(address, donor_value);
    if (address == DSP::DSP_DSBL && donor_value != 0 &&
        control == (DSP::DSP_CR_IMEM | DSP::DSP_CR_FROM_CPU)) {
      const auto first = m_impl->host.callbacks.read_memory(external);
      const auto second = m_impl->host.callbacks.read_memory(external + 1);
      m_impl->program_loaded = (first != 0 || second != 0);
      if (!m_impl->program_loaded)
        std::fprintf(stderr,
                     "[dsp-lle] DSP IMEM DMA source empty at 0x%08X\n",
                     external);
    }
  }
}

std::uint16_t Adapter::read_ifx(std::uint16_t address)
{
  return m_impl->initialized ? m_impl->core.DSPState().ReadIFX(address) : 0;
}
}  // namespace bluewake::dsp
