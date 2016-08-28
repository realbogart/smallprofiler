/*
* This is a single header, cross-platform profiler
*
* Usage: 
*
*	#define PROFILER_DEFINE
*	#include "profiler.h"
*
*	call profiler_initialize() on startup. This function will measure the performance
*	of your cpu for PROFILER_MEASURE_MILLISECONDS milliseconds. This measurement is
*	later used to convert the total cycle count to seconds.
*
*	Use PROFILER_START(name) to start the profiler and PROFILER_STOP(name) to stop.
*	
*	Several start/stop calls can be nestled and the time for all blocks with the
*	same name are combined.
*
*	Call profiler_dump_file(const char* filename) to dump a performace log to file
*	Call profiler_dump_console(const char* filename) to dump a performace log to console
*
*	You can define PROFILER_DISABLE to disable all macros and functions to remove
*	all profiler overhead.
*
*	Author: Johan Yngman (johan.yngman@gmail.com)
*/

#ifndef _PROFILER_
#define _PROFILER_

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define PROFILER_NODES_MAX 256
#define PROFILER_NAME_MAXLEN 256
#define PROFILER_BUFFER_SIZE 16384
#define PROFILER_MEASURE_MILLISECONDS 100
#define PROFILER_MEASURE_SECONDS ((float)PROFILER_MEASURE_MILLISECONDS / 1000.0f)

#ifdef PROFILER_DISABLE
#define profiler_initialize()
#define profiler_reset()
#define profiler_get_results(buffer)
#define profiler_dump_file(filename)
#define profiler_dump_console()
#else
void _profiler_initialize();
void _profiler_reset();
void _profiler_get_results(char* buffer);
void _profiler_dump_file(const char* filename);
void _profiler_dump_console();
void _profiler_strncpy(char* dst, const char* src, size_t size);

#define profiler_initialize()			_profiler_initialize();
#define profiler_reset()				_profiler_reset();
#define profiler_get_results(buffer)	_profiler_get_results(buffer);
#define profiler_dump_file(filename)	_profiler_dump_file(filename);
#define profiler_dump_console()			_profiler_dump_console();
#endif // PROFILER_DISABLE

#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
static uint64_t get_cycles()
{
	return __rdtsc();
}
static unsigned long get_milliseconds()
{
	LARGE_INTEGER timestamp;
	LARGE_INTEGER frequency;

	QueryPerformanceCounter(&timestamp);
	QueryPerformanceFrequency(&frequency);

	return (unsigned long)(timestamp.QuadPart / (frequency.QuadPart / 1000));
}
#else
#include <sys/time.h>
static uint64_t get_cycles()
{
	unsigned int lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}
static unsigned long get_milliseconds()
{
	struct timeval time; 
	gettimeofday(&time, NULL);
	unsigned long milliseconds = time.tv_sec * 1000LL + time.tv_usec / 1000;
	return milliseconds;
}
#endif

struct profiler_node
{
	char name[PROFILER_NAME_MAXLEN];
	int parent_id;
	uint64_t total_cycles;
};

#ifdef PROFILER_DEFINE
int profiler_current_parent = -1;
static uint64_t profiler_cycles_measure = 0;
struct profiler_node profiler_nodes[PROFILER_NODES_MAX];

char buffer[PROFILER_BUFFER_SIZE];

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

void _profiler_initialize()
{
	profiler_reset();

	unsigned long milliseconds = get_milliseconds();
	uint64_t cycles_start = get_cycles();

	while (get_milliseconds() - milliseconds < PROFILER_MEASURE_MILLISECONDS)
		;

	profiler_cycles_measure = get_cycles() - cycles_start;
}

void _profiler_reset()
{
	int i;
	for (i = 0; i < PROFILER_NODES_MAX; i++)
	{
		profiler_nodes[i].total_cycles = 0;
		profiler_nodes[i].parent_id = -1;
		_profiler_strncpy(profiler_nodes[i].name, "", 1);
	}
}

void _profiler_dump_file(const char* filename)
{
	profiler_get_results(buffer);
	FILE* file = fopen(filename, "w");
	fputs(buffer, file);
	fclose(file);
}

static void profiler_get_results_sorted(char* buffer, int parent_id, int level)
{
	char buffer_name[PROFILER_NAME_MAXLEN];

	uint64_t max_cycles_ceil = UINT64_MAX;

	int i;
	for (i = 0; i < PROFILER_NODES_MAX; i++)
	{
		uint64_t max_cycles = 0;
		int max_index = -1;

		int j;
		for (j = 0; j < PROFILER_NODES_MAX; j++)
		{
			if (profiler_nodes[j].parent_id == parent_id)
			{
				if (profiler_nodes[j].total_cycles > max_cycles &&
					profiler_nodes[j].total_cycles < max_cycles_ceil)
				{
					max_cycles = profiler_nodes[j].total_cycles;
					max_index = j;
				}
			}
		}

		max_cycles_ceil = max_cycles;

		if (max_index != -1)
		{
			strncpy(buffer_name, "", 1);

			int j;
			for (j = 0; j < level; j++)
				strcat(buffer_name, "    ");

			strcat(buffer_name, profiler_nodes[max_index].name);

			float seconds = (float)profiler_nodes[max_index].total_cycles / ((float)profiler_cycles_measure / PROFILER_MEASURE_SECONDS);
			sprintf(buffer + strlen(buffer), "%-40s%f : %" PRIu64 "\n", buffer_name, seconds, profiler_nodes[max_index].total_cycles);
			profiler_get_results_sorted(buffer, max_index, level + 1);
		}
		else
		{
			return;
		}
	}
}

void _profiler_get_results(char* buffer)
{
	sprintf(buffer, "%-40s%s  : %s\n", "Name", "Seconds", "CPU Cycles");
	sprintf(buffer + strlen(buffer), "-------------------------------------------------------------\n");
	profiler_get_results_sorted(buffer, -1, 0);
}

void _profiler_dump_console()
{
	profiler_get_results(buffer);
	printf("%s", buffer);
}

void _profiler_strncpy(char* dst, const char* src, size_t size)
{
	strncpy(dst, src, size);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
extern int profiler_current_parent;
extern uint64_t profiler_cycles_measure;
extern profiler_node profiler_nodes[PROFILER_NODES_MAX];
#endif

#ifdef PROFILER_DISABLE
#define PROFILER_START(NAME)
#define PROFILER_STOP(NAME)
#else

#define PROFILER_START(NAME) \
	static int __profiler_id_##NAME = __COUNTER__; \
	_profiler_strncpy(profiler_nodes[__profiler_id_##NAME].name, #NAME, strlen(#NAME)+1); \
	profiler_nodes[__profiler_id_##NAME].parent_id = profiler_current_parent; \
	profiler_current_parent = __profiler_id_##NAME; \
	uint64_t __profiler_start_##NAME = get_cycles(); \

#define PROFILER_STOP(NAME) \
	profiler_nodes[__profiler_id_##NAME].total_cycles += get_cycles() - __profiler_start_##NAME; \
	profiler_current_parent = profiler_nodes[__profiler_id_##NAME].parent_id; \

#endif

#endif //_PROFILER_
