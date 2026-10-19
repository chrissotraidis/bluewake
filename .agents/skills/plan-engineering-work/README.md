# Engineering planning skill

This provider-neutral [Agent Skills](https://agentskills.io/) package creates or
updates a synchronized `PRD.md`, `TECH_PLAN.md`, and `PROGRESS.md` without
implementing the planned feature. `SKILL.md` is the authoritative workflow;
provider metadata under `agents/` is optional and does not change it.

## Use it

Agents that discover repository skills under `.agents/skills/` can invoke it by
name:

```text
Use $plan-engineering-work to plan <change or investigation>.
```

With any other coding agent, point it directly to the portable entry point:

```text
Read and follow .agents/skills/plan-engineering-work/SKILL.md to plan
<change or investigation>. Do not implement the planned product change.
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
