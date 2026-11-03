#include "gfx/staging_budget.hpp"

#include <cassert>

using aurora::gfx::StagingLimits;
using aurora::gfx::StagingRequest;
using aurora::gfx::StagingUse;
using aurora::gfx::staging_fits;
using aurora::gfx::staging_project;

int main() {
  const StagingLimits limits{
      .capacity = StagingUse{.verts = 1024,
                             .indices = 1024,
                             .uniforms = 4096,
                             .storage = 1024},
      .uniformAlignment = 256,
      .storageAlignment = 256,
      .finishUniformHeadroom = 64,
  };
  const StagingRequest draw{.verts = 96,
                            .indices = 12,
                            .uniforms = 0x910,
                            .secondUniforms = 0xF0};

  assert(staging_fits(StagingUse{}, draw, limits));

  const StagingUse nearlyFull{.uniforms = 2048};
  assert(!staging_fits(nearlyFull, draw, limits));
  assert(staging_fits(StagingUse{}, draw, limits));

  const StagingUse projected = staging_project(StagingUse{}, draw, limits);
  assert(projected.uniforms == 0xB30);

  const StagingRequest exact{.uniforms = 4032};
  assert(staging_fits(StagingUse{}, exact, limits));
  const StagingRequest noFinishRoom{.uniforms = 4033};
  assert(!staging_fits(StagingUse{}, noFinishRoom, limits));
  return 0;
}
