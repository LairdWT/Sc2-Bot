# Sc2 API Integration Notes

## Purpose

This folder documents how the owned Terran bot code uses the checked-in SC2 API and adjacent runtime abstractions.

The authority order for these notes is:

1. local source under `L:\Sc2_Bot\src\sc2api`
2. owned Terran integration code under `L:\Sc2_Bot\examples`
3. local tutorials, tests, and notes under `L:\Sc2_Bot`

External references may inform future design notes, but they do not override the behavior implemented in this repository.

## Current Pages

- [TerranAgentApiIntegration.md](TerranAgentApiIntegration.md)

## Intended Scope

Pages in this folder should stay narrow and source-backed. Good topics include:

- callback and lifecycle flow through `sc2::Agent`
- observation and query acquisition
- unit, order, tag, and ability abstractions used by the Terran bot
- feature-layer data access and transformation
- integration boundaries between the checked-in API and bot-owned orchestration
