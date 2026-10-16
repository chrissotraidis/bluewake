// Synthetic Picto actor-id regression. Run manually with empty, isolated
// DOL_AURORA_CACHE_DIR and DOL_AURORA_USER_DIR; AURORA_SYNC_PIPELINES unset/0.
#include "gxruntime/aurora_backend.h"
#include "gxruntime/aurora_recomp/render_sink.hpp"
#include "gxruntime/gxcore/gxcore.hpp"

#include "gfx/gxcore_draw.hpp"
#include "gfx/common.hpp"

#include <aurora/gfx.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>

extern "C" {
void aurora_backend_present(void);
}

namespace ar = gxruntime::aurora_recomp;
namespace gxc = gxruntime::gxcore;

namespace {

ar::RenderStatePacket bp(std::uint32_t reg, std::uint32_t value) {
  return {.kind = ar::RenderStateKind::BpReg, .index = reg, .value = value};
}

void append_be32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 24u));
  out.push_back(static_cast<std::uint8_t>(value >> 16u));
  out.push_back(static_cast<std::uint8_t>(value >> 8u));
  out.push_back(static_cast<std::uint8_t>(value));
}

void append_be_f32(std::vector<std::uint8_t>& out, float value) {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof bits);
  append_be32(out, bits);
}

void configure_transform(ar::ConsumedDraw& draw) {
  draw.transform_flags =
      ar::kDrawTransformProjectionValid | ar::kDrawTransformViewportValid;
  draw.projection[0] = 1.0f;
  draw.projection[2] = 1.0f;
  draw.projection[4] = -1.0f;
  draw.projection_type = 1u;
  draw.current_pn_matrix = 0u;
  draw.position_matrix_valid_mask = 1u;
  draw.position_matrices[0][0] = 1.0f;
  draw.position_matrices[0][5] = 1.0f;
  draw.position_matrices[0][10] = 1.0f;
  draw.viewport[0] = 320.0f;
  draw.viewport[1] = -240.0f;
  draw.viewport[2] = 16777215.0f;
  draw.viewport[3] = 662.0f;
  draw.viewport[4] = 582.0f;
  draw.viewport[5] = 16777215.0f;
}

gxc::DrawPlan build_solid(std::array<std::uint8_t, 4> color,
                          std::uint32_t cmode0, bool dst_alpha_enable,
                          std::uint8_t dst_alpha, std::uint8_t pixel_format,
                          float left, float right,
                          gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 0u));
  state.apply(bp(0x40u, 0u));
  state.apply(bp(0x41u, cmode0));
  state.apply(bp(0x42u, static_cast<std::uint32_t>(dst_alpha) |
                            (dst_alpha_enable ? 1u << 8u : 0u)));
  state.apply(bp(0x43u, pixel_format));
  state.apply({.kind = ar::RenderStateKind::CpVcd,
               .index = 0u,
               .value = (1u << 9u) | (1u << 13u)});
  state.apply({.kind = ar::RenderStateKind::CpVcd,
               .index = 1u,
               .value = 0u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 1u | (4u << 1u) | (5u << 14u),
               .aux0 = 0u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 0u,
               .aux0 = 1u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 0u,
               .aux0 = 2u});

  ar::ConsumedDraw draw{};
  draw.primitive = 0x80u;
  draw.vertex_count = 4u;
  draw.vertex_size = 16u;
  configure_transform(draw);
  const float positions[4][3] = {
      {left, -1.0f, 0.0f},
      {right, -1.0f, 0.0f},
      {right, 1.0f, 0.0f},
      {left, 1.0f, 0.0f},
  };
  for (const auto& position : positions) {
    for (float value : position)
      append_be_f32(draw.vertex_payload, value);
    draw.vertex_payload.insert(draw.vertex_payload.end(), color.begin(),
                               color.end());
  }
  return state.build_draw_plan(draw, counters);
}

bool expect_id(const char* label, std::uint16_t x, std::uint8_t expected) {
  std::uint32_t argb = 0;
  const bool read = aurora_peek_argb(x, 240u, &argb);
  const auto actual = static_cast<std::uint8_t>(argb >> 24u);
  if (!read || actual != expected) {
    std::fprintf(stderr, "Picto %s: x=%u read=%d ARGB=%08x expected alpha=%u\n",
                 label, x, read, argb, expected);
    return false;
  }
  return true;
}

bool expect_ready(bool expected, const char* label) {
  if (aurora_color_peek_ready() == expected)
    return true;
  std::fprintf(stderr, "Picto %s: unexpected snapshot readiness\n", label);
  return false;
}

} // namespace

int main() {
  // Pipeline policy is cached on first use: never temporarily enable global
  // sync, even for initialization, or this would hide the cold-cache bug.
  const char* sync = std::getenv("AURORA_SYNC_PIPELINES");
  const char* cache = std::getenv("DOL_AURORA_CACHE_DIR");
  const char* user = std::getenv("DOL_AURORA_USER_DIR");
  if ((sync && sync[0] != '0') || !cache || !cache[0] || !user || !user[0] ||
      (std::filesystem::exists(cache) && !std::filesystem::is_empty(cache))) {
    std::fprintf(stderr, "Picto: requires fresh DOL_AURORA_CACHE_DIR, isolated "
                         "DOL_AURORA_USER_DIR, AURORA_SYNC_PIPELINES unset/0\n");
    return 2;
  }
  std::filesystem::create_directories(cache);
  std::filesystem::create_directories(user);
#ifdef _WIN32
  _putenv_s("SDL_AUDIODRIVER", "dummy");
#else
  setenv("SDL_AUDIODRIVER", "dummy", 1);
#endif
  const AuroraBackendConfig config = {
      .app_name = "BlueWake Picto peek fixture",
      .window_width = 640,
      .window_height = 480,
      .vsync = false,
      .allow_texture_dumps = false,
      .info_logging = true,
      .graphics_logging = false,
      .force_untextured = false,
  };
  char arg0[] = "bluewake_gxcore_picto_peek_pixels_test";
  char* argv[] = {arg0, nullptr};
  if (!dol_aurora_initialize(1, argv, &config)) {
    std::fprintf(stderr, "Picto: Aurora initialization failed\n");
    return 2;
  }

  gxc::GapCounters counters{};
  const auto alpha_rect = [&](std::uint8_t id, float left, float right) {
    // RGBA6_Z24, alpha update only, destination-alpha override. The TEV
    // color deliberately differs from the ID; alpha must come from dstalpha.
    return build_solid({255u, 0u, 255u, 255u}, 1u << 4u, true, id,
                       1u, left, right, counters);
  };
  const auto clear = alpha_rect(252u, -1.0f, 1.0f);
  const auto left = alpha_rect(4u, -0.75f, -0.25f);
  const auto right = alpha_rect(8u, 0.25f, 0.75f);
  const auto overwrite = alpha_rect(12u, -1.0f, 1.0f);
  const auto submit = [](const gxc::DrawPlan& plan) {
    return plan.ok && aurora::gfx::gxcore::submit_draw_plan(plan);
  };

  bool ok = expect_ready(false, "initial frame");
  ok = submit(clear) && ok;
  ok = submit(left) && ok;
  ok = submit(right) && ok;
  // No ordinary present, warmup draw, or polling retry before the first peek.
  ok = expect_id("cold clear", 32u, 252u) && ok;
  ok = expect_ready(true, "first capture") && ok;
  ok = expect_id("cold left actor", 160u, 4u) && ok;
  ok = expect_id("cold right actor", 480u, 8u) && ok;
  ok = expect_id("clear between actors", 320u, 252u) && ok;

  // Actually submit a different EFB while preserving this real frame. A
  // second capture would return 12; the public peek must reuse its snapshot.
  ok = submit(overwrite) && ok;
  ok = aurora::gfx::segment_frame() && ok;
  ok = expect_ready(true, "continued frame") && ok;
  ok = expect_id("cached left actor", 160u, 4u) && ok;
  ok = expect_id("cached right actor", 480u, 8u) && ok;
  ok = expect_id("cached clear", 32u, 252u) && ok;

  aurora_backend_present();
  ok = expect_ready(false, "next real frame") && ok;
  ok = submit(overwrite) && ok;
  ok = expect_id("fresh next-frame capture", 160u, 12u) && ok;
  ok = expect_ready(true, "next-frame capture") && ok;
  aurora_backend_present();
  dol_aurora_shutdown();
  std::printf("Picto peek fixture: %s (cold IDs 252/4/8, cached snapshot, "
              "next-frame ID 12; global pipeline sync disabled)\n",
              ok ? "PASS" : "FAIL");
  return ok ? 0 : 1;
}
