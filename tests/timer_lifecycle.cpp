#include <engine2/gameshareddefs.h>
#include "../src/utils/ctimer.cpp"
#include <cstdlib>

static CGlobalVars clockState;
static CS2ACUtils testUtils;
CS2ACUtils *g_pCS2ACUtils = &testUtils;
CGlobalVars *CS2ACUtils::GetGlobals() { return &clockState; }
static int calls = 0, checks = 0;
static void require(bool value) { ++checks; if (!value) std::abort(); }
static f64 once() { ++calls; return 0; }
static f64 again() { ++calls; return 1; }
static f64 spawn() { StartTimer(once, 0.0, false, false); return 0; }
static void reset() { RemoveAllTimers(); clockState.curtime = clockState.realtime = 0; calls = 0; }
static void advance(float time) { clockState.curtime = clockState.realtime = time; ProcessTimers(); }
struct ClearSelf final : CTimerBase
{
	int &destroyed;
	explicit ClearSelf(int &count) : CTimerBase(0, false), destroyed(count) {}
	~ClearSelf() override { ++destroyed; }
	bool Execute() override { RemoveAllTimers(); require(destroyed == 0); return true; }
};
int main()
{
	reset(); StartTimer(once, 1.0, false, false); advance(0); advance(.5f); require(calls == 0);
	advance(1); require(calls == 1 && g_NonPersistentTimers.empty()); advance(2); require(calls == 1);
	reset(); StartTimer(again, 0.0, true, false); advance(0); require(calls == 1); advance(1); require(calls == 2);
	RemoveAllTimers(); advance(2); require(calls == 2);
	reset(); StartTimer(spawn, 0.0, false, false); advance(0); require(calls == 0 && g_NonPersistentTimers.size() == 1);
	advance(1); require(calls == 1 && g_NonPersistentTimers.empty());
	reset(); StartTimer(once, 0.0, false, false); StartTimer(once, 0.0, true, false);
	RemoveNonPersistentTimers(); advance(0); require(calls == 1 && g_PersistentTimers.empty());
	reset(); StartTimer(once, 1.0, false, true); advance(0); clockState.curtime = 50; ProcessTimers(); require(calls == 0);
	clockState.realtime = 1; ProcessTimers(); require(calls == 1);
	reset(); int destroyed = 0; g_NonPersistentTimers.push_back(std::make_shared<ClearSelf>(destroyed));
	advance(0); require(destroyed == 1 && g_NonPersistentTimers.empty());
	std::printf("Timer lifecycle: %d checks passed\n", checks);
}
