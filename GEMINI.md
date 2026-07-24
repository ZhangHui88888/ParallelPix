# Gemini Project Rules

This is a Software project initialized from the shared software template. Treat `AGENTS.md` as the source of truth for local agent behavior.

## Read First

1. `AGENTS.md`
2. `.ai-context.md`
3. `.ai-worklog.md`
4. `.ai-learnings.md`
5. `E:\projects\_knowledge\10_domains\software\software-rules.md`
6. `docs/README.md` when task context requires project docs.

## Key Rules

- Use Chinese for user-facing replies unless the user asks otherwise.
- Do not work directly on `main` or `master`; use `feature/描述-MMDD`.
- Do not run `git add`, `git commit`, `git push`, or merge unless the user explicitly asks and confirms.
- Keep project facts in `docs/`; keep reusable cross-project knowledge in `E:\projects\_knowledge`.
- Update `docs/README.md` whenever project docs are added, moved, or deleted.
- Maintain database structure in `docs/sql/schema.sql` and initialization data in `docs/sql/init.sql`; update related design and test documents together.
- Do not modify production env files or commit secrets.

## Memory Triggers

- On “保存” / “checkpoint” / “暂停” / “存一下”: update `.ai-context.md` and append `.ai-worklog.md`.
- On “结束” / “总结” / “收工”: append reusable learnings to `.ai-learnings.md` and compress `.ai-context.md`.
- On “提交” / “commit” / “git提交”: inspect status and diff, show the proposed file list and commit message, then wait for confirmation.
