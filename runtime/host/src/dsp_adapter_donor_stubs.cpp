#include <cstddef>

#include "Core/System.h"
#include "Core/CoreTiming.h"
#include "Core/HW/SystemTimers.h"

// Set by the high-level DSP backend; the LLE route leaves it null.
extern "C" {
u64 (*bluewake_dsp_fake_timebase_hook)(void) = nullptr;
}

namespace
{
template <typename T>
T& dummy_object()
{
  alignas(T) static std::byte storage[sizeof(T)]{};
  return *reinterpret_cast<T*>(storage);
}
}  // namespace

namespace Core
{
struct System::Impl
{
};

System::System() = default;
System::~System() = default;

CoreTiming::CoreTimingManager& System::GetCoreTiming() const
{
  return dummy_object<CoreTiming::CoreTimingManager>();
}

SystemTimers::SystemTimersManager& System::GetSystemTimers() const
{
  return dummy_object<SystemTimers::SystemTimersManager>();
}
}  // namespace Core

namespace CoreTiming
{
void CoreTimingManager::ForceExceptionCheck(s64)
{
}
}  // namespace CoreTiming

namespace SystemTimers
{
u64 SystemTimersManager::GetFakeTimeBase() const
{
  // The LLE route never reads the timebase through Dolphin; the high-level
  // backend installs a hook to the guest timebase.
  return bluewake_dsp_fake_timebase_hook != nullptr ? bluewake_dsp_fake_timebase_hook() : 0;
}
}  // namespace SystemTimers
