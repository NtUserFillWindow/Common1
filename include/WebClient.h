#pragma once
#include "HttpTransport.h"

struct HttpRequest {
	std::string url;
	std::map<std::string, std::string> Header;

	HttpRequest() {}
	HttpRequest(const std::string& url) : url(url) {}
	void AddHeader(const std::string& key, const std::string& value) { Header[key] = value; }
	void AddHreader(const std::string& key, const std::string& value) { AddHeader(key, value); }
};

// HTTP 客户端，自动管理 cookie 和重定向
class WebClient {
	struct cookie {
		std::string host;
		std::string cookieStr;
	};
	std::vector<cookie> cookies;
	mutable std::mutex cookiesMutex;
	std::string GetHost(const std::string& strUrl)const;
	void ApplyCookies(const std::string& host, HttpTransport& http)const;
	void SaveCookies(const std::string& host, const Text::String& cookieText);
public:
	std::string Proxy;
	WebClient();
	virtual ~WebClient();
	void SetCookie(const std::string& host, const std::string& cookieStr);
	std::string GetCookie(const std::string& host);
	int HttpGet(const HttpRequest& request, std::string* response = NULL, int nTimeout = 60);
	int HttpPost(const HttpRequest& request, const std::string& data = "", std::string* response = NULL, int nTimeout = 60);
	int DownloadFile(const HttpRequest& request, const std::wstring& filename, const std::function<void(long long dltotal, long long dlnow)>& progressCallback = NULL, int nTimeout = 99999);
	int SubmitForm(const HttpRequest& request, const std::vector<PostForm::Field>& fieldValues, std::string* response = NULL, int nTimeout = 99999);
};
