# CLAUDE.md

Fork of [nikivanov/mural](https://github.com/nikivanov/mural) (wall-plotter firmware + web UI), being ported to run on the **MKS DLC32** controller board (https://github.com/makerbase-mks/MKS-DLC32). Remotes: `origin` = this fork, `upstream` = nikivanov/mural.

## Agent skills

### Issue tracker

Issues live as local markdown files under `.scratch/<feature>/` in this repo. See `docs/agents/issue-tracker.md`.

### Triage labels

Default five-role vocabulary (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`), recorded as a `Status:` line in each issue file. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.
