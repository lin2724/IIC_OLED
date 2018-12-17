#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>




using namespace std;


class LogHandler
{

	public:
	
	bool b_is_init;
	
	LogHandler(const char* pclog_file_path);
	LogHandler();
	~LogHandler();
	void log(string &log_str)
	{
		real_log(log_str);
	}
	#if 0
	void log(char* file, int line, char* pcstr, int arg1)
	{
		char str_buf[1024];
		char line_buf[16];
		string log_str;
		
		snprintf(str_buf, 1024, pcstr, arg1);
		log_str += file;
		log_str += ":";
		snprintf(line_buf, 16, "%d", line);
		log_str += line_buf;
		log_str += " ";
		log_str += pcstr;
		log_str += arg1;
		
		real_log(log_str);
	}
	#endif
	void log(const char* file, int line, const char* format, ...)
	{
	   	va_list ap;
	   	int int_arg;
	   	char* char_arg;
	   	int pos;
	   	char tmp_buf[128];
	   	char time_buf[128];
	   	string format_str(format);
	   	
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		snprintf(time_buf, 128, "%d-%d-%d %d:%d:%d ",
				tm.tm_year + 1900, 
				tm.tm_mon + 1, 
				tm.tm_mday, 
				tm.tm_hour, 
				tm.tm_min, 
				tm.tm_sec);
		string front_info(time_buf);
		front_info += file;
	   	snprintf(tmp_buf, 128, ":%d ", line);
	   	front_info += tmp_buf;
	   		
		va_start(ap, format);
		pos = 0;
		
		for (;;) 
		{
			pos = format_str.find("%", 0);
			if(string::npos == pos)
			{
				break;
			}
			if((pos + 1) == format_str.find("d", pos + 1))
			{
				int_arg = va_arg(ap, int);
				snprintf(tmp_buf, 128, "%d", int_arg);
				string int_tmp(tmp_buf);
				format_str.replace(pos, 2, int_tmp);
			}
			else
			if((pos + 1) == format_str.find("s", pos + 1))
			{
				char_arg = va_arg(ap, char*);
				string char_tmp(char_arg);
				format_str.replace(pos, 2, char_tmp);
			}
			else
			{
				break;
			}
		}
		va_end(ap);
		front_info += format_str;
		real_log(front_info);

	}
	#if 0
	void log(char* file, char* line, char* pcstr, int arg1, int arg2);
	void log(char* file, char* line, char* pcstr, char* arg1, char* arg2);
	void log(char* file, char* line, char* pcstr, char arg1, int arg2);
	#endif
	private:

	void real_log(string &pcstr);
	
	int fd;

};

extern LogHandler* gLogHandler;


#define LOG_0(arg0) gLogHandler->log(__FILE__, __LINE__, arg0)
#define LOG_1(arg0, arg1) gLogHandler->log(__FILE__, __LINE__, arg0, arg1)
#define LOG_2(arg0, arg1, arg2) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2)
#define LOG_3(arg0, arg1, arg2, arg3) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2, arg3)
#define LOG_4(arg0, arg1, arg2, arg3, arg4) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2, arg3, arg4)
#define LOG_5(arg0, arg1, arg2, arg3, arg4, arg5) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2, arg3, arg4, arg5)
#define LOG_6(arg0, arg1, arg2, arg3, arg4, arg5, arg6) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2, arg3, arg4, arg5, arg6)
#define LOG_7(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define LOG_8(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define LOG_9(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) gLogHandler->log(__FILE__, __LINE__, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)



int log_init(const char* log_file_path);















#endif
