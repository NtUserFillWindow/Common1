#pragma once
#include <vector>
#include <string>
#include <Windows.h>

namespace Text {
	//-----------------------------------------------Copy Start-----------------------------------------------
		/// UTF-8 字符串类，继承自 std::string。
		/// 内部存储为 UTF-8 编码，建议使用宽字符构造（std::wstring 或 wchar_t*）
		/// 以保证编码正确。
		/// 注意：不要通过 std::string* 删除动态分配的 String，因为 std::string 没有虚析构函数。
	class String :public std::string {
	public:
		String();
		virtual ~String();

		String(const String& right);
		String(String&& right);
		String& operator=(const String& right);
		String& operator=(String&& right);

		String(const char* szbuf);
		String(const char* pStr, size_t count);
		String(const std::string& str);

		String(const wchar_t* szbuf);
		String(const std::wstring& wstr);

		//返回utf8字符个数
		size_t utf8Length() const;
		//获取unicode字符串
		std::wstring unicode() const;
		//转为当前系统可以正常显示的字符串
		std::string ansi() const;
		void erase(char _ch);
		void erase(size_t pos, size_t count);
		String replace(char oldChar, char newChar)const;
		String replace(const String& oldText, const String& newText, bool allReplace = true)const;
		String toLower()const;
		String toUpper()const;
		int toInt()const;
		float toFloat()const;
		double toDouble()const;
		int64_t toInt64()const;
		//判断字符串是否包含一个字符串
		bool contains(const String& str)const;
		//去除前后空格
		String trim()const;
		//find value count
		size_t count(const String& value)const;
		//字符串分割
		std::vector<String> split(const String& ch)const;
		//字符串分割
		std::vector<String> split(char ch)const;
		bool operator==(const wchar_t* szbuf)const;
		bool operator==(const std::wstring& wStr)const;
		bool operator!=(const wchar_t* szbuf)const;
		bool operator!=(const std::wstring& wStr)const;
	public:
		//从当前系统的字符串转为utf8字符串
		static String fromLocal(const std::string& localStr);
	};

	//base convert
	void AnyToUnicode(const std::string& src_str, UINT codePage, std::wstring* out_wstr);
	void UnicodeToAny(const std::wstring& unicode_wstr, UINT codePage, std::string* out_str);
	//
	void GBKToUTF8(const std::string& str, std::string* outStr);
	void UTF8ToGBK(const std::string& str, std::string* outStr);
	void ANSIToUniCode(const std::string& str, std::wstring* outStr);
	void ANSIToUTF8(const std::string& str, std::string* outStr);
	void UnicodeToANSI(const std::wstring& wstr, std::string* outStr);
	void UnicodeToUTF8(const std::wstring& wstr, std::string* outStr);
	void UTF8ToANSI(const std::string& str, std::string* outStr);
	void UTF8ToUnicode(const std::string& str, std::wstring* outStr);
	//
	void Tolower(std::string* str_in_out);
	void Toupper(std::string* str_in_out);
	void Erase(std::string* str_in_out, char ch);
	void Replace(std::string* str_in_out, char oldChar, char newChar);
	size_t Replace(std::string* str_in_out, const std::string& oldText, const std::string& newText, bool replaceAll = true);
	void Split(const std::string& str_in, const std::string& ch, std::vector<std::string>* strs_out);

	/// <summary>
	/// 数字转为字符串(保留小数位数)
	/// </summary>
	/// <param name="number">数字</param>
	/// <param name="precision">保留小数位数</param>
	/// <returns></returns>
	String ToString(double number, int precision);
	//将指针转换成16进制字符串
	String ToString(void* ptr);
	//转换成"true"或"false"字符串
	String ToString(bool v);
	inline String ToString(char v) { return std::to_string((long long)v); }
	inline String ToString(signed char v) { return std::to_string((long long)v); }
	inline String ToString(unsigned char v) { return std::to_string((unsigned long long)v); }
	inline String ToString(short v) { return std::to_string((long long)v); }
	inline String ToString(unsigned short v) { return std::to_string((unsigned long long)v); }
	inline String ToString(int v) { return std::to_string((long long)v); }
	inline String ToString(unsigned int v) { return std::to_string((unsigned long long)v); }
	inline String ToString(long v) { return std::to_string((long long)v); }
	inline String ToString(unsigned long v) { return std::to_string((unsigned long long)v); }
	inline String ToString(long long v) { return std::to_string((long long)v); }
	inline String ToString(unsigned long long v) { return std::to_string((unsigned long long)v); }
	inline String ToString(float v) { return std::to_string((long double)v); }
	inline String ToString(double v) { return std::to_string((long double)v); }
	inline String ToString(long double v) { return std::to_string((long double)v); }
	//-----------------------------------------------Copy End-----------------------------------------------
};
using u8String = Text::String;
