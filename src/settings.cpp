#include "settings.h"

#include "convar.h"
#include "eiface.h"
#include "utils/interfaces.h"

#include <algorithm>
#include <charconv>
#include <new>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::size_t rejectedWhitelistEntries {};
	std::size_t duplicateWhitelistEntries {};
	std::uint64_t settingsRevision {1};
	std::uint64_t detectionMask {};
	bool detectionMaskDirty {true};
	bool pluginEnabled {};

	std::vector<std::uint64_t> &WhitelistedSteamIds()
	{
		static std::vector<std::uint64_t> steamIds;
		return steamIds;
	}

	void BumpRevision()
	{
		if (++settingsRevision == 0)
		{
			settingsRevision = 1;
		}
	}

	void OnPluginSettingChanged(CConVar<bool> *, CSplitScreenSlot, const bool *, const bool *)
	{
		detectionMaskDirty = true;
		BumpRevision();
	}

	void OnDetectionSettingChanged(CConVar<int32> *, CSplitScreenSlot, const int32 *, const int32 *)
	{
		detectionMaskDirty = true;
		BumpRevision();
	}

	void OnWhitelistChanged(CConVar<CUtlString> *, CSplitScreenSlot, const CUtlString *newValue, const CUtlString *)
	{
		auto &steamIds = WhitelistedSteamIds();
		steamIds.clear();
		rejectedWhitelistEntries = 0;
		duplicateWhitelistEntries = 0;
		std::string value = newValue ? newValue->Get() : "";
		std::replace(value.begin(), value.end(), ',', ' ');
		std::replace(value.begin(), value.end(), ';', ' ');
		std::istringstream entries(value);
		for (std::string entry; entries >> entry;)
		{
			std::uint64_t steamId = 0;
			const auto parsed = std::from_chars(entry.data(), entry.data() + entry.size(), steamId);
			if (parsed.ec == std::errc() && parsed.ptr == entry.data() + entry.size() && steamId != 0)
			{
				steamIds.push_back(steamId);
			}
			else
			{
				++rejectedWhitelistEntries;
			}
		}
		std::sort(steamIds.begin(), steamIds.end());
		const std::size_t parsedEntries = steamIds.size();
		steamIds.erase(std::unique(steamIds.begin(), steamIds.end()), steamIds.end());
		duplicateWhitelistEntries = parsedEntries - steamIds.size();
		BumpRevision();
	}

	struct Configuration
	{
		CConVar<bool> enabled {"cs2ac_enabled", FCVAR_NONE, "Enable or disable CS2AC", true, OnPluginSettingChanged};
		CConVar<int32> aimbotEnabled {"cs2ac_aimbot_enabled",   FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
									  OnDetectionSettingChanged};
		CConVar<int32> aimlockEnabled {"cs2ac_aimlock_enabled",  FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
									   OnDetectionSettingChanged};
		CConVar<int32> antiaimEnabled {"cs2ac_antiaim_enabled",  FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
									   OnDetectionSettingChanged};
		CConVar<int32> autostrafeEnabled {"cs2ac_autostrafe_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
										  OnDetectionSettingChanged};
		CConVar<int32> bhopEnabled {"cs2ac_bhop_enabled",     FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
									OnDetectionSettingChanged};
		CConVar<int32> dllInjectionEnabled {"cs2ac_dll_injection_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
											OnDetectionSettingChanged};
		CConVar<int32> desubtickingEnabled {"cs2ac_desubticking_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 1, true, 0, true, 2,
											OnDetectionSettingChanged};
		CConVar<int32> doubletapEnabled {"cs2ac_doubletap_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 1, true, 0, true, 2,
										 OnDetectionSettingChanged};
		CConVar<int32> hyperscrollEnabled {"cs2ac_hyperscroll_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
										   OnDetectionSettingChanged};
		CConVar<int32> inhumanAccuracyEnabled {"cs2ac_inhuman_accuracy_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
											   OnDetectionSettingChanged};
		CConVar<int32> invalidCvarEnabled {"cs2ac_invalid_cvar_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 1, true, 0, true, 2,
										   OnDetectionSettingChanged};
		CConVar<int32> invalidInputEnabled {"cs2ac_invalid_input_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
											OnDetectionSettingChanged};
		CConVar<int32> irregularBehaviorEnabled {
			"cs2ac_irregular_behavior_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2, OnDetectionSettingChanged};
		CConVar<int32> namechangerEnabled {"cs2ac_namechanger_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 1, true, 0, true, 2,
										   OnDetectionSettingChanged};
		CConVar<int32> nullsEnabled {"cs2ac_nulls_enabled",    FCVAR_NONE, "0=off, 1=website report, 2=punish", 1, true, 0, true, 2,
									 OnDetectionSettingChanged};
		CConVar<int32> silentaimEnabled {"cs2ac_silentaim_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 0, true, 0, true, 2,
										 OnDetectionSettingChanged};
		CConVar<int32> subtickSpamEnabled {"cs2ac_subtick_spam_enabled", FCVAR_NONE, "0=off, 1=website report, 2=punish", 1, true, 0, true, 2,
										   OnDetectionSettingChanged};
		CConVar<bool> chatAnnouncements {"cs2ac_chat_announcements", FCVAR_NONE, "Show CS2AC detections in public chat", true};
		CConVar<bool> centerAnnouncements {"cs2ac_center_announcements", FCVAR_NONE, "Show CS2AC detections in the center of the screen", true};
		CConVar<CUtlString> punishmentCommand {"cs2ac_punishment_command", FCVAR_NONE, "Command run for permanent-ban detections",
											   CUtlString("css_addban {steamid64} 0 CS2AC: {detection}")};
		CConVar<CUtlString> kickCommand {"cs2ac_kick_command", FCVAR_NONE, "Command run for kick-only detections",
										 CUtlString("css_kick #{userid} CS2AC: {detection}")};
		CConVar<CUtlString> webhookUrl {"cs2ac_webhook_url", FCVAR_PROTECTED, "Discord webhook URL for detection reports", CUtlString("")};
		CConVar<CUtlString> webhookRoleId {"cs2ac_webhook_role_id", FCVAR_NONE, "Discord role ID mentioned in detection reports", CUtlString("")};
		CConVar<CUtlString> webhookServerAddress {"cs2ac_webhook_server_address", FCVAR_NONE, "Public server address shown in Discord reports",
												  CUtlString("")};
		CConVar<CUtlString> webhookLogoUrl {"cs2ac_webhook_logo_url", FCVAR_NONE, "Public HTTPS URL for the logo shown in Discord reports",
											CUtlString("")};
		CConVar<CUtlString> reportUrl {"cs2ac_report_url", FCVAR_NONE, "HTTPS endpoint for website reports", CUtlString("")};
		CConVar<CUtlString> reportSecret {"cs2ac_report_secret", FCVAR_PROTECTED, "Secret used to authenticate website reports", CUtlString("")};
		CConVar<CUtlString> language {"cs2ac_language", FCVAR_NONE, "Language used for public messages and Discord reports", CUtlString("en")};
		CConVar<CUtlString> whitelist {"cs2ac_whitelist", FCVAR_NONE, "SteamID64s that CS2AC may detect but never punish", CUtlString(""),
									   OnWhitelistChanged};
	};

	Configuration *configuration {};

	int32 DetectionSetting(DetectionType detection)
	{
		if (!configuration)
		{
			return false;
		}
		switch (detection)
		{
			case DetectionType::Aimbot:
				return configuration->aimbotEnabled.Get();
			case DetectionType::Aimlock:
				return configuration->aimlockEnabled.Get();
			case DetectionType::AntiAim:
				return configuration->antiaimEnabled.Get();
			case DetectionType::Autostrafe:
				return configuration->autostrafeEnabled.Get();
			case DetectionType::Bhop:
				return configuration->bhopEnabled.Get();
			case DetectionType::DllInjection:
				return configuration->dllInjectionEnabled.Get();
			case DetectionType::Desubticking:
				return configuration->desubtickingEnabled.Get();
			case DetectionType::Doubletap:
				return configuration->doubletapEnabled.Get();
			case DetectionType::Hyperscroll:
				return configuration->hyperscrollEnabled.Get();
			case DetectionType::InhumanAccuracy:
				return configuration->inhumanAccuracyEnabled.Get();
			case DetectionType::InvalidCvar:
				return configuration->invalidCvarEnabled.Get();
			case DetectionType::InvalidInput:
				return configuration->invalidInputEnabled.Get();
			case DetectionType::IrregularBehavior:
				return configuration->irregularBehaviorEnabled.Get();
			case DetectionType::NameChanger:
				return configuration->namechangerEnabled.Get();
			case DetectionType::Nulls:
				return configuration->nullsEnabled.Get();
			case DetectionType::SilentAim:
				return configuration->silentaimEnabled.Get();
			case DetectionType::SubtickSpam:
				return configuration->subtickSpamEnabled.Get();
			case DetectionType::Count:
				return false;
		}
		return false;
	}
} // namespace

bool settings::Initialize()
{
	if (!configuration)
	{
		configuration = new (std::nothrow) Configuration;
		detectionMaskDirty = true;
	}
	return configuration != nullptr;
}

void settings::Shutdown()
{
	delete configuration;
	configuration = nullptr;
	WhitelistedSteamIds().clear();
	rejectedWhitelistEntries = 0;
	duplicateWhitelistEntries = 0;
	detectionMask = 0;
	detectionMaskDirty = true;
	pluginEnabled = false;
}

bool settings::IsPluginEnabled()
{
	GetDetectionMask();
	return pluginEnabled;
}

bool settings::IsDetectionEnabled(DetectionType detection)
{
	const auto index = static_cast<std::uint8_t>(detection);
	return index < static_cast<std::uint8_t>(DetectionType::Count) && (GetDetectionMask() & (std::uint64_t {1} << index)) != 0;
}

DetectionMode settings::GetDetectionMode(DetectionType detection)
{
	const int32 mode = DetectionSetting(detection);
	if (mode <= static_cast<int32>(DetectionMode::Disabled))
	{
		return DetectionMode::Disabled;
	}
	if (mode >= static_cast<int32>(DetectionMode::Punish))
	{
		return DetectionMode::Punish;
	}
	return DetectionMode::Report;
}

bool settings::IsPlayerWhitelisted(std::uint64_t steamId)
{
	const auto &steamIds = WhitelistedSteamIds();
	return steamId != 0 && std::binary_search(steamIds.begin(), steamIds.end(), steamId);
}

std::size_t settings::GetWhitelistCount()
{
	return WhitelistedSteamIds().size();
}

std::size_t settings::GetRejectedWhitelistCount()
{
	return rejectedWhitelistEntries;
}

std::size_t settings::GetDuplicateWhitelistCount()
{
	return duplicateWhitelistEntries;
}

std::size_t settings::GetEnabledDetectionCount()
{
	const std::uint64_t mask = GetDetectionMask();
	std::size_t count = 0;
	for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DetectionType::Count); ++index)
	{
		count += (mask >> index) & 1;
	}
	return count;
}

std::uint64_t settings::GetDetectionMask()
{
	if (!detectionMaskDirty)
	{
		return detectionMask;
	}

	detectionMask = 0;
	pluginEnabled = configuration && configuration->enabled.GetBool();
	if (pluginEnabled)
	{
		for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DetectionType::Count); ++index)
		{
			if (DetectionSetting(static_cast<DetectionType>(index)))
			{
				detectionMask |= std::uint64_t {1} << index;
			}
		}
	}
	detectionMaskDirty = false;
	return detectionMask;
}

std::uint64_t settings::GetRevision()
{
	return settingsRevision;
}

bool settings::ShowChatAnnouncements()
{
	return configuration && configuration->chatAnnouncements.GetBool();
}

bool settings::ShowCenterAnnouncements()
{
	return configuration && configuration->centerAnnouncements.GetBool();
}

const char *settings::GetPunishmentCommand()
{
	return configuration ? configuration->punishmentCommand.Get().Get() : "";
}

const char *settings::GetKickCommand()
{
	return configuration ? configuration->kickCommand.Get().Get() : "";
}

const char *settings::GetWebhookUrl()
{
	return configuration ? configuration->webhookUrl.Get().Get() : "";
}

const char *settings::GetWebhookRoleId()
{
	return configuration ? configuration->webhookRoleId.Get().Get() : "";
}

const char *settings::GetWebhookServerAddress()
{
	return configuration ? configuration->webhookServerAddress.Get().Get() : "";
}

const char *settings::GetWebhookLogoUrl()
{
	return configuration ? configuration->webhookLogoUrl.Get().Get() : "";
}

const char *settings::GetReportUrl()
{
	return configuration ? configuration->reportUrl.Get().Get() : "";
}

const char *settings::GetReportSecret()
{
	return configuration ? configuration->reportSecret.Get().Get() : "";
}

const char *settings::GetLanguage()
{
	return configuration ? configuration->language.Get().Get() : "en";
}

void settings::MarkConfigReloaded()
{
	BumpRevision();
}

void settings::ExecuteConfig()
{
	if (interfaces::pEngine)
	{
		interfaces::pEngine->ServerCommand("exec cs2ac.cfg\n");
	}
}
