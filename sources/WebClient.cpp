#include "WebClient.h"

#include <cctype>
#include <cstdlib>
#include <ctime>

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

	std::string NormalizeCookieHost(const std::string& host)
	{
		std::string value = Trim(host);
		const std::string httpOnlyPrefix = "#HttpOnly_";
		if (StartsWith(value, httpOnlyPrefix)) {
			value = value.substr(httpOnlyPrefix.size());
		}
		return ToLower(value);
	}

	std::vector<std::string> SplitCookieList(const std::string& cookieText)
	{
		std::vector<std::string> items;
		if (cookieText.find('\n') != std::string::npos) {
			size_t begin = 0;
			while (begin <= cookieText.size()) {
				size_t end = cookieText.find('\n', begin);
				if (end == std::string::npos) {
					items.push_back(cookieText.substr(begin));
					break;
				}
				items.push_back(cookieText.substr(begin, end - begin));
				begin = end + 1;
			}
			return items;
		}

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
		for (const auto& item : request.Header) {
			http.AddHeader(item.first, item.second);
		}
	}

	bool CookieExpired(const Cookie& cookie)
	{
		return cookie.expires > 0 && cookie.expires <= (long long)std::time(NULL);
	}

	bool SameCookieKey(const Cookie& left, const Cookie& right)
	{
		return NormalizeCookieHost(left.domain) == NormalizeCookieHost(right.domain) &&
			left.path == right.path &&
			left.name == right.name;
	}

	std::string GetUrlPath(const std::string& strUrl)
	{
		std::string url = Trim(strUrl);
		size_t begin = url.find("://");
		begin = begin == std::string::npos ? 0 : begin + 3;

		size_t path = url.find('/', begin);
		if (path == std::string::npos) {
			return "/";
		}

		size_t end = url.find_first_of("?#", path);
		std::string value = url.substr(path, end == std::string::npos ? std::string::npos : end - path);
		return value.empty() ? "/" : value;
	}

	bool IsSecureUrl(const std::string& strUrl)
	{
		std::string url = Trim(strUrl);
		size_t schemeEnd = url.find("://");
		return schemeEnd != std::string::npos && ToLower(url.substr(0, schemeEnd)) == "https";
	}

	bool CookiePathMatches(const std::string& requestPath, const std::string& cookiePath)
	{
		std::string storedPath = cookiePath.empty() ? "/" : cookiePath;
		std::string path = requestPath.empty() ? "/" : requestPath;
		if (storedPath == "/" || path == storedPath) {
			return true;
		}
		if (!StartsWith(path, storedPath)) {
			return false;
		}
		return storedPath.back() == '/' || (path.size() > storedPath.size() && path[storedPath.size()] == '/');
	}

	bool CookieMoreSpecific(const Cookie& candidate, const Cookie& current)
	{
		if (candidate.path.size() != current.path.size()) {
			return candidate.path.size() > current.path.size();
		}
		return NormalizeCookieHost(candidate.domain).size() > NormalizeCookieHost(current.domain).size();
	}

	void NormalizeCookie(Cookie& cookie)
	{
		cookie.domain = Trim(cookie.domain);
		const std::string httpOnlyPrefix = "#HttpOnly_";
		if (StartsWith(cookie.domain, httpOnlyPrefix)) {
			cookie.httpOnly = true;
			cookie.domain = cookie.domain.substr(httpOnlyPrefix.size());
		}
		cookie.domain = ToLower(cookie.domain);
		if (cookie.path.empty()) {
			cookie.path = "/";
		}
		else if (cookie.path[0] != '/') {
			cookie.path = "/" + cookie.path;
		}
		if (!cookie.domain.empty() && cookie.domain[0] == '.') {
			cookie.includeSubDomain = true;
		}
		else if (cookie.includeSubDomain && !cookie.domain.empty()) {
			cookie.domain = "." + cookie.domain;
		}
	}

	Cookie MakeSingleCookie(const Cookie& cookie, const std::string& key, const std::string& value)
	{
		Cookie item = cookie;
		item.fields.clear();
		item.name.clear();
		item.value.clear();
		item.AddField(key, value);
		return item;
	}

	void RemoveCookieLocked(std::vector<Cookie>& cookies, const Cookie& cookie)
	{
		for (auto it = cookies.begin(); it != cookies.end(); ) {
			if (SameCookieKey(*it, cookie)) {
				it = cookies.erase(it);
			}
			else {
				++it;
			}
		}
	}
}

void Cookie::AddField(const std::string& key, const std::string& fieldValue)
{
	if (key.empty()) {
		return;
	}

	fields[key] = fieldValue;
	if (name.empty() || name == key) {
		name = key;
		value = fieldValue;
	}
}

void Cookie::RemoveField(const std::string& key)
{
	fields.erase(key);
	if (name != key) {
		return;
	}

	if (fields.empty()) {
		name.clear();
		value.clear();
		return;
	}

	name = fields.begin()->first;
	value = fields.begin()->second;
}

std::string Cookie::GetField(const std::string& key)const
{
	auto item = fields.find(key);
	return item == fields.end() ? "" : item->second;
}

Cookie Cookie::FromCurlCookie(const std::string& curlCookie)
{
	auto columns = Split(curlCookie, '\t');
	if (columns.size() < 7) {
		return Cookie();
	}

	Cookie parsed;
	parsed.domain = columns[0];
	parsed.includeSubDomain = ToLower(columns[1]) == "true";
	parsed.path = columns[2].empty() ? "/" : columns[2];
	parsed.secure = ToLower(columns[3]) == "true";
	parsed.expires = std::atoll(columns[4].c_str());
	NormalizeCookie(parsed);
	parsed.AddField(columns[5], columns[6]);
	if (parsed.name.empty()) {
		return Cookie();
	}

	return parsed;
}

std::string Cookie::ToCurlCookie()const
{
	std::string cookieDomain = domain;
	if (httpOnly && !StartsWith(cookieDomain, "#HttpOnly_")) {
		cookieDomain = "#HttpOnly_" + cookieDomain;
	}
	std::string cookieName = name;
	std::string cookieValue = value;
	if (cookieName.empty() && !fields.empty()) {
		cookieName = fields.begin()->first;
		cookieValue = fields.begin()->second;
	}

	return cookieDomain + "\t" +
		(includeSubDomain ? "TRUE" : "FALSE") + "\t" +
		(path.empty() ? "/" : path) + "\t" +
		(secure ? "TRUE" : "FALSE") + "\t" +
		std::to_string(expires) + "\t" +
		cookieName + "\t" + cookieValue;
}

std::string Cookie::ToHeaderString()const
{
	if (fields.empty()) {
		return name.empty() ? "" : name + "=" + value;
	}

	std::string cookieText;
	for (auto& item : fields) {
		if (!cookieText.empty()) {
			cookieText += "; ";
		}
		cookieText += item.first + "=" + item.second;
	}
	return cookieText;
}

bool Cookie::MatchesHost(const std::string& requestHost)const
{
	std::string host = ToLower(Trim(requestHost));
	std::string cookieDomain = NormalizeCookieHost(domain);
	if (cookieDomain.empty()) {
		return true;
	}

	if (host == cookieDomain) {
		return true;
	}

	if (cookieDomain[0] == '.') {
		std::string rootDomain = cookieDomain.substr(1);
		return host == rootDomain || EndsWith(host, cookieDomain);
	}

	if (includeSubDomain) {
		return EndsWith(host, "." + cookieDomain);
	}
	return false;
}

WebClient::WebClient()
{
}

WebClient::~WebClient()
{
}

std::string WebClient::GetHost(const std::string& strUrl)const
{
	std::string url = Trim(strUrl);
	size_t begin = url.find("://");
	begin = begin == std::string::npos ? 0 : begin + 3;

	size_t end = url.find_first_of("/?#", begin);
	std::string host = url.substr(begin, end == std::string::npos ? std::string::npos : end - begin);

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

void WebClient::ApplyCookies(const std::string& url, HttpTransport& http)
{
	std::string host = GetHost(url);
	std::string path = GetUrlPath(url);
	bool secureRequest = IsSecureUrl(url);
	std::map<std::string, Cookie> selectedCookies;

	{
		std::lock_guard<std::mutex> lock(cookiesMutex);
		for (auto it = cookies.begin(); it != cookies.end(); ) {
			if (CookieExpired(*it)) {
				it = cookies.erase(it);
				continue;
			}

			if (!it->name.empty() &&
				it->MatchesHost(host) &&
				CookiePathMatches(path, it->path) &&
				(!it->secure || secureRequest)) {
				auto existing = selectedCookies.find(it->name);
				if (existing == selectedCookies.end() || CookieMoreSpecific(*it, existing->second)) {
					selectedCookies[it->name] = *it;
				}
			}
			++it;
		}
	}

	for (const auto& item : selectedCookies) {
		http.AddCookie(item.second.name, item.second.value);
	}
}

void WebClient::SaveCookies(const std::string& host, const Text::String& cookieText)
{
	for (auto& item : SplitCookieList(cookieText)) {
		item = Trim(item);
		if (item.empty()) {
			continue;
		}

		Cookie cookie = Cookie::FromCurlCookie(item);
		if (!cookie.name.empty()) {
			if (cookie.domain.empty()) {
				cookie.domain = host;
			}
			SetCookie(cookie);
		}
	}
}

bool WebClient::SaveCookie(const Cookie& cookie)
{
	if (cookie.name.empty()) {
		return false;
	}

	std::lock_guard<std::mutex> lock(cookiesMutex);
	for (auto& item : cookies) {
		if (SameCookieKey(item, cookie)) {
			item = cookie;
			return true;
		}
	}
	cookies.push_back(cookie);
	return true;
}

bool WebClient::SetCookie(const Cookie& cookie)
{
	Cookie newCookie = cookie;
	NormalizeCookie(newCookie);
	if (newCookie.fields.empty() && !newCookie.name.empty()) {
		newCookie.AddField(newCookie.name, newCookie.value);
	}

	if (newCookie.fields.empty()) {
		return false;
	}

	if (CookieExpired(newCookie)) {
		std::lock_guard<std::mutex> lock(cookiesMutex);
		for (const auto& field : newCookie.fields) {
			RemoveCookieLocked(cookies, MakeSingleCookie(newCookie, field.first, field.second));
		}
		return false;
	}

	bool saved = false;
	for (const auto& field : newCookie.fields) {
		saved = SaveCookie(MakeSingleCookie(newCookie, field.first, field.second)) || saved;
	}
	return saved;
}

Cookie WebClient::GetCookie(const std::string& host)
{
	std::string requestHost = GetHost(host);
	Cookie result;
	result.domain = requestHost;

	std::lock_guard<std::mutex> lock(cookiesMutex);
	for (auto it = cookies.begin(); it != cookies.end(); ) {
		if (CookieExpired(*it)) {
			it = cookies.erase(it);
			continue;
		}

		if (it->MatchesHost(requestHost) && !it->name.empty()) {
			result.AddField(it->name, it->value);
		}
		++it;
	}
	return result;
}

int WebClient::HttpGet(const HttpRequest& request, std::string* response, int nTimeout)
{
	std::string tempResponse;
	HttpTransport http;
	http.Proxy = Proxy;
	ApplyHeaders(request, http);

	std::string host = GetHost(request.url);
	ApplyCookies(request.url, http);

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
	ApplyCookies(request.url, http);

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
	ApplyCookies(request.url, http);

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
	ApplyCookies(request.url, http);

	int code = http.SubmitForm(request.url, fieldValues, response ? response : &tempResponse, nTimeout);
	SaveCookies(host, http.GetCookie());
	return code;
}
