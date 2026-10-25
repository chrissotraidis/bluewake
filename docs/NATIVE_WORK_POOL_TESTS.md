# Native work-pool test coverage

## Contents

- [Scope and review order](#scope-and-review-order)
- [Coverage map](#coverage-map)
- [Run the public tests](#run-the-public-tests)
- [Run the private oracle](#run-the-private-oracle)
- [Validation and limitations](#validation-and-limitations)
- [Traceability](#traceability)

Architecture: [Overview](architecture/Overview.md) · [Host](architecture/Host.md)

## Scope and review order

[#397](https://github.com/chrissotraidis/bluewake/pull/397) supplies the combined plan and pool/build
foundation. [#424](https://github.com/chrissotraidis/bluewake/pull/424) consolidates the remaining focused coverage from
#406–#411. It adds no game optimization, worker-default change or new offload.
Use it when a concrete optimization needs these contracts, not as a blanket gate.

The failure-injection environment variable is compiled only with
`BLUEWAKE_POOL_TESTING`. The versioned statistics getter is a production-module
diagnostic API, not a descriptor field; it reports existing counters and must be
read at a quiescent game-thread boundary, not treated as a concurrent telemetry API.

## Coverage map

| Contract | Source | When it runs | Evidence / limit |
|---|---|---|---|
| Real partitions, publication, repeated work and configured participant counts | [Pool harness](../tests/native_work_pool_test.c), [registration](../tests/native_work_pool/CMakeLists.txt) | Standalone CTest, Linux source-only CTest, sanitizer jobs | Exact visits, output and sentinels; synthetic inputs only |
| Complete/partial worker-creation failure | [Test-only pool seam](../cmake/composite/native_work_pool.c), [harness](../tests/native_work_pool_test.c) | Fresh CTest processes with failure after 0 or 2 created workers | Effective counts and correct fallback/output |
| Repeated shared-library load/work/unload | [Lifecycle harness](../tests/native_work_pool_lifecycle_test.c), [DSO fixture](../tests/native_work_pool_fixture.c) | POSIX standalone/Linux CTest and sanitizer jobs | Workers finish and join before unload; bounded timeout |
| Thresholds, uneven/non-identity data, decline and routed fallback | [Native-math guard](../tests/native_math_guard_test.c) | Host/Linux guard tests; Windows serial guard | Synthetic CPU/memory assertions, not gameplay |
| Translated, serial-native and candidate-module agreement | [Private oracle](../tests/native_math_test.c), [shared target](../cmake/native_math_tests.cmake) | Explicit manual execution with own-disc modules | Required accepted/declined/compared categories; never public CI |
| Native-path activation and effective pool settings | [Stats API](../cmake/composite/native_math.h), [implementation](../cmake/composite/native_math.c) | Guard assertions and explicit diagnostic callers | Candidate-local counters distinguish requested workers from actual work |
| Memory/undefined behavior and races | [Sanitizer workflow](../.github/workflows/gamethread-sanitizers.yml) | Separate ASan/UBSan and TSan jobs | Production pool with synthetic inputs; no game-wide race claim |

## Run the public tests

Requires CMake 3.20+, a C compiler and a build tool; POSIX workers use pthreads.
No RecompCore, renderer, ISO or game module is needed for this standalone suite.

```bash
cmake -S tests/native_work_pool -B build/native-work-pool -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native-work-pool --parallel
ctest --test-dir build/native-work-pool --output-on-failure -L native-work-pool
```

Linux/POSIX registers nine cases. Windows standalone registers the serial case;
its ordinary platform graph retains the existing serial native-math guard.
Results are in `build/native-work-pool/Testing/Temporary/LastTest.log`.

For the Linux-integrated graph, use the pinned runtime and the compiler/provider
configuration from the [Linux workflow](../.github/workflows/linux-host.yml), with
`BUILD_TESTING=OFF` and `BLUEWAKE_LINUX_REGRESSION_TESTS=ON`. Then:

```bash
cmake --build build/linux-host --parallel --target \
  bluewake_native_math_guard_test bluewake_native_work_pool_test \
  bluewake_native_work_pool_lifecycle_test
ctest --test-dir build/linux-host --output-on-failure \
  -R 'bluewake_native_(work_pool|math_(guard|workers))'
```

The lifecycle executable depends on its DSO fixture. CI must build the executable
explicitly: CTest registration alone does not build either artifact. This fixes
the missing-executable failure seen on #412 and #417; their deferred tooling is
not required for the fix.

To reproduce sanitizer checks, use the exact configure/run commands in the
[sanitizer workflow](../.github/workflows/gamethread-sanitizers.yml). Keep TSan
separate from ASan/UBSan. `detect_leaks=0` disables leak checking only; do not
report that configuration as a LeakSanitizer pass.

## Run the private oracle

After configuring the host or Linux graph with its pinned runtime:

```bash
cmake --build build/linux-host --target bluewake_native_math_test --parallel
BLUEWAKE_NATIVE_WORKERS=2 build/linux-host/bluewake_native_math_test \
  /absolute/path/reference-module.so --candidate /absolute/path/candidate-module.so
```

Use distinct, ABI-compatible own-disc reference and candidate modules. The
reference supplies translated behavior; the executable owns the serial-native
path, and the candidate DSO owns its worker pool. The oracle requires comparison
coverage instead of accepting a run that declined every native case. A no-argument
run only checks synthetic fallback; it is not a three-path oracle pass.

Keep generated modules and data local. Public synthetic tests cannot prove
private oracle agreement, gameplay equivalence or a Steam Deck speedup.

## Validation and limitations

Consolidation validation on a Linux desktop, Ubuntu 24.04 Docker image,
Clang 18.1.3, Debug, `-Wall -Wextra -Werror`:

| Run | Result |
|---|---|
| Targeted build of pool and lifecycle executables, including DSO dependency | Passed |
| Standalone CTest | 9/9 passed |
| ASan/UBSan, leak detection disabled | 9/9 passed |
| TSan | 9/9 passed |
| Native-math guard, serial and W=2 (direct Clang 18 build against the existing pinned runtime library) | Both passed; 0 and 5 parallel batches respectively |
| Private-oracle executable, no arguments | Synthetic fallback checks passed; no private differential run |

Full Linux/Windows CI and private module/gameplay/Deck results must be reported
separately for the reviewed head. No speedup or complete hardware qualification
is claimed by this consolidation. Performance changes still follow the
[benchmark tour](BENCHMARK_TOUR.md) and [performance strategy](PERFORMANCE.md).

## Traceability

| Earlier PR | Coverage retained here |
|---|---|
| [#406](https://github.com/chrissotraidis/bluewake/pull/406) | Synthetic native-math boundaries |
| [#407](https://github.com/chrissotraidis/bluewake/pull/407) | Three-path oracle, including the later required-serial-coverage correction |
| [#408](https://github.com/chrissotraidis/bluewake/pull/408) | Worker creation failures |
| [#409](https://github.com/chrissotraidis/bluewake/pull/409) | DSO lifecycle |
| [#410](https://github.com/chrissotraidis/bluewake/pull/410) | Activation statistics |
| [#411](https://github.com/chrissotraidis/bluewake/pull/411) | Sanitizer lanes and participant assertions |

The original commits/branches remain available. #412–#417 qualification tools
and [#421](https://github.com/chrissotraidis/bluewake/pull/421)'s broader plan are
deferred, not dependencies. [#397](https://github.com/chrissotraidis/bluewake/pull/397)
now contains that foundation; #399 is superseded, and #396/#400 remain independent.
