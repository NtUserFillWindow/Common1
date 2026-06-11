#include "WebClient.h"

#include <cctype>

namespace {

	std::string Trim(const std::string& value)
	{
		size_t begin = 0;
		while (begin < value.size() && std::isspace((unsigned char)value[begin])) {
			begin++;
		}

		size_t end = value.size();
		while (end > begin && std::isspace((unsigned char)value[end - 1])) {
			end--;
		}
		return value.substr(begin, end - begin);
	}

	std::string ToLower(std::string value)
	{
		for (auto& ch : value) {
			ch = (char)std::tolower((unsigned char)ch);
		}
		return value;
	}

	bool StartsWith(const std::string& value, const std::string& prefix)
	{
		return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
	}

	bool EndsWith(const std::string& value, const std::string& suffix)
	{
		return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
	}

	std::vector<std::string> Split(const std::string& value, char delimiter)
	{
		std::vector<std::string> fields;
		size_t begin = 0;
		while (begin <= value.size()) {
			size_t end = value.find(delimiter, begin);
			if (end == std::string::npos) {
				fields.push_back(value.substr(begin));
				break;
			}
			fields.push_back(value.substr(begin, end - begin));
			begin = end + 1;
		}
		return fields;
	}

	bool ParseCookiePair(const std::string& cookieStr, std::string* key, std::string* value)
	{
		size_t pos = cookieStr.find('=');
		if (pos == std::string::npos) {
			return false;
		}

		std::string cookieKey = Trim(cookieStr.substr(0, pos));
		if (cookieKey.empty()) {
			return false;
		}

		if (key) {
			*key = cookieKey;
		}
		if (value) {
			*value = Trim(cookieStr.substr(pos + 1));
		}
		return true;
	}

	std::string NormalizeCookieHost(const std::string& host)
	{
		std::string value = Trim(host);
		const std::string httpOnlyPrefix = "#HttpOnly_";
		if (StartsWith(value, httpOnlyPrefix)) {
			value = value.substr(httpOnlyPrefix.size());
		}
		return ToLower(value);
	}

	bool CookieHostMatches(const std::string& requestHost, const std::string& cookieHost)
	{
		std::string host = ToLower(requestHost);
		std::string storedHost = NormalizeCookieHost(cookieHost);
		if (storedHost.empty()) {
			return true;
		}
		if (host == storedHost) {
			return true;
		}
		if (storedHost[0] == '.') {
			std::string rootHost = storedHost.substr(1);
			return host == rootHost || EndsWith(host, storedHost);
		}
		return false;
	}

	std::vector<std::string> SplitCookieList(const std::string& cookieText)
	{
		std::vector<std::string> items;
		size_t begin = 0;
		while (begin < cookieText.size()) {
			size_t end = cookieText.find("; ", begin);
			if (end == std::string::npos) {
				items.push_back(cookieText.substr(begin));
				break;
			}
			items.push_back(cookieText.substr(begin, end - begin));
			begin = end + 2;
		}
		return items;
	}

	void ApplyHeaders(const HttpRequest& request, HttpTransport& http)
	{
		for (auto& item : request.Header) {
			http.AddHeader(item.first, item.second);
		}
	}
}

WebClient::WebClient()
{
}

WebClient::~WebClient()
{
}

std::string WebClient::GetHost(const std::string& strUrl)const
{
	size_t begin = strUrl.find("://");
	begin = begin == std::string::npos ? 0 : begin + 3;

	size_t end = strUrl.find_first_of("/?#", begin);
	std::string host = strUrl.substr(begin, end == std::string::npos ? std::string::npos : end - begin);

	size_t userInfo = host.rfind('@');
	if (userInfo != std::string::npos) {
		host = host.substr(userInfo + 1);
	}

	if (!host.empty() && host[0] == '[') {
		size_t ipv6End = host.find(']');
		if (ipv6End != std::string::npos) {
			return ToLower(host.substr(0, ipv6End + 1));
		}
	}

	size_t port = host.find(':');
	if (port != std::string::npos) {
		host = host.substr(0, port);
	}
	return ToLower(host);
}

void WebClient::ApplyCookies(const std::string& host, HttpTransport& http)const
{
	std::lock_guard<std::mutex> lock(cookiesMutex);
	for (auto& item : cookies) {
		if (!CookieHostMatches(host, item.host)) {
			continue;
		}

		std::string key;
		std::string value;
		if (ParseCookiePair(item.cookieStr, &key, &value)) {
			http.AddCookie(key, value);
		}
	}
}

void WebClient::SaveCookies(const std::string& host, const Text::String& cookieText)
{
	for (auto& item : SplitCookieList(cookieText)) {
		item = Trim(item);
		if (item.empty()) {
			continue;
		}

		auto fields = Split(item, '\t');
		if (fields.size() >= 7) {
			std::string cookieHost = NormalizeCookieHost(fields[0]);
			if (ToLower(fields[1]) == "true" && !cookieHost.empty() && cookieHost[0] != '.') {
				cookieHost = "." + cookieHost;
			}
			SetCookie(cookieHost.empty() ? host : cookieHost, fields[5] + "=" + fields[6]);
			continue;
		}

		SetCookie(host, item);
	}
}

void WebClient::SetCookie(const std::string& host, const std::string& cookieStr)
{
	std::string key;
	std::string value;
	if (!ParseCookiePair(cookieStr, &key, &value)) {
		return;
	}

	std::string cookieHost = host.find("://") == std::string::npos ? NormalizeCookieHost(host) : GetHost(host);
	std::string newCookie = key + "=" + value;

	std::lock_guard<std::mutex> lock(cookiesMutex);
	for (auto& item : cookies) {
		std::string oldKey;
		if (NormalizeCookieHost(item.host) == cookieHost && ParseCookiePair(item.cookieStr, &oldKey, NULL) && oldKey == key) {
			item.cookieStr = newCookie;
			return;
		}
	}

	cookie item;
	item.host = cookieHost;
	item.cookieStr = newCookie;
	cookies.push_back(item);
}

std::string WebClient::GetCookie(const std::string& host)
{
	std::string cookieText;
	std::string requestHost = host.find("://") == std::string::npos ? ToLower(host) : GetHost(host);

	std::lock_guard<std::mutex> lock(cookiesMutex);
	for (auto& item : cookies) {
		if (!CookieHostMatches(requestHost, item.host)) {
			continue;
		}

		if (!cookieText.empty()) {
			cookieText += "; ";
		}
		cookieText += item.cookieStr;
	}
	return cookieText;
}

int WebClient::HttpGet(const HttpRequest& request, std::string* response, int nTimeout)
{
	std::string tempResponse;
	HttpTransport http;
	http.Proxy = Proxy;
	ApplyHeaders(request, http);

	std::string host = GetHost(request.url);
	ApplyCookies(host, http);

	int code = http.HttpGet(request.url, response ? response : &tempResponse, nTimeout);
	SaveCookies(host, http.GetCookie());
	return code;
}

int WebClient::HttpPost(const HttpRequest& request, const std::string& data, std::string* response, int nTimeout)
{
	std::string tempResponse;
	HttpTransport http;
	http.Proxy = Proxy;
	ApplyHeaders(request, http);

	std::string host = GetHost(request.url);
	ApplyCookies(host, http);

	int code = http.HttpPost(request.url, data, response ? response : &tempResponse, nTimeout);
	SaveCookies(host, http.GetCookie());
	return code;
}

int WebClient::DownloadFile(const HttpRequest& request, const std::wstring& filename, const std::function<void(long long dltotal, long long dlnow)>& progressCallback, int nTimeout)
{
	HttpTransport http;
	http.Proxy = Proxy;
	ApplyHeaders(request, http);

	std::string host = GetHost(request.url);
	ApplyCookies(host, http);

	int code = http.DownloadFile(request.url, filename, progressCallback, nTimeout);
	SaveCookies(host, http.GetCookie());
	return code;
}

int WebClient::SubmitForm(const HttpRequest& request, const std::vector<PostForm::Field>& fieldValues, std::string* response, int nTimeout)
{
	std::string tempResponse;
	HttpTransport http;
	http.Proxy = Proxy;
	ApplyHeaders(request, http);

	std::string host = GetHost(request.url);
	ApplyCookies(host, http);

	int code = http.SubmitForm(request.url, fieldValues, response ? response : &tempResponse, nTimeout);
	SaveCookies(host, http.GetCookie());
	return code;
}
