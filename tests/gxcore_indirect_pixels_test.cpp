// Public GPU fixture for gxcore TEV indirect-texture coordinate displacement.
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

std::array<std::uint8_t, 64> rgba8_stripes() {
  std::array<std::uint8_t, 64> texture{};
  constexpr std::uint8_t colors[4][3] = {
      {255u, 0u, 0u}, {0u, 255u, 0u}, {0u, 0u, 255u}, {255u, 255u, 255u}};
  for (std::uint32_t y = 0; y < 4u; ++y) {
    for (std::uint32_t x = 0; x < 4u; ++x) {
      const std::size_t ar = static_cast<std::size_t>(y) * 8u + x * 2u;
      const std::size_t gb = 32u + static_cast<std::size_t>(y) * 8u + x * 2u;
      texture[ar] = 255u;
      texture[ar + 1u] = colors[x][0];
      texture[gb] = colors[x][1];
      texture[gb + 1u] = colors[x][2];
    }
  }
  return texture;
}

std::array<std::uint8_t, 64> rgba8_indirect_offset() {
  std::array<std::uint8_t, 64> texture{};
  for (std::uint32_t y = 0; y < 4u; ++y) {
    for (std::uint32_t x = 0; x < 4u; ++x) {
      const std::size_t ar = static_cast<std::size_t>(y) * 8u + x * 2u;
      const std::size_t gb = 32u + static_cast<std::size_t>(y) * 8u + x * 2u;
      texture[ar] = 8u;
      texture[ar + 1u] = 0u;
      texture[gb] = 0u;
      texture[gb + 1u] = 0u;
    }
  }
  return texture;
}

gxc::DrawPlan build_indirect_quad(
    const std::array<std::uint8_t, 64>& color_texture,
    const std::array<std::uint8_t, 64>& indirect_texture,
    gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  // One texgen, one TEV stage, and one indirect stage.
  state.apply(bp(0x00u, 1u | (1u << 16u)));
  state.apply(bp(0x40u, 0u));
  state.apply(bp(0x41u, (1u << 3u) | (1u << 4u)));
  // TEV stage 0 emits the regular texture sample unchanged.
  state.apply(bp(0xC0u, 8u | (15u << 4u) | (15u << 8u) |
                              (15u << 12u) | (1u << 19u)));
  state.apply(bp(0xC1u, (4u << 4u) | (7u << 7u) | (7u << 10u) |
                              (7u << 13u) | (1u << 19u)));
  state.apply(bp(0x28u, 1u << 6u));
  state.apply(bp(0xF6u, 1u << 2u));
  state.apply(bp(0xF7u, 2u | (3u << 2u)));
  // Indirect stage 0 samples texmap 1 with texcoord 0 at scale 1.
  state.apply(bp(0x25u, 0u));
  state.apply(bp(0x27u, 1u));
  // Texcoord 0 rasterizes against the 4x4 source dimensions.
  state.apply(bp(0x30u, 3u));
  state.apply(bp(0x31u, 3u));
  // Matrix 0: A=128, remaining coefficients zero, scale=17. With an alpha
  // sample of 8 this yields (128*8)>>3 == 128 fixed-point units: one texel.
  state.apply(bp(0x06u, 128u | (1u << 22u)));
  state.apply(bp(0x07u, 0u));
  state.apply(bp(0x08u, 1u << 22u));
  state.apply(bp(0x10u, 1u << 9u));
  state.apply(bp(0xF3u, (7u << 16u) | (7u << 19u)));

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

  ar::ConsumedDraw draw{};
  draw.primitive = 0x80u;
  draw.vertex_count = 4u;
  draw.vertex_size = 20u;
  configure_transform(draw);
  constexpr float vertices[4][5] = {
      {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f},
      {1.0f, -1.0f, 0.0f, 1.0f, 0.0f},
      {1.0f, 1.0f, 0.0f, 1.0f, 1.0f},
      {-1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
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
  draw.texture.size = static_cast<std::uint32_t>(color_texture.size());
  draw.texture.format = 0x6u;
  draw.texture.width = 4u;
  draw.texture.height = 4u;
  draw.texture.host_data = color_texture.data();
  draw.texture.host_available = static_cast<std::uint32_t>(color_texture.size());
  draw.textures[0] = draw.texture;
  draw.textures[1] = draw.texture;
  draw.textures[1].address = 0x80100100u;
  draw.textures[1].host_data = indirect_texture.data();
  return state.build_draw_plan(draw, counters);
}

bool near(std::uint8_t actual, int expected, int tolerance = 5) {
  return std::abs(static_cast<int>(actual) - expected) <= tolerance;
}

bool check_rgb(const std::uint8_t* rgba, std::uint32_t width,
               std::uint32_t height, float x_rate, int red, int green,
               int blue, const char* label) {
  const std::uint32_t x = static_cast<std::uint32_t>(width * x_rate);
  const std::uint32_t y = height / 2u;
  const std::uint8_t* pixel = rgba + (static_cast<std::size_t>(y) * width + x) * 4u;
  if (near(pixel[0], red) && near(pixel[1], green) && near(pixel[2], blue))
    return true;
  std::fprintf(stderr,
               "indirect fixture: %s pixel=(%u,%u,%u,%u), expected=(%d,%d,%d)\n",
               label, pixel[0], pixel[1], pixel[2], pixel[3], red, green, blue);
  return false;
}

} // namespace

int main() {
  setenv("AURORA_SYNC_PIPELINES", "1", 1);
  const AuroraBackendConfig config = {
      .app_name = "BlueWake indirect fixture",
      .window_width = 640,
      .window_height = 480,
      .vsync = false,
      .allow_texture_dumps = false,
      .info_logging = false,
      .graphics_logging = false,
      .force_untextured = false,
  };
  char arg0[] = "bluewake_gxcore_indirect_pixels_test";
  char* argv[] = {arg0, nullptr};
  if (!dol_aurora_initialize(1, argv, &config)) {
    std::fprintf(stderr, "indirect fixture: Aurora initialization failed\n");
    return 2;
  }

  aurora::gfx::gxcore::reset_texture_cache();
  gxc::GapCounters counters{};
  const auto color_texture = rgba8_stripes();
  const auto indirect_texture = rgba8_indirect_offset();
  const gxc::DrawPlan quad =
      build_indirect_quad(color_texture, indirect_texture, counters);
  bool ok = quad.ok && quad.texmap_mask == 0x3u &&
            counters.indirect_active == 1u && counters.indirect_ignored == 0u &&
            quad.pixel_constants.indtexmtx[0][0] == 128 &&
            quad.pixel_constants.indtexmtx[0][3] == 0;
  ok = ok && aurora::gfx::gxcore::submit_draw_plan(quad);

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
    std::fprintf(stderr, "indirect fixture: framebuffer readback timed out\n");
    ok = false;
  } else {
    ok = check_rgb(rgba, width, height, 0.125f, 0, 255, 0,
                   "one-texel displacement") && ok;
  }

  dol_aurora_shutdown();
  if (!ok) {
    std::fprintf(stderr,
                 "indirect fixture: FAIL plans=%llu skipped=%llu ignored=%llu\n",
                 counters.draws_planned, counters.draws_skipped,
                 counters.indirect_ignored);
    return 1;
  }
  std::printf("indirect fixture: PASS %ux%u\n", width, height);
  return 0;
}
