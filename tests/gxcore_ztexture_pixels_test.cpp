// Public GPU fixture for Wind Waker's late ZTexture EFB-clear path.
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

void configure_pos_tex(gxc::GxCoreState& state) {
  state.apply({.kind = ar::RenderStateKind::CpVcd,
               .index = 0u,
               .value = 1u << 9u});
  state.apply({.kind = ar::RenderStateKind::CpVcd,
               .index = 1u,
               .value = 1u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 1u | (4u << 1u) | (1u << 21u) | (4u << 22u),
               .aux0 = 0u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 0u,
               .aux0 = 1u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 0u,
               .aux0 = 2u});
}

void configure_pos_color(gxc::GxCoreState& state) {
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
}

constexpr std::array<std::uint8_t, 64> kWindWakerClearZ = {
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

gxc::DrawPlan build_z_clear(gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 1u));
  state.apply(bp(0x40u, 0x1Fu)); // enabled, Always, write
  state.apply(bp(0x41u, (1u << 3u) | (1u << 4u)));
  state.apply(bp(0x43u, 0u)); // late Z compare
  state.apply(bp(0xC0u, 8u | (15u << 4u) | (15u << 8u) |
                              (15u << 12u) | (1u << 19u)));
  state.apply(bp(0xC1u, (4u << 4u) | (7u << 7u) | (7u << 10u) |
                              (7u << 13u) | (1u << 19u)));
  state.apply(bp(0x28u, 1u << 6u));
  state.apply(bp(0x30u, 3u));
  state.apply(bp(0x31u, 3u));
  state.apply(bp(0x80u, 0u));
  state.apply(bp(0xF3u, (7u << 16u) | (7u << 19u)));
  state.apply(bp(0xF4u, 0u));
  state.apply(bp(0xF5u, 2u | (2u << 2u))); // U24, REPLACE
  configure_pos_tex(state);

  ar::ConsumedDraw draw{};
  draw.primitive = 0x80u;
  draw.vertex_count = 4u;
  draw.vertex_size = 20u;
  configure_transform(draw);
  constexpr float vertices[4][5] = {
      {-1.0f, -1.0f, 0.5f, 0.0f, 0.0f},
      {1.0f, -1.0f, 0.5f, 1.0f, 0.0f},
      {1.0f, 1.0f, 0.5f, 1.0f, 1.0f},
      {-1.0f, 1.0f, 0.5f, 0.0f, 1.0f},
  };
  for (const auto& vertex : vertices)
    for (float value : vertex)
      append_be_f32(draw.vertex_payload, value);
  draw.xf_regs[0x00u] = 60u << 6u;
  draw.xf_regs[0x27u] = 1u;
  draw.xf_regs[0x28u] = 5u << 7u;
  draw.xf_reg_mask =
      (1ull << 0x00u) | (1ull << 0x27u) | (1ull << 0x28u);
  draw.tex_matrices[10][0] = 1.0f;
  draw.tex_matrices[10][5] = 1.0f;
  draw.tex_matrices[10][10] = 1.0f;
  draw.tex_matrix_word_mask[10] = 0xFFFu;
  draw.texture.valid = true;
  draw.texture.resolved = true;
  draw.texture.address = 0x80100000u;
  draw.texture.size = static_cast<std::uint32_t>(kWindWakerClearZ.size());
  draw.texture.format = 0x6u; // Z24X8 samples through the RGBA8 storage layout
  draw.texture.width = 4u;
  draw.texture.height = 4u;
  draw.texture.host_data = kWindWakerClearZ.data();
  draw.texture.host_available =
      static_cast<std::uint32_t>(kWindWakerClearZ.size());
  return state.build_draw_plan(draw, counters);
}

gxc::DrawPlan build_same_depth_green(gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 0u));
  state.apply(bp(0x40u, 0x13u)); // enabled, Less, write
  state.apply(bp(0x41u, (1u << 3u) | (1u << 4u)));
  configure_pos_color(state);

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
    draw.vertex_payload.insert(draw.vertex_payload.end(),
                               {0u, 255u, 0u, 255u});
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
      .app_name = "BlueWake ZTexture fixture",
      .window_width = 640,
      .window_height = 480,
      .vsync = false,
      .allow_texture_dumps = false,
      .info_logging = false,
      .graphics_logging = false,
      .force_untextured = false,
  };
  char arg0[] = "bluewake_gxcore_ztexture_pixels_test";
  char* argv[] = {arg0, nullptr};
  if (!dol_aurora_initialize(1, argv, &config))
    return 2;

  aurora::gfx::gxcore::reset_texture_cache();
  gxc::GapCounters counters{};
  const auto zclear = build_z_clear(counters);
  const auto green = build_same_depth_green(counters);
  bool ok = zclear.ok && green.ok && counters.ztexture_active == 1u &&
            counters.ztexture_ignored == 0u &&
            zclear.pipeline.shader.ztex_op == 2u &&
            zclear.pipeline.shader.ztex_type == 2u;
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(zclear);
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
    const std::size_t offset =
        (static_cast<std::size_t>(height / 2u) * width + width / 2u) * 4u;
    const std::uint8_t* pixel = rgba + offset;
    if (!near(pixel[0], 0) || !near(pixel[1], 255) || !near(pixel[2], 0)) {
      std::fprintf(stderr,
                   "ZTexture fixture: pixel=(%u,%u,%u,%u), expected green\n",
                   pixel[0], pixel[1], pixel[2], pixel[3]);
      ok = false;
    }
  }

  dol_aurora_shutdown();
  if (!ok) {
    std::fprintf(stderr,
                 "ZTexture fixture: FAIL active=%llu ignored=%llu\n",
                 counters.ztexture_active, counters.ztexture_ignored);
    return 1;
  }
  std::printf("ZTexture fixture: PASS %ux%u\n", width, height);
  return 0;
}
