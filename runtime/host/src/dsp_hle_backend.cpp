// High-level DSP backend: Dolphin's DSPHLE (the Zelda ucode for Wind Waker)
// behind the same C adapter interface as the donor LLE interpreter.
//
// The LLE interpreter executes the DSP's own microcode instruction by
// instruction and costs 16.7% of the game thread in an Outset play-scene sample
// on the iOS simulator. Dolphin's HLE implements the same ucode's mail protocol
// and mixer natively. It changes when mails and interrupts arrive relative to
// the CPU, so it is opt-in (BLUEWAKE_DSP_MODE=hle) and carries its own
// acceptance, separate from the certified LLE route digest.
//
// Dolphin's HLE reaches the rest of the emulator through Core::System. The few
// services it uses are implemented here over host callbacks: guest-memory
// pointers (alias-aware), the ARAM buffer, the guest timebase and the DSP
// interrupt. Everything else it can reach (config, files, analytics) is inert.

#include "dsp_hle_backend.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "Common/ChunkFile.h"
#include "Common/CommonTypes.h"
#include "Common/Swap.h"
#include "Core/Config/MainSettings.h"
#include "Core/Core.h"
#include "Core/DSPEmulator.h"
#include "Core/DolphinAnalytics.h"
#include "Core/HW/DSP.h"
#include "Core/HW/DSPHLE/DSPHLE.h"
#include "Core/HW/Memmap.h"
#include "Core/HW/SystemTimers.h"
#include "Core/System.h"

namespace
{
struct HleHost
{
  void* user = nullptr;
  BluewakeDspGuestPointerFn guest_pointer = nullptr;
  BluewakeDspTimebaseFn timebase = nullptr;
  BluewakeDspInterruptFn interrupt = nullptr;
  u8* aram = nullptr;
  u32 aram_size = 0;
  // Interrupts the HLE raises are delivered on the next run_cycles call. The
  // mail handler raises the next mail's interrupt from inside the CPU's read of
  // the current mail, and the host clears its pending flag right after that
  // read, so an immediate delivery would be lost.
  bool interrupt_pending = false;
  u64 interrupts = 0;
  int trace = 0;  // BLUEWAKE_DSP_HLE_TRACE=N logs the first N mail events
};
HleHost g_host;

template <typename T>
T& inert_object()
{
  alignas(T) static std::byte storage[sizeof(T)]{};
  return *reinterpret_cast<T*>(storage);
}

u8* guest_pointer(u32 address, size_t size)
{
  if (g_host.guest_pointer == nullptr || size > 0xFFFFFFFFu)
    return nullptr;
  return g_host.guest_pointer(g_host.user, address, static_cast<u32>(size));
}
}  // namespace

extern "C" u64 (*bluewake_dsp_fake_timebase_hook)(void);

namespace Core
{
DSP::DSPManager& System::GetDSP() const
{
  return inert_object<DSP::DSPManager>();
}

Memory::MemoryManager& System::GetMemory() const
{
  return inert_object<Memory::MemoryManager>();
}

void DisplayMessage(std::string message, int)
{
  std::fprintf(stderr, "[dsp-hle] %s\n", message.c_str());
}
}  // namespace Core

namespace DSP
{
void DSPManager::GenerateDSPInterruptFromDSPEmu(DSPInterruptType, int)
{
  g_host.interrupt_pending = true;
}

u8* DSPManager::GetARAMPtr() const
{
  return g_host.aram;
}

u32 DSPManager::GetARAMSize() const
{
  return g_host.aram_size;
}

u8 DSPManager::ReadARAM(u32 address) const
{
  return g_host.aram != nullptr ? g_host.aram[address & (g_host.aram_size - 1)] : 0;
}

void DSPManager::WriteARAM(u8 value, u32 address)
{
  if (g_host.aram != nullptr)
    g_host.aram[address & (g_host.aram_size - 1)] = value;
}
}  // namespace DSP

namespace Memory
{
u8* MemoryManager::GetPointerForRange(u32 address, size_t size) const
{
  return guest_pointer(address, size);
}

u16 MemoryManager::Read_U16(u32 address) const
{
  const u8* p = guest_pointer(address, 2);
  return p != nullptr ? static_cast<u16>((p[0] << 8) | p[1]) : 0;
}

u32 MemoryManager::Read_U32(u32 address) const
{
  const u8* p = guest_pointer(address, 4);
  return p != nullptr ? (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | p[3] : 0;
}

u32 MemoryManager::Read_U32_Swap(u32 address) const
{
  return Common::swap32(Read_U32(address));
}

void MemoryManager::Write_U16(u16 value, u32 address)
{
  if (u8* p = guest_pointer(address, 2))
  {
    p[0] = static_cast<u8>(value >> 8);
    p[1] = static_cast<u8>(value);
  }
}

void MemoryManager::Write_U32(u32 value, u32 address)
{
  if (u8* p = guest_pointer(address, 4))
  {
    p[0] = static_cast<u8>(value >> 24);
    p[1] = static_cast<u8>(value >> 16);
    p[2] = static_cast<u8>(value >> 8);
    p[3] = static_cast<u8>(value);
  }
}

void MemoryManager::CopyToEmu(u32 address, const void* data, size_t size)
{
  if (u8* p = guest_pointer(address, size))
    std::memcpy(p, data, size);
}
}  // namespace Memory

namespace SystemTimers
{
u32 SystemTimersManager::GetTicksPerSecond() const
{
  return 486000000u;
}
}  // namespace SystemTimers

DSPEmulator::~DSPEmulator() = default;

DolphinAnalytics& DolphinAnalytics::Instance()
{
  return inert_object<DolphinAnalytics>();
}

void DolphinAnalytics::ReportGameQuirk(GameQuirk)
{
}

namespace Config
{
const Info<bool> MAIN_DUMP_UCODE{{System::Main, "DSP", "DumpUCode"}, false};
}  // namespace Config

namespace
{
u64 host_fake_timebase()
{
  return g_host.timebase != nullptr ? g_host.timebase(g_host.user) : 0;
}
}  // namespace

namespace bluewake::dsp
{
struct HleBackend::Impl
{
  DSP::HLE::DSPHLE hle{Core::System::GetInstance()};
};

HleBackend::HleBackend() : m_impl(new Impl)
{
}

HleBackend::~HleBackend()
{
  m_impl->hle.Shutdown();
  delete m_impl;
  bluewake_dsp_fake_timebase_hook = nullptr;
  g_host = HleHost{};
}

bool HleBackend::initialize(void* user, BluewakeDspGuestPointerFn guest_pointer_fn,
                            std::uint8_t* aram, std::uint32_t aram_size,
                            BluewakeDspTimebaseFn timebase, BluewakeDspInterruptFn interrupt)
{
  if (guest_pointer_fn == nullptr || aram == nullptr || aram_size == 0 ||
      (aram_size & (aram_size - 1)) != 0 || interrupt == nullptr)
    return false;
  g_host = HleHost{};
  g_host.user = user;
  g_host.guest_pointer = guest_pointer_fn;
  g_host.timebase = timebase;
  g_host.interrupt = interrupt;
  g_host.aram = aram;
  g_host.aram_size = aram_size;
  if (const char* trace = std::getenv("BLUEWAKE_DSP_HLE_TRACE"))
    g_host.trace = std::atoi(trace);
  bluewake_dsp_fake_timebase_hook = host_fake_timebase;
  return m_impl->hle.Initialize(false, false);
}

int HleBackend::run_cycles(int cycles)
{
  m_impl->hle.DSP_Update(cycles);
  if (g_host.interrupt_pending)
  {
    g_host.interrupt_pending = false;
    g_host.interrupts++;
    if (g_host.trace > 0)
    {
      g_host.trace--;
      std::fprintf(stderr, "[dsp-hle] interrupt delivered #%llu\n",
                   static_cast<unsigned long long>(g_host.interrupts));
    }
    g_host.interrupt(g_host.user);
  }
  return cycles;
}

void HleBackend::write_control(std::uint16_t value)
{
  m_impl->hle.DSP_WriteControlRegister(value);
}

std::uint16_t HleBackend::read_control()
{
  return m_impl->hle.DSP_ReadControlRegister();
}

void HleBackend::write_cpu_mailbox(std::uint32_t value)
{
  if (g_host.trace > 0)
  {
    g_host.trace--;
    std::fprintf(stderr, "[dsp-hle] cpu->dsp %08x\n", value);
  }
  m_impl->hle.DSP_WriteMailBoxHigh(true, static_cast<u16>(value >> 16));
  m_impl->hle.DSP_WriteMailBoxLow(true, static_cast<u16>(value));
}

std::uint32_t HleBackend::peek_cpu_mailbox()
{
  return (u32(m_impl->hle.DSP_ReadMailBoxHigh(true)) << 16) |
         m_impl->hle.DSP_ReadMailBoxLow(true);
}

std::uint32_t HleBackend::peek_dsp_mailbox()
{
  // Only the high half is observed by the host before the popping low read.
  return u32(m_impl->hle.DSP_ReadMailBoxHigh(false)) << 16;
}

std::uint16_t HleBackend::read_dsp_mailbox_low()
{
  const u16 high = m_impl->hle.DSP_ReadMailBoxHigh(false);
  const u16 low = m_impl->hle.DSP_ReadMailBoxLow(false);
  if (g_host.trace > 0)
  {
    g_host.trace--;
    std::fprintf(stderr, "[dsp-hle] dsp->cpu %04x%04x\n", high, low);
  }
  return low;
}

std::uint64_t HleBackend::interrupts() const
{
  return g_host.interrupts;
}

namespace
{
void do_backend_state(DSP::HLE::DSPHLE& hle, PointerWrap& p)
{
  p.Do(g_host.interrupt_pending);
  p.Do(g_host.interrupts);
  hle.DoState(p);
}
}  // namespace

std::vector<std::uint8_t> HleBackend::save_state()
{
  u8* measure_ptr = nullptr;
  PointerWrap measure(&measure_ptr, 0, PointerWrap::Mode::Measure);
  do_backend_state(m_impl->hle, measure);
  std::vector<std::uint8_t> buffer(reinterpret_cast<std::size_t>(measure_ptr));
  u8* write_ptr = buffer.data();
  PointerWrap write(&write_ptr, buffer.size(), PointerWrap::Mode::Write);
  do_backend_state(m_impl->hle, write);
  if (!write.IsWriteMode())
    buffer.clear();
  return buffer;
}

bool HleBackend::load_state(const std::uint8_t* data, std::size_t size)
{
  u8* read_ptr = const_cast<u8*>(data);
  PointerWrap read(&read_ptr, size, PointerWrap::Mode::Read);
  do_backend_state(m_impl->hle, read);
  return read.IsReadMode() && read_ptr == data + size;
}
}  // namespace bluewake::dsp
