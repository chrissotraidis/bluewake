#!/usr/bin/env python3
"""The native generators' frame form (scripts/windows/native_gx_gen.py, #451).

inline_save_restore_gpr.py saves and restores a register range through a host
pointer into the frame (cmake/composite/inline_gpr.h's bw_inline_gpr_frame), and
the generators stopped on that shape. This checks the rewrite that turns the same
addresses into the run's own memory, on the exact text the inline pass emits.
"""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts" / "windows"))
import native_gx_gen  # noqa: E402  (the generator, imported for one method)

rewrite = native_gx_gen.Gen.inline_gpr_frame  # a text transform; it holds no state


def check(name, source, expected):
    got = rewrite(None, source)
    if got != expected:
        print(f"FAIL {name}\n--- got\n{got}--- want\n{expected}")
        return False
    return True


ok = True

# A save: the frame at r11 - 4 * (32 - 26) = r11 - 24, its stores as gx_stp32.
ok &= check("save", """                ctx->downcount -= 6;
                u8* const slots = bw_inline_gpr_frame(ctx, 26u, true);
                write_be32(slots + 0, (u32)ctx->gpr[26]);
                write_be32(slots + 20, (u32)ctx->gpr[31]);
""", """                ctx->downcount -= 6;
                const u32 slots_0 = ctx->gpr[11] - 24u;
                gx_stp32(&s, 0u, slots_0 + 0u, (u32)ctx->gpr[26]);
                gx_stp32(&s, 0u, slots_0 + 20u, (u32)ctx->gpr[31]);
""")

# A restore, and two frames in one native: inline_gpr inlines both into one scope,
# so the names have to differ.
ok &= check("restore and two frames", """                u8* const slots = bw_inline_gpr_frame(ctx, 27u, false);
                ctx->gpr[27] = read_be32(slots + 0);
                u8* const slots = bw_inline_gpr_frame(ctx, 26u, true);
                write_be32(slots + 4, (u32)ctx->gpr[27]);
""", """                const u32 slots_0 = ctx->gpr[11] - 20u;
                ctx->gpr[27] = gx_ld32(&s, slots_0 + 0u);
                const u32 slots_1 = ctx->gpr[11] - 24u;
                gx_stp32(&s, 0u, slots_1 + 4u, (u32)ctx->gpr[27]);
""")

# The frame inside the block inline_gpr rewrites: the surrounding lines survive.
ok &= check("frame inside the inline block", """            if (!(s.downcount - 5 > -s.budget))
                return 0; /* the routine itself, through the chassis */
            s.downcount -= 6;
            u8* const slots = bw_inline_gpr_frame(ctx, 30u, true);
            write_be32(slots + 0, (u32)ctx->gpr[30]);
            s.suffix = 0u;
""", """            if (!(s.downcount - 5 > -s.budget))
                return 0; /* the routine itself, through the chassis */
            s.downcount -= 6;
            const u32 slots_0 = ctx->gpr[11] - 8u;
            gx_stp32(&s, 0u, slots_0 + 0u, (u32)ctx->gpr[30]);
            s.suffix = 0u;
""")

# Text without the frame form is untouched.
ok &= check("no frame", "    ctx->gpr[3] = 1u;\n", "    ctx->gpr[3] = 1u;\n")

print("test_native_generator_frame:", "OK" if ok else "FAILED")
sys.exit(0 if ok else 1)
