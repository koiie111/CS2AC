#pragma once

#include <khook.hpp>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include "utlvector.h"
#include "gameconfig.h"

class CDetourBase
{
public:
	virtual ~CDetourBase() = default;
	virtual const char *GetName() = 0;
	virtual bool CreateDetour(CGameConfig *gameConfig) = 0;
	virtual bool EnableDetour() = 0;
	virtual bool DisableDetour() = 0;
	virtual bool FreeDetour() = 0;
	virtual bool IsInstalled() const = 0;
};

extern CUtlVector<CDetourBase *> g_vecDetours;

template<typename Signature>
class CDetour;

// Register movement callbacks with Metamod's shared KHook service. A callback
// invokes the original through KHook's trampoline, then supersedes its call.
template<typename Return, typename... Args>
class CDetour<Return(Args...)> final : public CDetourBase
{
public:
	using Function = Return(Args...);

	CDetour(Function *callback, const char *name) : callback(callback), name(name) {}

	const char *GetName() override
	{
		return name;
	}

	bool IsInstalled() const override
	{
		return installed;
	}

	bool CreateDetour(CGameConfig *gameConfig) override
	{
		if (target)
		{
			return true;
		}
		target = reinterpret_cast<Function *>(gameConfig->ResolveFunctionSignature(name));
		if (!target)
		{
			return false;
		}
		g_vecDetours.AddToTail(this);
		return true;
	}

	bool EnableDetour() override
	{
		if (installed)
		{
			return true;
		}
		if (!target || !KHook::__exported__khook)
		{
			return false;
		}
		hook = std::make_unique<KHook::Function<Return, Args...>>(this, &CDetour::OnHook, nullptr);
		hook->Configure(reinterpret_cast<void *>(target));
		installed = true;
		return true;
	}

	bool DisableDetour() override
	{
		hook.reset();
		installed = false;
		return true;
	}

	bool FreeDetour() override
	{
		DisableDetour();
		target = nullptr;
		return true;
	}

	Function *GetFunc()
	{
		return reinterpret_cast<Function *>(KHook::FindOriginal(reinterpret_cast<void *>(target)));
	}

	template<typename... CallArgs>
	auto operator()(CallArgs &&...args)
	{
		return std::invoke(GetFunc(), std::forward<CallArgs>(args)...);
	}

private:
	KHook::Return<Return> OnHook(Args... args)
	{
		if constexpr (std::is_void_v<Return>)
		{
			callback(args...);
			return {KHook::Action::Supersede};
		}
		else
		{
			return {KHook::Action::Supersede, callback(args...)};
		}
	}

	Function *callback;
	const char *name;
	Function *target {};
	std::unique_ptr<KHook::Function<Return, Args...>> hook;
	bool installed {};
};

#define DECLARE_DETOUR(name, detour) CDetour<decltype(detour)> name(detour, #name)
#define INIT_DETOUR(config, name)    (name.CreateDetour(config) && name.EnableDetour())

bool FlushAllDetours();
