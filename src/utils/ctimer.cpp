#include "ctimer.h"

std::vector<std::shared_ptr<CTimerBase>> g_NonPersistentTimers;
std::vector<std::shared_ptr<CTimerBase>> g_PersistentTimers;

static void ClearTimerList(std::vector<std::shared_ptr<CTimerBase>> &timers)
{
	for (const auto &timer : timers) timer->active = false;
	timers.clear();
}

static void ProcessTimerList(std::vector<std::shared_ptr<CTimerBase>> &timers)
{
	if (!g_pCS2ACUtils || !g_pCS2ACUtils->GetGlobals()) return;
	// A callback may cancel timers or append another one. Keep the current
	// invocation alive and visit only the timers present at the start.
	const auto snapshot = timers;
	for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it)
	{
		const auto &timer = *it;
		if (!timer->active) continue;
		const auto *globals = g_pCS2ACUtils->GetGlobals();
		const f64 now = timer->useRealTime ? globals->realtime : globals->curtime;
		if (timer->lastExecute == -1) timer->lastExecute = now;
		if (timer->lastExecute + timer->interval > now) continue;
		const bool repeat = timer->Execute();
		if (repeat && timer->active) timer->lastExecute = now;
		else
		{
			timer->active = false;
			timers.erase(std::remove(timers.begin(), timers.end(), timer), timers.end());
		}
	}
}

void ProcessTimers()
{
	ProcessTimerList(g_PersistentTimers);
	ProcessTimerList(g_NonPersistentTimers);
}

void RemoveNonPersistentTimers() { ClearTimerList(g_NonPersistentTimers); }
void RemoveAllTimers()
{
	ClearTimerList(g_NonPersistentTimers);
	ClearTimerList(g_PersistentTimers);
}
