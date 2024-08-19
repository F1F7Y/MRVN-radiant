# MRVN-Radiant

The open-source, cross-platform level editor for Respawn Entertainment Source based games.

MRVN-Radiant is a fork of NetRadiant-custom (GtkRadiant 1.4 &rarr; massive rewrite &rarr; 1.5 &rarr; NetRadiant &rarr; NetRadiant-custom &rarr; this)

<div align=left>
<img alt="GitHub Workflow Status" src="https://img.shields.io/github/actions/workflow/status/MRVNRadiant/MRVNRadiant/build.yml?style=for-the-badge">
<img alt="GitHub Issues" src="https://img.shields.io/github/issues/MRVNRadiant/MRVNRadiant?style=for-the-badge">
<img alt="GitHub Pullrequests" src="https://img.shields.io/github/issues-pr/MRVNRadiant/MRVNRadiant?style=for-the-badge">
</div>

## Supported games
- Titanfall Online ( Titanfall Online Revival )
- Titanfall 2 ( [Northstar](https://northstar.tf) )
- Apex Legends ( [R5Reloaded](https://r5reloaded.com/) )

## Status
| Game | Coverage | Note |
|------|----------|------|
| Titanfall Online | 38% | No triangle collision, no lighting, no portals |
| Titanfall 2 | 37% | No triangle collision, no lighting, no portals |
| Apex Legends | 25% | No collision, no lighting, no portals |

> Coverage = Lumps generated / Total lump count. Stubbed lumps arent counted because they're not custom.

> NOTE: These values are updated manually.

## Other tools
- [bsp_tool](https://github.com/snake-biscuits/bsp_tool)
    - A Python library for analyzing .bsp files

- [MRVN-Explorer](https://github.com/MRVN-Radiant/MRVN-Explorer)
    - A OpenGL/ImGui respawn bsp viewer
