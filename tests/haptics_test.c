// Public synthetic guest memory and SDL virtual controllers; no game inputs.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "haptics.h"
#include "gxruntime/platform.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static Uint16 low, high, left, right;
static unsigned stops;
static SDL_AtomicInt effect_on, effect_count;
static bool SDLCALL effect(void* unused, const void* data, int size) {
    (void)unused;
    const Uint8* bytes = data;
    assert(size == 47 && bytes[0] == 0x0C);
    assert(bytes[10] == bytes[21]);
    assert(bytes[10] == 0x26 || bytes[10] == 0x05);
    SDL_SetAtomicInt(&effect_on, bytes[10] == 0x26);
    SDL_AddAtomicInt(&effect_count, 1);
    return true;
}
static bool SDLCALL rumble(void* unused, Uint16 a, Uint16 b) {
    (void)unused; low = a; high = b; return true;
}
static bool SDLCALL triggers(void* unused, Uint16 a, Uint16 b) {
    (void)unused; left = a; right = b; return true;
}
static void motor(u32 channel, u32 command) {
    assert(channel < 4);
    if (command == 2) ++stops;
}
static void set_env(const char* key, const char* value) {
#ifdef _WIN32
    assert(_putenv_s(key, value) == 0);
#else
    assert(setenv(key, value, 1) == 0);
#endif
}
int main(int argc, char** argv) {
    assert(argc == 2);
    assert(SDL_Init(SDL_INIT_GAMEPAD));
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = "BlueWake haptics regression";
    desc.Rumble = rumble;
    const bool ps5 = strcmp(argv[1], "ps5") == 0;
    if (ps5) {
        desc.vendor_id = 0x054C;
        desc.product_id = 0x0CE6;
        desc.SendEffect = effect;
    } else desc.RumbleTriggers = triggers;
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    assert(id != 0);
    SDL_Gamepad* pad = SDL_OpenGamepad(id);
    assert(pad != NULL);
    DolPlatformOps ops = {0};
    ops.pad_control_motor = motor;
    dol_platform_install(&ops);
    CPUState cpu;
    assert(cpu_init(&cpu));
    set_env("BLUEWAKE_HAPTICS", ps5 ? "enhanced" : argv[1]);
    set_env("BLUEWAKE_HAPTICS_STRENGTH", "80");
    set_env("BLUEWAKE_HAPTICS_TRIGGERS", "1");
    bluewake_haptics_attach(&cpu);
    assert(bluewake_haptics_forward_motor()); // Classic or unrecognized guest fallback.
    if (strcmp(argv[1], "classic") == 0) {
        bluewake_haptics_block(true);
        assert(!bluewake_haptics_forward_motor());
        assert(stops == 4);
        bluewake_haptics_block(false);
        assert(bluewake_haptics_forward_motor());
    } else {
        const u32 base = 0x803CA5A8u;
        mem_write32(&cpu, base + 0x80, 0x8037D460u);
        mem_write32(&cpu, base + 0x48, (u32)-1); // No shock.
        mem_write32(&cpu, base + 0x60, 1); // Continuous quake.
        mem_write32(&cpu, base + 0x64, 0xFFFFFFFFu);
        mem_write32(&cpu, base + 0x68, 32);
        for (unsigned i = 0; i < 30; ++i) bluewake_haptics_retrace();
        assert(!bluewake_haptics_forward_motor());
        assert(low > 0 && high > 0);
        if (ps5) {
            assert(SDL_GetGamepadType(pad) == SDL_GAMEPAD_TYPE_PS5);
            assert(SDL_GetAtomicInt(&effect_on));
            SDL_Delay(350); // A stall stops persistent effects without a retrace.
            assert(!SDL_GetAtomicInt(&effect_on));
            mem_write32(&cpu, base + 0x78, 1);
            bluewake_haptics_retrace();
            assert(SDL_GetAtomicInt(&effect_on));
            // More reconnects than the fixed slot count must still get effects.
            for (unsigned i = 0; i < 10; ++i) {
                SDL_CloseGamepad(pad);
                assert(SDL_DetachVirtualJoystick(id));
                id = SDL_AttachVirtualJoystick(&desc);
                assert(id != 0);
                pad = SDL_OpenGamepad(id);
                assert(pad != NULL);
                int before = SDL_GetAtomicInt(&effect_count);
                SDL_Delay(55);
                mem_write32(&cpu, base + 0x78, 2 + i);
                bluewake_haptics_retrace();
                assert(SDL_GetAtomicInt(&effect_count) > before);
                assert(SDL_GetAtomicInt(&effect_on));
            }
        } else assert(left > 0 && right > 0);
        // The Mac menu stops retraces. Feedback must stop on opening it.
        bluewake_haptics_block(true);
        assert(low == 0 && high == 0 && left == 0 && right == 0);
        assert(!SDL_GetAtomicInt(&effect_on));
        bluewake_haptics_block(false);
        mem_write32(&cpu, base + 0x78, 1);
        bluewake_haptics_retrace();
        assert(low > 0);
        set_env("BLUEWAKE_HAPTICS", "off");
        bluewake_haptics_reload();
        assert(!bluewake_haptics_forward_motor());
        assert(low == 0 && high == 0 && left == 0 && right == 0);
        assert(!SDL_GetAtomicInt(&effect_on));
    }
    // Shut down with an active effect, while its timer is still armed.
    if (ps5) {
        set_env("BLUEWAKE_HAPTICS", "enhanced");
        bluewake_haptics_reload();
        mem_write32(&cpu, 0x803CA5A8u + 0x78, 100);
        bluewake_haptics_retrace();
        assert(SDL_GetAtomicInt(&effect_on));
    }
    bluewake_haptics_shutdown();
    assert(!SDL_GetAtomicInt(&effect_on));
    int after = SDL_GetAtomicInt(&effect_count);
    if (ps5) SDL_Delay(350);
    assert(SDL_GetAtomicInt(&effect_count) == after);
    cpu_free(&cpu);
    dol_platform_reset();
    SDL_CloseGamepad(pad);
    assert(SDL_DetachVirtualJoystick(id));
    SDL_Quit();
}
