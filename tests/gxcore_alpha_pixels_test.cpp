// Public GPU fixture for the native Outset alpha-compositing hypothesis.
#include "gxruntime/aurora_backend.h"
#include "gxruntime/aurora_recomp/render_sink.hpp"
#include "gxruntime/gxcore/gxcore.hpp"

#include "gfx/gxcore_draw.hpp"

#include <aurora/gfx.h>

#include <array>
#include <chrono>
#include <cmath>
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

gxc::DrawPlan build_background(gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 0u));
  state.apply(bp(0x40u, 0u));
  state.apply(bp(0x41u, (1u << 3u) | (1u << 4u)));
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
      {-1.0f, -1.0f, 0.0f},
      {1.0f, -1.0f, 0.0f},
      {1.0f, 1.0f, 0.0f},
      {-1.0f, 1.0f, 0.0f},
  };
  for (const auto& position : positions) {
    for (float value : position)
      append_be_f32(draw.vertex_payload, value);
    draw.vertex_payload.insert(draw.vertex_payload.end(), {0u, 0u, 255u, 255u});
  }
  return state.build_draw_plan(draw, counters);
}

std::array<std::uint8_t, 64> rgba8_texture(bool alpha_mask) {
  std::array<std::uint8_t, 64> texture{};
  constexpr std::uint8_t alpha[4] = {0u, 0u, 128u, 255u};
  for (std::uint32_t y = 0; y < 4u; ++y) {
    for (std::uint32_t x = 0; x < 4u; ++x) {
      const std::size_t ar = static_cast<std::size_t>(y) * 8u + x * 2u;
      const std::size_t gb = 32u + static_cast<std::size_t>(y) * 8u + x * 2u;
      texture[ar] = alpha_mask ? alpha[x] : 255u;
      texture[ar + 1u] = 255u;
      texture[gb] = alpha_mask ? 255u : 0u;
      texture[gb + 1u] = alpha_mask ? 255u : 0u;
    }
  }
  return texture;
}

gxc::DrawPlan build_alpha_quad(const std::array<std::uint8_t, 64>& color_texture,
                               const std::array<std::uint8_t, 64>& alpha_texture,
                               gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 2u | (1u << 10u)));
  state.apply(bp(0x40u, 0u));
  state.apply(bp(0x41u, 1u | (5u << 5u) | (4u << 8u) | (1u << 3u) |
                              (1u << 4u)));
  state.apply(bp(0xC0u, 15u | (10u << 4u) | (8u << 8u) | (15u << 12u) |
                              (1u << 19u)));
  state.apply(bp(0xC1u, (7u << 4u) | (5u << 7u) | (4u << 10u) |
                              (7u << 13u) | (1u << 19u)));
  state.apply(bp(0xC2u, 15u | (8u << 4u) | (0u << 8u) | (15u << 12u) |
                              (1u << 19u)));
  state.apply(bp(0xC3u, (7u << 4u) | (4u << 7u) | (0u << 10u) |
                              (7u << 13u) | (1u << 19u)));
  state.apply(bp(0x28u, (1u << 6u) | (1u << 12u) | (1u << 15u) |
                              (1u << 18u)));
  // Clamp both sampled texmaps while retaining linear min/mag filtering.
  state.apply(bp(0x80u, (1u << 4u) | (4u << 5u)));
  state.apply(bp(0x81u, (1u << 4u) | (4u << 5u)));
  // Deliberately decouple texcoord 1's S scale from the 4x4 image: this is the
  // hardware distinction the SU oracle protects. Texcoord 0 is 4x4; texcoord
  // 1 rasterizes S at scale 3 and T at scale 4.
  state.apply(bp(0x30u, 3u));
  state.apply(bp(0x31u, 3u));
  state.apply(bp(0x32u, 2u));
  state.apply(bp(0x33u, 3u));
  state.apply(bp(0xF6u, 1u << 2u));
  state.apply(bp(0xF7u, 2u | (3u << 2u)));
  state.apply(bp(0xF3u, (4u << 16u) | (7u << 19u)));
  state.apply({.kind = ar::RenderStateKind::CpVcd,
               .index = 0u,
               .value = 1u << 9u});
  state.apply({.kind = ar::RenderStateKind::CpVcd,
               .index = 1u,
               .value = 1u | (1u << 2u)});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 1u | (4u << 1u) | (1u << 21u) | (4u << 22u),
               .aux0 = 0u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 1u | (4u << 1u),
               .aux0 = 1u});
  state.apply({.kind = ar::RenderStateKind::CpVat,
               .index = 0u,
               .value = 0u,
               .aux0 = 2u});

  ar::ConsumedDraw draw{};
  draw.primitive = 0x80u;
  draw.vertex_count = 4u;
  draw.vertex_size = 28u;
  configure_transform(draw);
  constexpr float vertices[4][5] = {
      {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f},
      {1.0f, -1.0f, 0.0f, 2.0f, 0.0f},
      {1.0f, 1.0f, 0.0f, 2.0f, 1.0f},
      {-1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
  };
  for (const auto& vertex : vertices) {
    for (float value : vertex)
      append_be_f32(draw.vertex_payload, value);
    append_be_f32(draw.vertex_payload, vertex[3]);
    append_be_f32(draw.vertex_payload, vertex[4]);
  }
  draw.xf_regs[0x00u] = (60u << 6u) | (60u << 12u);
  draw.xf_regs[0x27u] = 2u;
  draw.xf_regs[0x28u] = 5u << 7u;
  draw.xf_regs[0x29u] = 6u << 7u;
  draw.xf_reg_mask = (1ull << 0x00u) | (1ull << 0x27u) | (1ull << 0x28u) |
                     (1ull << 0x29u);
  draw.tex_matrices[10][0] = 1.0f;
  draw.tex_matrices[10][5] = 1.0f;
  draw.tex_matrices[10][10] = 1.0f;
  draw.tex_matrix_word_mask[10] = 0xFFFu;
  draw.texture.valid = true;
  draw.texture.resolved = true;
  draw.texture.address = 0x80100000u;
  draw.texture.size = static_cast<std::uint32_t>(color_texture.size());
  draw.texture.format = 0x6u;
  draw.texture.width = 4u;
  draw.texture.height = 4u;
  draw.texture.host_data = color_texture.data();
  draw.texture.host_available = static_cast<std::uint32_t>(color_texture.size());
  draw.textures[0] = draw.texture;
  draw.textures[1] = draw.texture;
  draw.textures[1].address = 0x80100100u;
  draw.textures[1].host_data = alpha_texture.data();
  return state.build_draw_plan(draw, counters);
}

bool near(std::uint8_t actual, int expected, int tolerance = 2) {
  return std::abs(static_cast<int>(actual) - expected) <= tolerance;
}

bool check_rgb(const std::uint8_t* rgba, std::uint32_t width,
               std::uint32_t height, float x_rate, int red, int green,
               int blue, const char* label, int tolerance = 2) {
  const std::uint32_t x = static_cast<std::uint32_t>(width * x_rate);
  const std::uint32_t y = height / 2u;
  const std::uint8_t* pixel = rgba + (static_cast<std::size_t>(y) * width + x) * 4u;
  if (near(pixel[0], red, tolerance) && near(pixel[1], green, tolerance) &&
      near(pixel[2], blue, tolerance))
    return true;
  std::fprintf(stderr,
               "alpha fixture: %s pixel=(%u,%u,%u,%u), expected=(%d,%d,%d)\n",
               label, pixel[0], pixel[1], pixel[2], pixel[3], red, green, blue);
  return false;
}

} // namespace

int main() {
  setenv("AURORA_SYNC_PIPELINES", "1", 1);
  const AuroraBackendConfig config = {
      .app_name = "BlueWake alpha fixture",
      .window_width = 640,
      .window_height = 480,
      .vsync = false,
      .allow_texture_dumps = false,
      .info_logging = false,
      .graphics_logging = false,
      .force_untextured = false,
  };
  char arg0[] = "bluewake_gxcore_alpha_pixels_test";
  char* argv[] = {arg0, nullptr};
  if (!dol_aurora_initialize(1, argv, &config)) {
    std::fprintf(stderr, "alpha fixture: Aurora initialization failed\n");
    return 2;
  }

  aurora::gfx::gxcore::reset_texture_cache();
  gxc::GapCounters counters{};
  const auto color_texture = rgba8_texture(false);
  const auto alpha_texture = rgba8_texture(true);
  const gxc::DrawPlan background = build_background(counters);
  const gxc::DrawPlan alpha_quad =
      build_alpha_quad(color_texture, alpha_texture, counters);
  bool ok = background.ok && alpha_quad.ok && alpha_quad.texmap_mask == 0x3u &&
            counters.texcoord_scale_active == 1u &&
            counters.texcoord_scale_mismatch == 1u;
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(background);
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(alpha_quad);

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
    std::fprintf(stderr, "alpha fixture: framebuffer readback timed out\n");
    ok = false;
  } else {
    ok = check_rgb(rgba, width, height, 0.125f, 0, 0, 255, "alpha-0") && ok;
    ok = check_rgb(rgba, width, height, 0.3125f, 45, 0, 210,
                   "su-scale-3-midpoint-a", 3) && ok;
    ok = check_rgb(rgba, width, height, 0.4375f, 141, 0, 114,
                   "su-scale-3-midpoint-b", 3) && ok;
    ok = check_rgb(rgba, width, height, 0.750f, 255, 0, 0,
                   "clamp-outside-u") && ok;
  }

  dol_aurora_shutdown();
  if (!ok) {
    std::fprintf(stderr, "alpha fixture: FAIL plans=%llu skipped=%llu\n",
                 counters.draws_planned, counters.draws_skipped);
    return 1;
  }
  std::printf("alpha fixture: PASS %ux%u\n", width, height);
  return 0;
}
