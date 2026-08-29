# Agent workflow

This repository uses the following multi-agent workflow unless the user explicitly requests otherwise.

- The primary agent uses GPT-5.6 Sol with high reasoning and focuses on supervision, task decomposition, review, integration decisions, and monitoring.
- Delegate implementation and research to subagents using GPT-5.6 Luna with max reasoning.
- The primary agent may step in directly when a Luna subagent is blocked, repeatedly struggling, or needs help with a difficult design or debugging problem.
- Have the primary agent review material subagent changes and verify the integrated result before reporting completion.
- Prefer one focused Luna subagent at a time to conserve usage. Use parallel subagents only for genuinely independent work where parallelism materially reduces latency.
- Do not delegate trivial communication, status reporting, or work that is faster to complete through direct supervision.
