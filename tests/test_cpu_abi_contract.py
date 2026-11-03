import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
COMPOSITE = ROOT / "generated" / "full" / "composite-lib"
CPU_HEADER = ROOT / "ref" / "recompcore" / "GXRuntime" / "include" / "core" / "cpu.h"
GENERATED_HEADER = COMPOSITE / "generated_composite.h"
GENERATED_CHUNK = COMPOSITE / "chunks_dol" / "chunk_0192_text1_803016E0.c"
START_DMA_CHUNK = COMPOSITE / "chunks_dol" / "chunk_0173_text1_802B56E0.c"
RETAIL_ARAM_PIECE = ROOT / "ref" / "tww" / "src" / "JSystem" / "JKernel" / "JKRAramPiece.cpp"
DOLRECOMP_CYCLE_PATCH = (
    ROOT / "patches" / "dolrecomp" /
    "0003-backend-emit-cycle-observation-suffixes.patch"
)


def test_full_composite_emits_cycle_charges():
    chunks = sorted(COMPOSITE.rglob("*.c"))
    assert len(chunks) == 748
    missing = [path for path in chunks if "downcount -=" not in path.read_text()]
    assert not missing, f"translated chunks without downcount charges: {missing[:3]}"


def test_cpu_abi_keeps_charge_fields_at_tail():
    text = CPU_HEADER.read_text()
    assert "#define GXRUNTIME_CPU_ABI_VERSION 6u" in text
    assert re.search(r"s64 downcount;\s*\n\s*s64 cycle_budget;", text)
    assert "u32 cycle_observation_suffix;" in text
    assert "u32 cycle_deadline_active;" in text
    assert "s64 cycle_deadline_budget;" in text


def test_host_validates_module_cpu_abi_before_execution():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "mod->cpu_abi_version != GXRUNTIME_CPU_ABI_VERSION" in host
    assert "mod->cpu_state_size != sizeof(CPUState)" in host
    assert "module ABI mismatch" in host


def test_promoted_host_clock_converts_cycles_to_timebase_ticks():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "#define GUEST_CYCLES_PER_TIMEBASE_TICK 12u" in host
    assert "g_guest_clock_cycle_remainder" in host
    assert "cycle_total / GUEST_CYCLES_PER_TIMEBASE_TICK" in host


def test_scheduler_context_coherence_is_default_on_and_not_thread_specific():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    contract = (
        ROOT / "runtime" / "host" / "src" / "scheduler_contract.c"
    ).read_text()
    assert "bluewake_scheduler_save_exception_context(cpu, context);" in host
    assert "bluewake_scheduler_interrupt_safe(cpu.pc)" in host
    assert "context == 0x803E9260u" not in contract
    assert "OS_SAVE_CONTEXT_CONTINUATION" in contract
    for rejected_hook in (
        "BLUEWAKE_WAKE_DEFAULT",
        "BLUEWAKE_WAKE_DVD",
        "BLUEWAKE_LOAD_DVD_CONTEXT",
        "BLUEWAKE_RESUME_SCHEDULER",
    ):
        assert rejected_hook not in host


def test_generated_scheduler_contract_runs_retail_message_entry_points():
    test = (ROOT / "tests" / "generated_scheduler_contract_test.c").read_text()
    cmake = (ROOT / "runtime" / "host" / "CMakeLists.txt").read_text()
    assert "#define OS_SEND_MESSAGE 0x80305908u" in test
    assert "#define OS_RECEIVE_MESSAGE 0x803059D0u" in test
    assert "dispatch_until(module, &cpu, sender_resume);" in test
    assert "dispatch_until(module, &cpu, receiver_return);" in test
    assert "bluewake_generated_scheduler_contract_test" in cmake


def test_generated_aram_stream_contract_runs_retail_loop_and_completion():
    test = (ROOT / "tests" / "generated_aram_stream_contract_test.c").read_text()
    cmake = (ROOT / "runtime" / "host" / "CMakeLists.txt").read_text()
    assert "#define JKR_ARAM_STREAM_WRITE_TO_ARAM 0x802B637Cu" in test
    assert "#define JKR_ARAM_PCS 0x802B5ED4u" in test
    assert "state.dma_bytes == total_size" in test
    assert "mem_read32(&cpu, message_slot) == total_size" in test
    assert "bluewake_generated_aram_stream_contract_test" in cmake


def test_host_aram_dma_is_register_owned_and_default_on():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    dma = (ROOT / "runtime" / "host" / "src" / "aram_dma.c").read_text()
    cmake = (ROOT / "runtime" / "host" / "CMakeLists.txt").read_text()
    assert "const bool accepted = bluewake_aram_dma_write(" in host
    assert "&g_aram_dma, ctx, address, size, value" in host
    assert "bluewake_aram_dma_interrupt_pending(&g_aram_dma)" in host
    assert "[aram-dma] summary transfers=" in host
    assert "arq_post_active" not in host
    assert "arq_interrupt_active" not in host
    assert "BLUEWAKE_ARAM_DMA_BASE" in dma
    assert "aram_dma_to_aram(cpu->ram" in dma
    assert "aram_dma_to_ram(cpu->ram" in dma
    assert "bluewake_aram_dma_contract_test" in cmake


def test_host_retains_default_dvd_lifecycle_summary():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "dvd_jas_forward_failures" in host
    assert "dvd_jas_active_command" in host
    assert '"[dvd-lifecycle] forwards=%llu failures=%llu' in host


def test_dsp_lle_uses_donor_slice_cadence_and_cycle_ratio():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "#define DSP_LLE_UPDATE_RATE 12600u" in host
    assert "#define DSP_LLE_MAIL_SLICE 72u" in host
    assert "#define DSP_LLE_CONTROL_MASK 0x0C07u" in host
    assert "const u64 dsp_cycles = cpu_cycles / 6u;" in host
    assert "host_dsp_sync_mailbox_read(ctx);" in host
    assert "host_dsp_advance_schedule(elapsed_cycles);" in host


def test_dsp_mailbox_slice_publishes_interrupt_before_read_returns():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    start = host.index("static void host_dsp_sync_mailbox_read(CPUState* cpu)")
    end = host.index("static void host_dsp_advance_schedule", start)
    mailbox_sync = host[start:end]

    run = mailbox_sync.index("host_dsp_run_cpu_cycles(DSP_LLE_MAIL_SLICE);")
    publish = mailbox_sync.index("host_refresh_interrupt_sources(cpu);", run)
    assert run < publish

    read_start = host.index("static u64 host_mmio_read(")
    read_end = host.index("static void host_mmio_write(", read_start)
    mmio_read = host[read_start:read_end]
    assert mmio_read.count("host_dsp_sync_mailbox_read(ctx);") == 2


def test_dsp_work_completion_is_guest_callback_owned():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    retail = (ROOT / "ref" / "tww" / "src" / "JSystem" / "JAudio" /
              "dspproc.c").read_text()

    assert "void setup_callback(u16 param_1)" in retail
    assert "void dummy_callback(u16 param_1)" in retail
    assert "flag = 0;" in retail
    assert "d_waitflag = 0;" in retail
    assert "cpu.pc == 0x8028E708u" not in host
    assert "cpu.pc == 0x8028E838u" not in host
    assert "setup callback completion flag=0" not in host
    assert "sync callback completion flag=0" not in host


def test_vi_and_audio_advance_from_generated_guest_cycles():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "dol_vi_clock_advance(g_cycle_vi_clock, elapsed_cycles);" in host
    assert "host_cycle_cursor_delta(&g_vi_cycle_cursor)" in host
    assert "host_cycle_cursor_delta(&g_audio_cycle_cursor)" in host
    assert "dol_audio_dma_consume_pcm16_stereo_work(" in host
    assert "&g_audio_dma, elapsed_cycles" in host
    assert "k_host_retrace_blocks" not in host
    assert "adapter_control & DSP_LLE_CONTROL_MASK" in host


def test_mmio_writes_publish_deadlines_created_by_the_write():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    start = host.index("static void host_mmio_write(")
    end = host.index("static bool host_graphics_guest_resolve(", start)
    mmio_write = host[start:end]
    observe = mmio_write.index("bluewake_cycle_domain_observe(")
    rebudget = mmio_write.index("rebudget:", observe)

    assert "host_refresh_interrupt_sources(ctx);" in mmio_write[rebudget:]
    assert "goto rebudget;" in mmio_write[observe:rebudget]


def test_dsp_adapter_retries_the_donor_external_interrupt_pending_bit():
    adapter = (ROOT / "runtime" / "host" / "src" / "dsp_adapter.cpp").read_text()
    assert "deliver_pending_external_interrupt" in adapter
    assert "control_reg & DSP::CR_EXTERNAL_INT" in adapter
    assert "m_impl->deliver_pending_external_interrupt();" in adapter
    assert "core.CheckExternalInterrupt();" in adapter
    assert "core.CheckExceptions();" in adapter
    assert "SetExternalInterrupt(true)" not in adapter


def test_dsp_dma_boundary_retains_bounded_payload_content_metrics():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "static void host_dsp_dma_write" in host
    assert "nonzero_bytes" in host
    assert "payload_hash" in host
    assert '"first=0x%04X nonzero=%u hash=0x%08X\\n"' in host


def test_audio_sequence_active_boundary_retains_track_state_metrics():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "cpu.pc == 0x8029E1B4u" in host
    assert "cpu.lr == 0x80296EB8u" in host
    assert "track + 0x37Eu" in host
    assert "track + 0x320u + i * 4u" in host
    assert '"[audio-sequence-active] return track=0x%08X result=%u "' in host
    assert "cpu.pc == 0x8029E518u" in host
    assert "cpu.lr == 0x80296ED4u" in host
    assert '"[audio-sequence-active] root-init return data=0x%08X "' in host


def test_dynamic_rel_handoff_materializes_and_aliases_all_executable_sections():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert 'module_name[0] != \'\\0\'' in host
    assert "host_register_rel_alias(&cpu, mod, module)" in host
    assert "if (module_id == 1u)" in host
    assert "linked->linked_start" in host
    assert "host_alias_rel_pc(&cpu)" in host
    assert "host_rel_section_linked_start(mod, 1u, 1u)" in host
    assert "cpu.pc = 0x81F800D4u" not in host


def test_rel_file_backed_storage_uses_shared_linked_aliases():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "host_add_shared_guest_alias(image->linked_start, image->size," in host
    assert 'dlsym(lib, "ppc_guest_alias_add_shared")' in host
    assert "ppc_guest_alias_get_storage(linked_start, size, &storage)" in host
    assert "g_module_alias_add_shared(linked_start, size, storage)" in host
    assert "image->bytes == NULL && image->module_id == 336u" in host


def test_dsp_adapter_batches_interpreter_without_enabling_jit():
    adapter = (ROOT / "runtime" / "host" / "src" / "dsp_adapter.cpp").read_text()
    assert "CoreType::Interpreter" in adapter
    assert "constexpr int batch_limit = 8;" in adapter
    assert "void run_exact_eight_cycles()" in adapter
    assert "interpreter.Step();" in adapter
    assert "m_impl->run_exact_eight_cycles();" in adapter
    assert "m_impl->core.RunCycles(batch);" in adapter
    assert "m_impl->deliver_pending_external_interrupt();" in adapter


def test_rel_lifecycle_bridge_accepts_tagged_dol_entry_points():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "return pc & ~0x40000000u;" in host
    assert "host_canonical_linked_pc(cpu->pc)" in host
    assert "lifecycle_pc != 0x80241178u" in host
    assert "lifecycle_pc != 0x802411F8u" in host


def test_title_ready_observer_accepts_tagged_rel_entry_point():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    assert "host_canonical_linked_pc(cpu.pc) == 0x81E01B88u" in host
    assert "host_canonical_linked_pc(cpu.pc) == 0x81E01BA4u" in host
def test_composite_dispatch_preserves_budget_resume_contract():
    header = GENERATED_HEADER.read_text()
    chunk = GENERATED_CHUNK.read_text()
    assert "return ctx->cycle_budget > 0 ? ctx->cycle_budget : 256;" in header
    assert (
        "#define DOLRECOMP_C_LOOP_CYCLE_BUDGET "
        "dolrecomp_loop_cycle_budget(ctx)" in header
    )
    assert "dolrecomp_block_can_precharge" in header
    assert "if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) return;" in chunk
    assert "ctx->pc = address;" in header
    assert "if (dolrecomp_call_original(ctx, address)) return 1;" in header
    assert "if (dolrecomp_call_original(ctx, alias)) return 1;" in header


def test_dolrecomp_mtmsr_returns_at_interrupt_delivery_boundary():
    patch = DOLRECOMP_CYCLE_PATCH.read_text()
    assert "+    case PPC_OP_MTMSR:" in patch
    assert (
        '+        fprintf(out, "    ctx->pc = 0x%08Xu;\\n", '
        "inst->address + 4u);" in patch
    )
    assert '+        fprintf(out, "    return;\\n");' in patch
    assert "run_interrupt_enable_boundary" in patch
    assert "enable_2.pc == 0x80004148u" in patch


def test_pending_interrupt_deadline_starts_only_when_guest_enables_delivery():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    test = (ROOT / "tests" / "interrupt_sources_test.c").read_text()
    assert "if ((cpu->msr & PPC_MSR_EE) != 0u &&" in host
    assert "cpu.msr |= PPC_MSR_EE;" in test
    assert "bluewake_cycle_domain_rebudget(&domain, &cpu);" in test


def test_dynamic_cycle_cap_is_a_deadline_bounded_public_mode():
    host = (ROOT / "runtime" / "host" / "src" / "main.c").read_text()
    domain = (ROOT / "runtime" / "host" / "src" / "cycle_domain.c").read_text()
    assert 'strcmp(cycle_cap_env, "dynamic") == 0' in host
    assert "bluewake_cycle_domain_set_dynamic_cap(&g_cycle_domain, 256, 1024u);" in host
    assert "distance <= domain->dynamic_threshold" in domain
    assert "budget = domain->dynamic_near_cap;" in domain


def test_generated_start_dma_preserves_retail_arq_argument_mapping():
    generated = START_DMA_CHUNK.read_text()
    retail = RETAIL_ARAM_PIECE.read_text()
    assert "command->mTransferDirection, 0, command->mSrc" in retail
    assert "command->mDst, command->mDataLength" in retail
    assert re.search(r"lwz\s+r5, 64\(r31\).*?ctx->gpr\[5\]", generated, re.S)
    assert re.search(r"lwz\s+r7, 72\(r31\).*?ctx->gpr\[7\]", generated, re.S)
    assert re.search(r"lwz\s+r8, 76\(r31\).*?ctx->gpr\[8\]", generated, re.S)
    assert re.search(r"lwz\s+r9, 68\(r31\).*?ctx->gpr\[9\]", generated, re.S)


def test_generated_prepare_command_preserves_field_offsets():
    generated = START_DMA_CHUNK.read_text()
    retail = RETAIL_ARAM_PIECE.read_text()
    assert "command->mTransferDirection = direction" in retail
    assert "command->mSrc = src" in retail
    assert "command->mDst = dst" in retail
    assert "command->mDataLength = length" in retail
    assert "command->mAramBlock = block" in retail
    assert "command->mCallback = callback" in retail
    for register, offset in (("r26", 64), ("r27", 72), ("r28", 76),
                             ("r29", 68), ("r30", 80), ("r31", 88)):
        assert f"stw     {register}, {offset}(r4)" in generated


def test_generated_order_sync_materializes_prepare_command_arguments():
    generated = START_DMA_CHUNK.read_text()
    retail = RETAIL_ARAM_PIECE.read_text()
    assert "JKRAramPiece::orderAsync(direction, source, destination, length, block, NULL)" in retail
    for destination, source in ((26, 3), (27, 4), (28, 5), (29, 6),
                                (30, 7), (31, 8)):
        assert f"or   r{destination}, r{source}, r{source}" in generated
    assert "// 802B5F20: bl      0x802B5CB4" in generated


if __name__ == "__main__":
    for name, test in sorted(globals().items()):
        if name.startswith("test_"):
            test()
    print("CPU ABI contract tests passed.")
