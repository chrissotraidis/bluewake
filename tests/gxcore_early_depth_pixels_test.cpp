// Public GPU fixture for GXSetZCompLoc early depth delivery.
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

extern "C" void aurora_backend_present(void);

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

gxc::DrawPlan build_quad(std::array<std::uint8_t, 4> color,
                         bool depth_test, bool early_depth, bool alpha_fail,
                         gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 0u));
  // Strict Less makes the second same-depth quad fail after an early write.
  state.apply(bp(0x40u, depth_test ? 0x13u : 0u));
  state.apply(bp(0x41u, (1u << 3u) | (1u << 4u)));
  state.apply(bp(0x43u, early_depth ? 1u << 6u : 0u));
  if (alpha_fail) {
    // One TEV stage passes raster color/alpha through, then Never && Always.
    state.apply(bp(0xC0u, 10u | (15u << 4u) | (15u << 8u) |
                              (15u << 12u) | (1u << 19u)));
    state.apply(bp(0xC1u, 5u | (7u << 4u) | (7u << 7u) |
                              (7u << 10u) | (7u << 13u) | (1u << 19u)));
    state.apply(bp(0xF3u, 7u << 19u));
  }
  state.apply({.kind = ar::RenderStateKind::CpVcd,
               .index = 0u,
               .value = (1u << 9u) | (1u << 13u)});
  state.apply({.kind = ar::RenderStateKind::CpVcd, .index = 1u, .value = 0u});
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
  constexpr float positions[4][3] = {
      {-1.0f, -1.0f, 0.5f}, {1.0f, -1.0f, 0.5f},
      {1.0f, 1.0f, 0.5f},   {-1.0f, 1.0f, 0.5f},
  };
  for (const auto& position : positions) {
    for (float value : position)
      append_be_f32(draw.vertex_payload, value);
    draw.vertex_payload.insert(draw.vertex_payload.end(), color.begin(),
                               color.end());
  }
  return state.build_draw_plan(draw, counters);
}

bool near(std::uint8_t actual, int expected) {
  return static_cast<int>(actual) >= expected - 2 &&
         static_cast<int>(actual) <= expected + 2;
}

} // namespace

int main() {
  setenv("AURORA_SYNC_PIPELINES", "1", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);
  const AuroraBackendConfig config = {
      .app_name = "BlueWake early depth fixture",
      .window_width = 640,
      .window_height = 480,
      .vsync = false,
      .allow_texture_dumps = false,
      .info_logging = false,
      .graphics_logging = false,
      .force_untextured = false,
  };
  char arg0[] = "bluewake_gxcore_early_depth_pixels_test";
  char* argv[] = {arg0, nullptr};
  if (!dol_aurora_initialize(1, argv, &config))
    return 2;

  aurora::gfx::gxcore::reset_texture_cache();
  gxc::GapCounters counters{};
  const auto blue = build_quad({0u, 0u, 255u, 255u}, false, false, false,
                               counters);
  const auto discarded_red = build_quad({255u, 0u, 0u, 255u}, true, true,
                                        true, counters);
  const auto green = build_quad({0u, 255u, 0u, 255u}, true, false, false,
                                counters);
  bool ok = blue.ok && discarded_red.ok && green.ok;
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(blue);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(discarded_red);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(green);

  aurora_request_framebuffer_readback();
  aurora_backend_present();
  const std::uint8_t* rgba = nullptr;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  for (int attempt = 0; attempt < 20000 && rgba == nullptr; ++attempt) {
    aurora_take_framebuffer_readback(&rgba, &width, &height);
    if (rgba == nullptr) {
      aurora_pump_framebuffer_readback();
      std::this_thread::sleep_for(std::chrono::microseconds(250));
    }
  }
  if (rgba == nullptr) {
    ok = false;
  } else {
    const auto offset =
        (static_cast<std::size_t>(height / 2u) * width + width / 2u) * 4u;
    const std::uint8_t* pixel = rgba + offset;
    if (!near(pixel[0], 0) || !near(pixel[1], 0) || !near(pixel[2], 255)) {
      std::fprintf(stderr,
                   "early depth fixture: pixel=(%u,%u,%u,%u), expected blue\n",
                   pixel[0], pixel[1], pixel[2], pixel[3]);
      ok = false;
    }
  }

  dol_aurora_shutdown();
  if (!ok) {
    std::fprintf(stderr, "early depth fixture: FAIL active=%llu\n",
                 counters.early_depth_active);
    return 1;
  }
  std::printf("early depth fixture: PASS %ux%u\n", width, height);
  return 0;
}
