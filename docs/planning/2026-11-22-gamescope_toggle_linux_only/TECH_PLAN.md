# Technical plan — Show "Present without gamescope's frame layer" on Linux only

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Follow-up to [#586 review](https://github.com/chrissotraidis/bluewake/pull/586#pullrequestreview-6011550248)

## Change

- `settings_table.h`: `BLUEWAKE_NO_GAMESCOPE_WSI` gets `.platforms = Os::Linux`. `platforms` is where a setting exists (the menu draws it, the file keeps it). The review suggested `.on = Os::Linux`, but `.on` is where a switch is on by default, so it would have turned the switch on for every Linux player.
- `tests/settings_table_test.cpp`: a new `kLinuxOnly` list. Its settings must exist on Linux only; every other setting keeps the existing checks unchanged.

## Verification

- `settings_table_test` and `settings_file_test` built and passed locally.
