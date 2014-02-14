/*
 * tb_logging.c
 *
 *  Created on: 28/01/2014
 *      Author: michael
 */

#include "tb_logging.h"
#include "tb_common.h"

#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

tb_log_t
*tb_create_log(char *file_path)
{
	tb_log_t *log = malloc(sizeof(tb_log_t));
	log->file_path = strdup(file_path);

	if((log->file = fopen(file_path, "a+")) == NULL)
	{
		perror("tb_create_log: fopen");
		return NULL;
	}

	return log;
}

tb_log_t
*tb_create_flog(char *file_path, tb_log_format_t format)
{
	char fmt_path[50];
	char time_str[30];

	tb_get_f_time(time_str, sizeof time_str, "%F");

#ifdef _DEBUG_LOGGING
	PRT_I_S("tb_create_flog: time_str: ", time_str)
#endif

	snprintf(fmt_path, sizeof fmt_path, "%s_%s", file_path, time_str);

	return tb_create_log(fmt_path);
}

void
tb_destroy_log(tb_log_t *log)
{
	fclose(log->file);
	free(log->file_path);
	free(log);
}

int
tb_write_log(tb_log_t *log, char *info, tb_log_type_t type)
{
	assert(log != NULL && info != NULL && type >= LOG_INFO && type <= LOG_ERR);

	char buff[100];
	char time_str[30];
	char *m_type;

#ifdef _DEBUG_LOGGING
	PRT_INFO("Get the current time, prepend it to info")
	PRT_I_S("info: ", info)
#endif

	tb_get_f_time(time_str, sizeof(time_str), "%c");

#ifdef _DEBUG_LOGGING
	PRT_INFO("Determine type of log")
	PRT_I_S("time_str", time_str)
#endif

	switch(type)
	{
	case LOG_INFO: m_type = L_INFO; break;
	case LOG_ACK: m_type = L_ACK; break;
	case LOG_ERR: m_type = L_ERR; break;
	}

#ifdef _DEBUG_LOGGING
	PRT_INFO("Create string")
#endif

	snprintf(buff, sizeof buff, "%s [ %s ] %s\n", m_type, time_str, info);

#ifdef _DEBUG_LOGGING
	PRT_I_S("Log string: ", buff)
	PRT_INFO("Write to file")
#endif

	if(fputs(buff, log->file) == EOF)
	{
		perror("Cannot write to log");
		return -1;
	}

	if(fflush(log->file) == EOF)
	{
		perror("Cannot flush file");
		return -1;
	}

#ifdef _DEBUG_LOGGING
	PRT_INFO("Log written")
#endif

	return 0;
}

void
tb_get_f_time(char *time_str, size_t len, const char *format)
{
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(time_str, len, format, timeinfo);
}

void
tb_log_error_no(tb_log_t *log, int log_en, const char *info, int err_no)
{
	char log_str[50];

	snprintf(log_str, sizeof(log_str), "%s%s", info, strerror(err_no));

	tb_log_info(log, log_en, log_str, LOG_ERR);

	fprintf(stderr, "%s\n", log_str);
}

void
tb_log_info(tb_log_t *log, int log_en, const char *log_str, tb_log_type_t type)
{
	if(log_en && log != NULL)
	{
		tb_write_log(log, log_str, type);
	}

	switch(type)
	{
	case LOG_INFO:
		fprintf(stdout, INFO("%s\n"), log_str);
		break;
	case LOG_ACK:
		fprintf(stdout, ACK("%s\n"), log_str);
		break;
	case LOG_ERR:
		fprintf(stdout, ERR("%s\n"), log_str);
	}

}

