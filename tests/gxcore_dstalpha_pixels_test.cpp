// Public GPU fixture for GX destination-alpha and EFB alpha semantics.
#include "gxruntime/aurora_backend.h"
#include "gxruntime/aurora_recomp/render_sink.hpp"
#include "gxruntime/gxcore/gxcore.hpp"

#include "gfx/gxcore_draw.hpp"

#include <aurora/gfx.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
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
                          gxc::GapCounters& counters,
                          bool top_half_scissor = false) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 0u));
  state.apply(bp(0x40u, 0u));
  state.apply(bp(0x41u, cmode0));
  state.apply(bp(0x42u, static_cast<std::uint32_t>(dst_alpha) |
                            (dst_alpha_enable ? 1u << 8u : 0u)));
  state.apply(bp(0x43u, pixel_format));
  if (top_half_scissor) {
    state.apply(bp(0x20u, (342u << 12u) | 342u));
    state.apply(bp(0x21u, (981u << 12u) | 581u));
    state.apply(bp(0x59u, 171u | (171u << 10u)));
  }
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

bool near(std::uint8_t actual, int expected, int tolerance = 2) {
  const int delta = static_cast<int>(actual) - expected;
  return delta >= -tolerance && delta <= tolerance;
}

} // namespace

int main() {
  setenv("AURORA_SYNC_PIPELINES", "1", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);
  const AuroraBackendConfig config = {
      .app_name = "BlueWake destination alpha fixture",
      .window_width = 640,
      .window_height = 480,
      .vsync = false,
      .allow_texture_dumps = false,
      .info_logging = false,
      .graphics_logging = false,
      .force_untextured = false,
  };
  char arg0[] = "bluewake_gxcore_dstalpha_pixels_test";
  char* argv[] = {arg0, nullptr};
  if (!dol_aurora_initialize(1, argv, &config)) {
    std::fprintf(stderr,
                 "destination alpha fixture: Aurora initialization failed\n");
    return 2;
  }

  aurora::gfx::gxcore::reset_texture_cache();
  gxc::GapCounters counters{};
  constexpr std::uint32_t color_alpha_update = (1u << 3u) | (1u << 4u);
  constexpr std::uint32_t alpha_update = 1u << 4u;
  constexpr std::uint32_t dstalpha_blend =
      1u | (7u << 5u) | (6u << 8u) | (1u << 3u);
  const gxc::DrawPlan blue =
      build_solid({0u, 0u, 255u, 255u}, color_alpha_update, false, 0u,
                  1u, -1.0f, 1.0f, counters);
  const gxc::DrawPlan write_zero_alpha =
      build_solid({0u, 255u, 0u, 255u}, alpha_update, true, 0u,
                  1u, -1.0f, 0.0f, counters);
  const gxc::DrawPlan blend_red =
      build_solid({255u, 0u, 0u, 255u}, dstalpha_blend, false, 0u,
                  1u, -1.0f, 0.0f, counters);

  // RGB8_Z24 has no writable alpha and reads destination alpha as one. The
  // first right-half draw attempts to poison host alpha with zero; the second
  // must therefore render red rather than multiplying by that stored zero.
  const gxc::DrawPlan rgb8_alpha_write =
      build_solid({0u, 255u, 0u, 0u}, alpha_update, false, 0u,
                  0u, 0.0f, 1.0f, counters);
  const gxc::DrawPlan rgb8_blend_red =
      build_solid({255u, 0u, 0u, 255u},
                  1u | (6u << 8u) | (1u << 3u), false, 0u,
                  0u, 0.0f, 1.0f, counters);
  const gxc::DrawPlan scissored_green =
      build_solid({0u, 255u, 0u, 255u}, color_alpha_update, false, 0u,
                  1u, -1.0f, 1.0f, counters, true);

  bool ok = blue.ok && write_zero_alpha.ok && blend_red.ok &&
            rgb8_alpha_write.ok && rgb8_blend_red.ok && scissored_green.ok;
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(blue);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(write_zero_alpha);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(blend_red);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(rgb8_alpha_write);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(rgb8_blend_red);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(scissored_green);

  aurora_request_framebuffer_readback();
  aurora_backend_present();
  const std::uint8_t* rgba = nullptr;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  bool readback = false;
  for (int attempt = 0; attempt < 20000; ++attempt) {
    if (aurora_take_framebuffer_readback(&rgba, &width, &height)) {
      readback = true;
      break;
    }
    aurora_pump_framebuffer_readback();
    std::this_thread::sleep_for(std::chrono::microseconds(250));
  }
  if (!readback) {
    std::fprintf(stderr,
                 "destination alpha fixture: framebuffer readback timed out\n");
    ok = false;
  } else {
    const auto top_row = static_cast<std::size_t>(height / 4u) * width;
    const auto bottom_row = static_cast<std::size_t>(3u * height / 4u) * width;
    const std::uint8_t* top_left = rgba + (top_row + width / 4u) * 4u;
    const std::uint8_t* top_right = rgba + (top_row + 3u * width / 4u) * 4u;
    const std::uint8_t* bottom_left = rgba + (bottom_row + width / 4u) * 4u;
    const std::uint8_t* bottom_right =
        rgba + (bottom_row + 3u * width / 4u) * 4u;
    const bool top_green = near(top_left[0], 0) && near(top_left[1], 255) &&
                           near(top_left[2], 0) && near(top_right[0], 0) &&
                           near(top_right[1], 255) && near(top_right[2], 0);
    const bool bottom_blue_red =
        near(bottom_left[0], 0) && near(bottom_left[1], 0) &&
        near(bottom_left[2], 255) && near(bottom_right[0], 255) &&
        near(bottom_right[1], 0) && near(bottom_right[2], 0);
    if (!top_green || !bottom_blue_red) {
      std::fprintf(stderr,
                   "destination alpha fixture: tl=(%u,%u,%u) tr=(%u,%u,%u) "
                   "bl=(%u,%u,%u) br=(%u,%u,%u), expected green/green/blue/red\n",
                   top_left[0], top_left[1], top_left[2], top_right[0],
                   top_right[1], top_right[2], bottom_left[0], bottom_left[1],
                   bottom_left[2], bottom_right[0], bottom_right[1],
                   bottom_right[2]);
      ok = false;
    }
  }

  dol_aurora_shutdown();
  if (!ok) {
    std::fprintf(stderr,
                 "destination alpha fixture: FAIL plans=%llu skipped=%llu\n",
                 counters.draws_planned, counters.draws_skipped);
    return 1;
  }
  std::printf("destination alpha fixture: PASS %ux%u\n", width, height);
  return 0;
}
