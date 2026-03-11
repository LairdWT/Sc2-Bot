# Coding Standards

## Naming
- Use PascalCase for variables, functions, types, classes, structs, and data members unless a stricter project file states otherwise.
- Do not use single-letter variable names or abbreviations unless absolutely required.
- Do not name variables with the same names as types.
- Enums must begin with `E`.
- Structs must begin with `F`.
- New helper types, classes, or structs must live in their own appropriately named files.

## Types And Signatures
- Use explicit types. Do not use `auto`, `var`, or `any`.
- Prefer `const` and references for function arguments and local variables when reasonable.
- Pass trivial-to-copy types by value only when smaller than a pointer.
- Mark member functions `const` when available and reasonable.
- Prefer `static`, `const`, or `constexpr` for variables when possible.

## Control Flow And Design
- Prefer enums over booleans for representing state when reasonable.
- Prefer `switch` over long `if` or `if else` chains when it improves clarity for state dispatch.
- Prefer composition over inheritance.
- Prefer interfaces for object-to-object APIs to keep callers decoupled from implementations.
- Use single-responsibility functions with names that describe exactly what they do.
- Use data-oriented design and structure-of-arrays layouts when reasonable.
- Use Producer, Accumulator, Consumer architecture where it fits the system design.

## Comments
- Comments must be informative and objective.
- Do not use subjective or unprofessional language in comments or naming.
- Do not use emojis in code, comments, or documentation.

## Build And Debug
- Prefer compile-time preprocessing for debug-only logging or checks when build configuration differences are required.
- Fix and debug issues directly instead of removing code as a shortcut around warnings or errors.

## Project Workflow
- Review this file before making code changes to maintain compliance.
- Follow explicit user instructions completely, including requested test counts or verification steps.
- Do not avoid repetitive manual edits when they are required for correctness.
