# CS2 1.41.9.7 Linux compatibility

Version 1.1.2-14197 keeps the existing detector policy and cs2ac.cfg. Deploy only
cs2ac.so and its loader; existing gamedata and translations remain valid.

The engine replaced virtual network-message handles with 40-byte registration
records. The SDK pin 81020d1a26b9d4321b9f030c96929ec18e8c0cff implements the new
lookup slots and inline allocation/name/ID helpers. utils/network_abi.h rejects
an old SDK at compile time. See alliedmodders/s2sdk#124.

The port uses the SDK server/client interfaces, schema names, entity-system
accessor, tier0 implementation files and Steam game-server interface accessors.
The old private CServerSideClient layout is replaced by the SDK declaration.
The cvar-response hook validates its gamedata against the compiled SDK before
installing a detour. On the verified Linux engine, its slot remains 40,
client-slot offset 72 and server client-vector offset 616. Detector settings,
punishment templates and website configuration are not changed.

Build pins are recorded in engine-build.json and the submodules. Run bootstrap.sh
and build-linux.sh for the Steam Runtime build, or use Clang/AMBuild with
configure.py --enable-optimize in an out-of-tree build directory. lld is required.

cs2ac_status additionally reports observed fire-bullets messages, posted cvar
queries and validated replies. These are transport diagnostics, not detection
counts. A successful load alone does not validate a real client's cvar replies
or the rendering of announcements.

Linux native integration checks cover late load, bot spawn/movement/shooting,
announcement messages, map transitions, settings reload, native unload/reload
and clean startup alongside
native FakeRanks. The final release passed six complete announcement/map/unload/reload cycles,
then three more after a cold start. Tests must keep reporting endpoints empty and punishment
commands disabled when using isolated fixtures. Windows runtime compatibility
has not been tested by this change.

Configuration and timer lifetime
--------------------------------
The SDK ConVar registration list retains pointers into Configuration until
ConVar_Unregister runs. Cleanup now unregisters while Configuration is alive,
then destroys Configuration. Destroying it first caused use-after-free during
unload and corrupted the allocator before a subsequent plugin load.

The temporary unload guard used during diagnosis is not part of this source.
The integration test includes actual successful unload/reload, announcements,
map transitions and cold startup. No full cheat-detector acceptance is claimed.

The announcement broadcast transfers event ownership to the engine's FireEvent
API. Timers use owning snapshots so a callback may cancel or append timers
without destroying an invocation still on the stack. The actual timer code has
13 offline lifecycle checks in tests/timer_lifecycle.cpp. The Linux export map
exposes only Metamod's CreateInterface entry point.
