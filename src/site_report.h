#pragma once

#include "common.h"

#undef snprintf
#include <steam/steam_gameserver.h>

class MovementPlayer;

class SiteReportService
{
public:
	void Reload();
	void Unload();
	void OnGameFrame();
	bool Report(const char *detection, MovementPlayer *player, std::string_view evidence);

	bool IsConfigured() const;

	bool IsDisabled() const
	{
		return disabled;
	}

	std::size_t QueueSize() const
	{
		return queue.size();
	}

	static bool IsValidUrl(const char *url);

private:
	struct ReportData
	{
		std::string payload;
		unsigned retries {};
	};

	void SendNext();
	void OnCompleted(HTTPRequestCompleted_t *result, bool failed);
	void CancelRequest();
	void Disable(const char *reason);
	void RetryOrDrop(int status);
	static std::string BuildPayload(const char *detection, MovementPlayer *player, std::string_view evidence);

	CSteamGameServerAPIContext steamContext;
	ISteamHTTP *http {};
	CCallResult<SiteReportService, HTTPRequestCompleted_t> callResult;
	std::deque<ReportData> queue;
	HTTPRequestHandle request {INVALID_HTTPREQUEST_HANDLE};
	std::chrono::steady_clock::time_point nextAttempt;
	bool disabled {};
	bool overflowWarned {};
	bool httpUnavailableWarned {};
	bool configurationWarned {};
};
