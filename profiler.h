/*
* This is a single header, cross-platform profiler
*
* Usage: 
*	call profiler_initialize() on startup. This function will measure the performance
*	of your cpu for PROFILER_MEASURE_MILLISECONDS milliseconds. This measurement is
*	later used to convert the total cycle count to seconds.
*
*	Use PROFILER_START(name) to start the profiler and PROFILER_STOP(name) to stop.
*	
*	Several start/stop calls can be nestled and the time for all blocks with the
*	same name are combined.
*
*	Call profiler_dump() to dump a performace log with name PROFILER_LOG_NAME to
*	a file. Default is profiler.txt
*
*	You can define PROFILER_DISABLE to disable all macros and functions to remove
*	all profiler overhead.
*
*	Author: Johan Yngman (johan.yngman@gmail.com)
*/

#ifndef _PROFILER_
#define _PROFILER_

#ifdef _WIN32
#pragma warning( push )
#pragma warning(disable:4996)
#endif

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#define PROFILE_NODES_MAX 256
#define PROFILE_NAME_MAXLEN 256
#define PROFILE_BUFFER_SIZE 16384
#define PROFILER_MEASURE_MILLISECONDS 1000
#define PROFILER_LOG_NAME "profiler.txt"

#ifndef PROFILER_DISABLE
#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
uint64_t get_cycles()
{
	return __rdtsc();
}
unsigned long get_milliseconds()
{
	return GetTickCount();
}
#else
uint64_t get_cycles()
{
	unsigned int lo, hi;
	__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}
unsigned long get_milliseconds()
{
	return 1; // TODO: Implement this
}
#endif

struct profile_node
{
	char name[PROFILE_NAME_MAXLEN];
	int level;
	uint64_t total_cycles;
};

struct profile_node profile_nodes[256];
int profile_nodes_level = 0;

static uint64_t profiler_cycles_measure = 0;
#endif

#ifndef PROFILER_DISABLE
char buffer[PROFILE_BUFFER_SIZE];
#endif

void profiler_reset()
{
#ifndef PROFILER_DISABLE
	for (int i = 0; i < PROFILE_NODES_MAX; i++)
	{
		profile_nodes[i].total_cycles = 0;
		profile_nodes[i].level = 0;
		strcpy_s(profile_nodes[i].name, 1, "");
	}
#endif
}

void profiler_initialize()
{
#ifndef PROFILER_DISABLE
	profiler_reset();

	unsigned long milliseconds = get_milliseconds();
	uint64_t cycles_start = get_cycles();

	while (get_milliseconds() - milliseconds < PROFILER_MEASURE_MILLISECONDS)
		;

	profiler_cycles_measure = get_cycles() - cycles_start;
#endif
}

void profiler_get_results(char* buffer)
{
#ifndef PROFILER_DISABLE
	char buffer_name[PROFILE_NAME_MAXLEN];

	sprintf(buffer, "%-40s%s  : %s\n", "Name", "Seconds", "CPU Cycles");
	sprintf(buffer + strlen(buffer), "--------------------------------------------------------------\n");

	for (int i = 0; i < PROFILE_NODES_MAX; i++)
	{
		if (strlen(profile_nodes[i].name) == 0)
			break;

		strcpy_s(buffer_name, 1, "");
		for (int j = 0; j < profile_nodes[i].level; j++)
			strcat(buffer_name, "    ");

		strcat(buffer_name, profile_nodes[i].name);

		float seconds = (float)profile_nodes[i].total_cycles / (float)profiler_cycles_measure;
		sprintf(buffer + strlen(buffer), "%-40s%f : %" PRIu64 "\n", buffer_name, seconds, profile_nodes[i].total_cycles);
	}
#endif
}

void profiler_dump()
{
#ifndef PROFILER_DISABLE
	profiler_get_results(buffer);
	FILE* file = fopen(PROFILER_LOG_NAME, "w");
	fputs(buffer, file);
	fclose(file);
#endif
}

void profiler_dump_console()
{
#ifndef PROFILER_DISABLE
	profiler_get_results(buffer);
	printf("%s", buffer);
#endif
}

#ifdef PROFILER_DISABLE
#define PROFILER_START(NAME)
#define PROFILER_STOP(NAME)
#else

#define PROFILER_START(NAME) \
	static int __profile_id_##NAME = __COUNTER__; \
	strcpy_s(profile_nodes[__profile_id_##NAME].name, strlen(#NAME)+1, #NAME); \
	profile_nodes[__profile_id_##NAME].level = profile_nodes_level; \
	profile_nodes_level++; \
	uint64_t __profile_start_##NAME = get_cycles(); \

#define PROFILER_STOP(NAME) \
	profile_nodes[__profile_id_##NAME].total_cycles += get_cycles() - __profile_start_##NAME; \
	profile_nodes_level--; \

#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

#endif //_PROFILER_
