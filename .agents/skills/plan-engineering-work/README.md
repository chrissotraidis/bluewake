# Engineering planning skill

This provider-neutral [Agent Skills](https://agentskills.io/) package creates or
updates a synchronized `PRD.md`, `TECH_PLAN.md`, and `PROGRESS.md` without
implementing the planned feature. `SKILL.md` is the authoritative workflow;
provider metadata under `agents/` is optional and does not change it.

## Use it

The repository copy is available automatically when the agent discovers
`.agents/skills/`. To install it outside this checkout, copy the complete skill
folder to the skills directory configured by your agent:

```bash
mkdir -p /path/to/your-agent/skills
cp -R .agents/skills/plan-engineering-work /path/to/your-agent/skills/
```

Use the location documented by the agent client; the package itself does not
depend on a specific provider or installation path.

### Use as a slash command

Start a new agent session after installation. Agents that expose installed
skills directly as slash commands can invoke it in one step:

```text
/plan-engineering-work Plan <change or investigation>.
```

If the agent uses a skill selector instead, enter `/skills` and select
**Engineering Plan**, then send the planning request.

### Other invocation methods

Named invocation is also supported:

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
then write new planning artifacts under
`docs/planning/YYYY-MM-DD-feature_name/`, using the user's local creation date
and a lowercase `snake_case` feature name. It honors an explicit destination
and preserves an existing plan's directory unless a move is requested.

Example:

```text
Use $plan-engineering-work to plan reducing Steam Deck frame-time spikes.
Start from issue #215, preserve current gameplay behavior, and separate
source-only validation from hardware measurements.
```

The result is three synchronized artifacts, for example:

```text
docs/planning/2026-10-19-frame_pacing/
├── PRD.md       # measurable frame-pacing outcome and acceptance
├── TECH_PLAN.md # source surfaces, proposed contract, tests and milestones
└── PROGRESS.md  # M0-T01 tasks, dependencies and evidence ledger
```

[PR #397](https://github.com/chrissotraidis/bluewake/pull/397) is a concrete
upstream example of the planning trio and its implementation milestones.

## Validate the result

Run the focused validator regression suite after changing the skill:

```bash
python3 .agents/skills/plan-engineering-work/scripts/test_validate_plan.py
```

Run the included structural validator against the generated planning directory:

```bash
python3 .agents/skills/plan-engineering-work/scripts/validate_plan.py \
  docs/planning/YYYY-MM-DD-feature_name
```

The validator checks artifact structure, milestone/task traceability,
milestone dependencies, and local links. Remote references and technical claims
still require manual review, as reported by the command.
