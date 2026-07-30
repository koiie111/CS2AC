#include "site_report.h"

#include "movement_analysis/player_context.h"
#include "settings.h"

namespace
{
	constexpr std::size_t maximumQueueSize = 64;

	std::string JsonEscape(std::string_view value)
	{
		std::string result;
		result.reserve(value.size() + 8);
		for (const unsigned char character : value)
		{
			switch (character)
			{
				case '"':
					result += "\\\"";
					break;
				case '\\':
					result += "\\\\";
					break;
				case '\b':
					result += "\\b";
					break;
				case '\f':
					result += "\\f";
					break;
				case '\n':
					result += "\\n";
					break;
				case '\r':
					result += "\\r";
					break;
				case '\t':
					result += "\\t";
					break;
				default:
					if (character < 0x20)
					{
						static constexpr char hex[] = "0123456789abcdef";
						result += "\\u00";
						result += hex[character >> 4];
						result += hex[character & 0x0f];
					}
					else
					{
						result += static_cast<char>(character);
					}
					break;
			}
		}
		return result;
	}
} // namespace

bool SiteReportService::IsValidUrl(const char *url)
{
	if (!url || !*url)
	{
		return true;
	}
	const std::string_view value(url);
	constexpr std::string_view prefix = "https://";
	return value.size() > prefix.size() && value.substr(0, prefix.size()) == prefix && value.find_first_of(" \t\r\n") == std::string_view::npos;
}

bool SiteReportService::IsConfigured() const
{
	const char *url = settings::GetReportUrl();
	const char *secret = settings::GetReportSecret();
	return url && *url && secret && *secret;
}

void SiteReportService::Reload()
{
	CancelRequest();
	queue.clear();
	disabled = false;
	overflowWarned = false;
	httpUnavailableWarned = false;
	configurationWarned = false;
	nextAttempt = {};
	if (IsConfigured() && !IsValidUrl(settings::GetReportUrl()))
	{
		Disable("cs2ac_report_url must be a valid HTTPS URL.");
	}
}

void SiteReportService::Unload()
{
	CancelRequest();
	queue.clear();
	http = nullptr;
	steamContext.Clear();
	disabled = false;
	overflowWarned = false;
	httpUnavailableWarned = false;
	configurationWarned = false;
	nextAttempt = {};
}

void SiteReportService::CancelRequest()
{
	callResult.Cancel();
	if (request != INVALID_HTTPREQUEST_HANDLE && http)
	{
		http->ReleaseHTTPRequest(request);
	}
	request = INVALID_HTTPREQUEST_HANDLE;
}

bool SiteReportService::Report(const char *detection, MovementPlayer *player, std::string_view evidence)
{
	if (!IsConfigured() || disabled)
	{
		if (!configurationWarned)
		{
			Msg("[CS2AC] A website report was not queued. Configure cs2ac_report_url and cs2ac_report_secret, then run cs2ac_reload.\n");
			configurationWarned = true;
		}
		return false;
	}
	if (!detection || !player || !player->GetSteamId64(false))
	{
		return false;
	}
	if (queue.size() >= maximumQueueSize)
	{
		queue.erase(queue.begin() + (request == INVALID_HTTPREQUEST_HANDLE ? 0 : 1));
		if (!overflowWarned)
		{
			Msg("[CS2AC] The website report queue filled up. The oldest report was dropped so gameplay stays unaffected.\n");
			overflowWarned = true;
		}
	}
	queue.push_back({BuildPayload(detection, player, evidence)});
	return true;
}

void SiteReportService::OnGameFrame()
{
	if (request == INVALID_HTTPREQUEST_HANDLE && !queue.empty() && std::chrono::steady_clock::now() >= nextAttempt)
	{
		SendNext();
	}
}

void SiteReportService::SendNext()
{
	if (!http)
	{
		if (!steamContext.Init() || !(http = steamContext.SteamHTTP()))
		{
			if (!httpUnavailableWarned)
			{
				Msg("[CS2AC] Website reports are waiting because Steam's HTTP service is not ready yet.\n");
				httpUnavailableWarned = true;
			}
			nextAttempt = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			return;
		}
		httpUnavailableWarned = false;
	}

	request = http->CreateHTTPRequest(k_EHTTPMethodPOST, settings::GetReportUrl());
	if (request == INVALID_HTTPREQUEST_HANDLE || !http->SetHTTPRequestHeaderValue(request, "X-CS2AC-Secret", settings::GetReportSecret())
		|| !http->SetHTTPRequestRawPostBody(request, "application/json", reinterpret_cast<uint8 *>(queue.front().payload.data()),
											static_cast<uint32>(queue.front().payload.size()))
		|| !http->SetHTTPRequestNetworkActivityTimeout(request, 5) || !http->SetHTTPRequestAbsoluteTimeoutMS(request, 10000)
		|| !http->SetHTTPRequestRequiresVerifiedCertificate(request, true))
	{
		if (request != INVALID_HTTPREQUEST_HANDLE)
		{
			http->ReleaseHTTPRequest(request);
			request = INVALID_HTTPREQUEST_HANDLE;
		}
		RetryOrDrop(0);
		return;
	}

	SteamAPICall_t call {};
	if (!http->SendHTTPRequest(request, &call))
	{
		http->ReleaseHTTPRequest(request);
		request = INVALID_HTTPREQUEST_HANDLE;
		RetryOrDrop(0);
		return;
	}
	callResult.SetGameserverFlag();
	callResult.Set(call, this, &SiteReportService::OnCompleted);
}

void SiteReportService::OnCompleted(HTTPRequestCompleted_t *result, bool failed)
{
	if (!result || result->m_hRequest != request)
	{
		return;
	}
	const int status = static_cast<int>(result->m_eStatusCode);
	const auto now = std::chrono::steady_clock::now();
	if (!failed && result->m_bRequestSuccessful && ((status >= 200 && status <= 299) || status == 409))
	{
		queue.pop_front();
		overflowWarned = false;
		nextAttempt = {};
	}
	else if (status == 401 || status == 403 || status == 404)
	{
		Disable("The website rejected the endpoint or secret.");
	}
	else if (status == 429)
	{
		nextAttempt = now + std::chrono::seconds(5);
	}
	else if (!queue.empty() && queue.front().retries++ == 0)
	{
		nextAttempt = now + std::chrono::seconds(2);
	}
	else
	{
		Msg("[CS2AC] A website report failed after one retry and was dropped (HTTP %d).\n", status);
		queue.pop_front();
		nextAttempt = {};
	}
	http->ReleaseHTTPRequest(request);
	request = INVALID_HTTPREQUEST_HANDLE;
}

void SiteReportService::RetryOrDrop(int status)
{
	if (!queue.empty() && queue.front().retries++ == 0)
	{
		nextAttempt = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		return;
	}
	Msg("[CS2AC] A website report failed after one retry and was dropped%s.\n", status ? " because the endpoint returned an error" : "");
	if (!queue.empty())
	{
		queue.pop_front();
	}
	nextAttempt = {};
}

void SiteReportService::Disable(const char *reason)
{
	disabled = true;
	queue.clear();
	Msg("[CS2AC] Website reports are disabled until the next cs2ac_reload. %s\n", reason);
}

std::string SiteReportService::BuildPayload(const char *detection, MovementPlayer *player, std::string_view evidence)
{
	const std::string playerName = player->GetName() ? player->GetName() : "Unknown player";
	return "{\"source\":\"cs2ac\",\"target_name\":\"" + JsonEscape(playerName) + "\",\"target_steam_id\":\""
		   + std::to_string(player->GetSteamId64(false)) + "\",\"detection\":\"" + JsonEscape(detection) + "\",\"evidence\":\"" + JsonEscape(evidence)
		   + "\"}";
}
