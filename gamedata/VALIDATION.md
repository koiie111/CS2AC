# Linux gamedata validation

Checked on 2026-09-25 for CS2 1.41.8.2 and the installed Linux server,
CS2 1.41.8.3 (`ServerVersion=2000915`, `SourceRevision=11030201`).

## Signatures

All 15 Linux signatures already in `cs2ac.games.txt` match uniquely.
No Linux byte patterns needed changing.

- [GDC report for Linux 1.41.8.2](https://gdc.kitsune-lab.com/result/387bbe98953141e5b6e3df31bac861):
  15 passed, 0 not found, 0 warnings.
- Scanning the executable sections of the installed 1.41.8.3 `libserver.so`
  also found exactly one match for each of the 15 signatures.
- [CS2 signature tracker](https://github.com/ianlucas/cs2-signatures/blob/main/.github/docs/cs2kz-metamod.md)
  provides an independent reference for the movement signatures.

For GDC, convert the plugin's `\x2A` wildcard bytes to `?` in IDA-style
patterns (for example, `55 48 89 E5 ?`). Do not submit `\x2A` as a literal
byte. Select **Linux** and the intended game version.

A unique byte-pattern match checks resolution; it does not alone prove
function identity or runtime behavior.

## Offsets

| Key | Previous Linux value | Verified Linux value | Meaning |
| --- | ---: | ---: | --- |
| GameEntitySystem | 80 | 80 | Byte offset in the game resource service |
| Teleport | 162 | 164 | Virtual function index |
| IsEntityPawn | 168 | 170 | Virtual function index |
| IsEntityController | 169 | 171 | Virtual function index |
| ClientOffset | 584 | 616 | Client vector byte offset (`0x268`) |
| ProcessRespondCvarValue | 40 | 40 | Primary `CServerSideClient` vtable index |
| ClientSlotOffset | 72 | 72 | `m_nClientSlot` byte offset (`0x48`) |

The last two values remain current; increasing them to follow unrelated
entity-vtable changes would be incorrect. They agree with
[VoltMod's engine gamedata](https://github.com/voltygg/voltmod/blob/main/gamedata/gamedata.jsonc).
In the installed Linux `libengine2.so`, the primary `CServerSideClient`
vtable is at ELF virtual address `0x9cf3c0`, and entry 40 points to
`0x540cc0`. Client code reads the slot at `this + 0x48`, and accesses the
server's client-vector count/storage at `server + 0x268`/`server + 0x270`.

The installed binaries used for the offline check have these SHA-256 hashes:

```text
libserver.so  23373cfdb96dee1f2da858274c03346c952faff2942b5e7923525e187366e87f
libengine2.so 0b6079345288c57f51ba4a51ad1a54a435ab080cb68e654c442c33b46059a6c0
```

## Server verification

The original CS2AC 1.1.1 binary was restored, with no C++ changes, and loaded
with the corrected gamedata on Linux 1.41.8.3. Metamod reported the plugin
running. Startup (`de_mirage` to workshop `aim_map`) and explicit changes
`aim_map -> de_mirage -> aim_map` completed in the same server process.

The previous server gamedata contained stray `..` after `"IsEntityPawn"`;
Metamod reported CS2AC stopped because the file could not be read. The
deployed file was replaced with valid gamedata before this test.

There were no human players during the test. Actual client cvar responses
and all detector behaviors were not exercised by this map-change check.
