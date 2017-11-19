#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;


#include "log.h"

LogHandler* gLogHandler;


LogHandler::LogHandler(const char* pclog_file_path)
{
	fd = open(pclog_file_path, O_WRONLY|O_APPEND|O_CREAT, S_IRWXU|S_IRWXG);
	int ret = 0;
	if(0 < fd)
	{
		b_is_init = true;
		printf("Succeed to open [%s] ret [%d]\n", pclog_file_path, fd);
		
	}
	else
	{
		printf("Failed to open [%s] ret [%d]\n", pclog_file_path, fd);
		printf("open ret [%d], [%d]\n", ret, errno);
	}
	
}

LogHandler::LogHandler()
{
	string log_file_path;
	log_file_path += "/var/log/";
	log_file_path += __FILE__;
	log_file_path += ".log";
	fd = open(log_file_path.c_str(), O_WRONLY|O_APPEND|O_CREAT, S_IRWXU|S_IRWXG);
	if(0 < fd)
	{
		b_is_init = true;
		printf("Succeed to open [%s] ret [%d]\n", log_file_path.c_str(), fd);
	}
	else
	{
		printf("Failed to open [%s] ret [%d]\n", log_file_path.c_str(), fd);
	}
	
}
LogHandler::~LogHandler()
{
	if(b_is_init)
	{
		close(fd);
	}
}


void LogHandler::real_log(string &pcstr)
{
	if (false == b_is_init)
	{
		return;
	}
	write(fd, pcstr.c_str(), pcstr.length());
	write(fd, "\n", 1);
}

/*
*@param: NULL or log_file_apth
*return: 0:succeed  -1:failed
*
*
*/
int log_init(const char* log_file_path)
{
	if(NULL != log_file_path)
	{
		gLogHandler = new LogHandler(log_file_path);
	}
	else
	{
		gLogHandler = new LogHandler();
	}
	if(gLogHandler->b_is_init)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}
