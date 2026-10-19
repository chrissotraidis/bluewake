# Engineering planning skill

This repository-local Codex skill creates or updates a synchronized `PRD.md`,
`TECH_PLAN.md`, and `PROGRESS.md` without implementing the planned feature.
Codex discovers it automatically from `.agents/skills/`.

## Use it

Start Codex in the BlueWake checkout and ask it to use the skill explicitly:

```text
Use $plan-engineering-work to plan <change or investigation>.
```

Include the motivating problem, affected users, desired outcome, constraints,
and any relevant issue or existing planning directory. The skill will inspect
the current repository and upstream work, ask only for material ambiguities,
then write the planning trio under the existing project convention or
`docs/planning/YYYY-MM-DD-<slug>/`.

Example:

```text
Use $plan-engineering-work to plan reducing Steam Deck frame-time spikes.
Start from issue #215, preserve current gameplay behavior, and separate
source-only validation from hardware measurements.
```

## Validate the result

Run the included structural validator against the generated planning directory:

```bash
python3 .agents/skills/plan-engineering-work/scripts/validate_plan.py \
  docs/planning/YYYY-MM-DD-<slug>
```

The validator checks artifact structure, requirement/task/impact traceability,
milestone dependencies, and local links. Remote references and technical claims
still require manual review, as reported by the command.
