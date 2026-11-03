// Public GPU fixture for GX half-scale EFB copies.
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

void configure_pos_color(gxc::GxCoreState& state) {
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

gxc::DrawPlan solid_quad(float left, float top, float right, float bottom,
                         std::array<std::uint8_t, 4> color,
                         gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 0u));
  state.apply(bp(0x40u, 0u));
  state.apply(bp(0x41u, (1u << 3u) | (1u << 4u)));
  configure_pos_color(state);

  ar::ConsumedDraw draw{};
  draw.primitive = 0x80u;
  draw.vertex_count = 4u;
  draw.vertex_size = 16u;
  configure_transform(draw);
  const float positions[4][3] = {
      {left, top, 0.0f}, {right, top, 0.0f},
      {right, bottom, 0.0f}, {left, bottom, 0.0f},
  };
  for (const auto& position : positions) {
    for (float value : position)
      append_be_f32(draw.vertex_payload, value);
    draw.vertex_payload.insert(draw.vertex_payload.end(), color.begin(),
                               color.end());
  }
  return state.build_draw_plan(draw, counters);
}

gxc::DrawPlan copied_texture_quad(gxc::GapCounters& counters) {
  gxc::GxCoreState state;
  state.reset();
  state.apply(bp(0x00u, 1u));
  state.apply(bp(0x40u, 0u));
  state.apply(bp(0x41u, (1u << 3u) | (1u << 4u)));
  state.apply(bp(0xC0u, 8u | (15u << 4u) | (15u << 8u) |
                              (15u << 12u) | (1u << 19u)));
  state.apply(bp(0xC1u, (4u << 4u) | (7u << 7u) | (7u << 10u) |
                              (7u << 13u) | (1u << 19u)));
  state.apply(bp(0x28u, 1u << 6u));
  state.apply(bp(0x30u, 319u));
  state.apply(bp(0x31u, 239u));
  state.apply(bp(0x80u, 0u));
  state.apply(bp(0xF3u, (7u << 16u) | (7u << 19u)));
  configure_pos_tex(state);

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
  draw.texture.address = 0x80500000u;
  draw.texture.size = 320u * 240u * 4u;
  draw.texture.format = 6u;
  draw.texture.width = 320u;
  draw.texture.height = 240u;
  return state.build_draw_plan(draw, counters);
}

bool near(std::uint8_t actual, int expected) {
  return std::abs(static_cast<int>(actual) - expected) <= 3;
}

int color_id(const std::uint8_t* pixel) {
  if (near(pixel[0], 32) && near(pixel[1], 32) && near(pixel[2], 32))
    return 1;
  if (near(pixel[0], 96) && near(pixel[1], 96) && near(pixel[2], 96))
    return 2;
  if (near(pixel[0], 160) && near(pixel[1], 160) && near(pixel[2], 160))
    return 3;
  if (near(pixel[0], 224) && near(pixel[1], 224) && near(pixel[2], 224))
    return 4;
  return 0;
}

} // namespace

int main() {
  setenv("AURORA_SYNC_PIPELINES", "1", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);
  const AuroraBackendConfig config = {
      .app_name = "BlueWake EFB half-scale fixture",
      .window_width = 640,
      .window_height = 480,
      .vsync = false,
      .allow_texture_dumps = false,
      .info_logging = false,
      .graphics_logging = false,
      .force_untextured = false,
  };
  char arg0[] = "bluewake_gxcore_efb_half_scale_pixels_test";
  char* argv[] = {arg0, nullptr};
  if (!dol_aurora_initialize(1, argv, &config))
    return 2;

  aurora::gfx::gxcore::reset_texture_cache();
  gxc::GapCounters counters{};
  const std::array plans{
      solid_quad(-1.0f, -1.0f, 0.0f, 0.0f, {32u, 32u, 32u, 255u}, counters),
      solid_quad(0.0f, -1.0f, 1.0f, 0.0f, {96u, 96u, 96u, 255u}, counters),
      solid_quad(-1.0f, 0.0f, 0.0f, 1.0f, {160u, 160u, 160u, 255u}, counters),
      solid_quad(0.0f, 0.0f, 1.0f, 1.0f, {224u, 224u, 224u, 255u}, counters),
  };
  bool ok = true;
  for (const auto& plan : plans)
    ok = ok && plan.ok && aurora::gfx::gxcore::submit_draw_plan(plan);

  // The guest may reuse one destination for a differently shaped/formatted
  // copy. Poison the cache with an I8 target first; the RGBA8 copy below must
  // replace the active destination with a compatible texture.
  const gxc::EfbCopyCommand poison{
      .dest_address = 0x80500000u,
      .byte_size = 160u * 120u,
      .format = 1u,
      .src_x = 0u,
      .src_y = 0u,
      .width = 640u,
      .height = 480u,
      .destination_width = 160u,
      .destination_height = 120u,
      .clear_z = 0xFFFFFFu,
  };
  aurora::gfx::gxcore::copy_efb_to_texture(poison);

  const gxc::EfbCopyCommand copy{
      .dest_address = 0x80500000u,
      .byte_size = 320u * 240u * 4u,
      .format = 6u,
      .src_x = 0u,
      .src_y = 0u,
      .width = 640u,
      .height = 480u,
      .destination_width = 320u,
      .destination_height = 240u,
      .clear = true,
      .clear_z = 0xFFFFFFu,
  };
  aurora::gfx::gxcore::copy_efb_to_texture(copy);
  const auto sampled = copied_texture_quad(counters);
  ok = ok && sampled.ok && aurora::gfx::gxcore::submit_draw_plan(sampled);

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
    std::array<int, 4> ids{};
    std::array<std::array<std::uint8_t, 3>, 4> rgb{};
    const std::array<std::array<std::uint32_t, 2>, 4> samples{{
        {width / 4u, height / 4u},
        {3u * width / 4u, height / 4u},
        {width / 4u, 3u * height / 4u},
        {3u * width / 4u, 3u * height / 4u},
    }};
    for (std::size_t i = 0; i < samples.size(); ++i) {
      const auto [x, y] = samples[i];
      const auto* pixel =
          rgba + (static_cast<std::size_t>(y) * width + x) * 4u;
      ids[i] = color_id(pixel);
      rgb[i] = {pixel[0], pixel[1], pixel[2]};
    }
    std::array<bool, 5> seen{};
    for (int id : ids)
      if (id >= 1 && id <= 4)
        seen[static_cast<std::size_t>(id)] = true;
    ok = ok && seen[1] && seen[2] && seen[3] && seen[4];
    if (!ok)
      std::fprintf(stderr,
                   "EFB half-scale fixture: corner ids=%d,%d,%d,%d "
                   "rgb=(%u,%u,%u)/(%u,%u,%u)/(%u,%u,%u)/(%u,%u,%u), "
                   "expected all four colors\n",
                   ids[0], ids[1], ids[2], ids[3], rgb[0][0], rgb[0][1],
                   rgb[0][2], rgb[1][0], rgb[1][1], rgb[1][2], rgb[2][0],
                   rgb[2][1], rgb[2][2], rgb[3][0], rgb[3][1], rgb[3][2]);
  }

  dol_aurora_shutdown();
  return ok ? 0 : 1;
}
