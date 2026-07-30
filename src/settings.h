#pragma once

#include <cstddef>
#include <cstdint>

enum class DetectionType : std::uint8_t
{
	Aimbot,
	Aimlock,
	AntiAim,
	Autostrafe,
	Bhop,
	DllInjection,
	Desubticking,
	Doubletap,
	Hyperscroll,
	InhumanAccuracy,
	InvalidCvar,
	InvalidInput,
	IrregularBehavior,
	NameChanger,
	Nulls,
	SilentAim,
	SubtickSpam,
	Count,
};

enum class DetectionMode : std::uint8_t
{
	Disabled = 0,
	Report = 1,
	Punish = 2,
};

namespace settings
{
	bool Initialize();
	void Shutdown();
	bool IsPluginEnabled();
	bool IsDetectionEnabled(DetectionType detection);
	DetectionMode GetDetectionMode(DetectionType detection);
	bool IsPlayerWhitelisted(std::uint64_t steamId);
	std::size_t GetWhitelistCount();
	std::size_t GetRejectedWhitelistCount();
	std::size_t GetDuplicateWhitelistCount();
	std::size_t GetEnabledDetectionCount();
	std::uint64_t GetDetectionMask();
	std::uint64_t GetRevision();
	bool ShowChatAnnouncements();
	bool ShowCenterAnnouncements();
	const char *GetPunishmentCommand();
	const char *GetKickCommand();
	const char *GetWebhookUrl();
	const char *GetWebhookRoleId();
	const char *GetWebhookServerAddress();
	const char *GetWebhookLogoUrl();
	const char *GetReportUrl();
	const char *GetReportSecret();
	const char *GetLanguage();
	void MarkConfigReloaded();
	void ExecuteConfig();
} // namespace settings
