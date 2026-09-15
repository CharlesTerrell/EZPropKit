# AI Contribution Policy

## Overview

This project allows the use of Artificial Intelligence (AI) and Large Language Model (LLM) tooling to assist contributors with code generation, refactoring, test authoring, and documentation.

However, **AI is an assistant, not an author.** A human contributor must always remain "in the loop" and is held fully accountable for all code, documentation, comments, and interactions submitted to the project.

---

## Core Principles

1. **Human Accountability**: You are the author of your submission. You are personally responsible for the correctness, security, quality, licensing, and maintainability of any contribution you submit, regardless of the tools used to produce it.
2. **Comprehension**: If you cannot understand, explain, or defend a line of code or design decision during code review, do not submit it.
3. **Respect Maintainer Time**: Maintainer attention is a scarce community resource. Submitting unverified, unreviewed, or raw AI-generated output shifts the burden of validation and debugging onto project maintainers and is considered an anti-pattern.

---

## Allowed & Encouraged Uses

AI assistance is not restricted to autocompletion or boilerplate. Contributors may use AI tools to generate substantive logic, write full implementations, or refactor existing code, provided you take full ownership of the result.

Permitted uses include:
- **Code Generation & Implementation**: Prompting models to draft functions, algorithms, modules, or feature logic, provided you critically evaluate, adapt, and understand the code.
- **Interactive Code Completion**: Using editor-integrated assistants for inline suggestions.
- **Refactoring & Modernization**: Restructuring existing components, simplifying complex logic, or improving readability.
- **Tests & Scaffolding**: Generating unit tests, edge-case coverage, mocks, or repetitive boilerplate.
- **Brainstorming & Architecture**: Exploring design patterns, trade-offs, or debugging approaches.
- **Communication & Documentation**: Drafting pull request descriptions, commit summaries, or refining documentation for clarity and grammar.

---

## Contributor Requirements

When contributing AI-assisted work, you must adhere to the following:

- **Local Verification & Testing**: All AI-assisted code must be executed, tested, and validated in your local environment prior to submission. Never submit code you have not run and verified yourself.
- **Licensing & Provenance**: You must ensure that any generated content does not copy unlicensed third-party intellectual property or violate open source licenses.
- **Transparency & Disclosure**: When AI tools contribute a substantial portion of a pull request or commit, note it in your PR description or commit message (e.g., using a commit trailer such as `Assisted-by: <Tool/Model>`). Transparency helps reviewers understand context.
- **No Unsupervised Bots**: Submitting automated, unattended, or mass-generated pull requests and issues without direct human oversight and engagement is prohibited.

---

## Maintainer Discretion

Project maintainers reserve the right to close without review any pull request, issue, or comment that appears to be unverified AI output, or where the contributor is unable or unwilling to meaningfully engage in technical discussion and iterate on feedback.

---

## Portability & Adoption

This policy is designed to be lightweight, plain-language, and easily portable across open source projects. To adopt it:
1. Copy this `AI_POLICY.md` file into your repository's root or `.github/` folder.
2. Reference it from your `CONTRIBUTING.md` or `README.md`.
3. Customize any project-specific preferences (such as specific disclosure tags or strictness levels) as appropriate for your community.

---

## Sources & Inspiration

This policy draws upon principles, community discussions, and established guidance across open source projects and foundations:

- **[Apache Software Foundation (ASF) Generative Tooling Guidance](https://www.apache.org/legal/generative-tooling.html)** — Emphasizes contributor accountability under contributor license agreements, originality representations, and third-party copyright/licensing verification.
- **[Linux Foundation Generative AI Policy](https://www.linuxfoundation.org/legal/generative-ai)** — Establishes that AI contributions should be evaluated on technical merit through peer review, while ensuring tool terms and outputs comply with open source licenses and attribution requirements.
- **[LLVM AI Tool Use Policy](https://llvm.org/docs/AIToolPolicy.html)** — Highlights the "human in the loop" requirement, protecting maintainer time from extractive unverified submissions ("AI slop"), and establishing conventions for disclosure (such as `Assisted-by:` tags).

