# Player connection crash: stale datamap ABI

The supplied Linux CS2 1.41.8.4 crash stops at `cs2ac.so + 0x1f8175`.
Disassembly of the supplied binary identifies the datamap loop in
`InitSchemaKeyValueMap`: it multiplies the field index by `0x70`, loads
`fieldName` at offset `0x08`, then crashes reading its first byte. At the
fault, the index is 3, the field count is 5, and the name pointer is
`0xaaf515d56693a6e8`. A null check cannot make that pointer safe.

The pinned HL2SDK revision `17b5ecc9` described the old prediction record.
Current CS2 spawn-key records are `0x38` bytes. The old loop skips entries
and walks beyond the actual array. Schema caches are initialized lazily,
which explains why startup succeeds and player connection triggers the crash.

Update the SDK to `6315f0104d22eb9ea3c33d0505dbe14e8b193bc3`, which includes
AlliedModders' datamap ABI correction in
https://github.com/alliedmodders/hl2sdk/commit/aeaa10b6dbbe9898d47db03c9dcf336528623ce9.
This corrects both cache initialization and `HasField`, while retaining
datamap-only fields used by movement analysis. Do not remove the datamap
fallback or substitute zero offsets for these fields.

Compile-time checks now reject an SDK with the old field stride or
incompatible member offsets. Datamap loops also require a non-null field
array and retain the SDK's full-width field count.

Validation:

- Optimized Linux x64 build in the pinned SteamRT3 SDK container succeeds.
- Compiling the corrected schema source against the previous packaged SDK
  fails at the new stride assertion, as expected.
- Changed C++ source passes clang-format; translation validation passes.

This build targets the updated CS2 ABI. An older server with the previous
datamap layout requires a matching older plugin/SDK build.
