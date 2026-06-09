#include "Log.h"
namespace Log {
	//是否启用日志
	bool WriteFile = true;
	std::mutex global_log_mutex;
	static std::mutex global_file_log_mutex;  // 单独保护文件写入
	void WriteLog(const Text::String& log)
	{
		std::lock_guard<std::mutex> autoLock(global_file_log_mutex);

		if (!WriteFile)return;
#ifdef _DEBUG
		Text::String logPath = Path::StartPath() + "\\" + Path::GetFileNameWithoutExtension(Path::StartFileName()) + "_Log";
#else
		Text::String logPath = Path::GetAppTempPath() + "_Log";
#endif
		Directory::Create(logPath);
		Text::String logFile = logPath + "\\" + DateTime::Now().ToString("yyyy-MM-dd") + ".log";
		std::ofstream ofs(logFile.unicode(), std::ios::binary | std::ios::app);
		ofs.write(log.c_str(), log.size());
		ofs.flush();
		ofs.close();
	}
};

