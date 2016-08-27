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

#define PROFILER_NODES_MAX 256
#define PROFILER_NAME_MAXLEN 256
#define PROFILER_BUFFER_SIZE 16384
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

struct profiler_node
{
	char name[PROFILER_NAME_MAXLEN];
	int level;
	int parent_id;
	uint64_t total_cycles;
};

struct profiler_node profiler_nodes[PROFILER_NODES_MAX];

int profiler_nodes_level = 0;
int profiler_current_parent = -1;

static uint64_t profiler_cycles_measure = 0;
#endif

#ifndef PROFILER_DISABLE
char buffer[PROFILER_BUFFER_SIZE];
#endif

void profiler_reset()
{
#ifndef PROFILER_DISABLE
	for (int i = 0; i < PROFILER_NODES_MAX; i++)
	{
		profiler_nodes[i].total_cycles = 0;
		profiler_nodes[i].parent_id = -1;
		strcpy_s(profiler_nodes[i].name, 1, "");
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

void profiler_get_results_sorted(char* buffer, int parent_id, int level)
{
#ifndef PROFILER_DISABLE
	char buffer_name[PROFILER_NAME_MAXLEN];

	uint64_t max_cycles_ceil = UINT64_MAX;

	for (int i = 0; i < PROFILER_NODES_MAX; i++)
	{
		uint64_t max_cycles = 0;
		int max_index = -1;
		for (int j = 0; j < PROFILER_NODES_MAX; j++)
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
			strcpy_s(buffer_name, 1, "");
			for (int j = 0; j < level; j++)
				strcat(buffer_name, "    ");

			strcat(buffer_name, profiler_nodes[max_index].name);

			float seconds = (float)profiler_nodes[max_index].total_cycles / (float)profiler_cycles_measure;
			sprintf(buffer + strlen(buffer), "%-40s%f : %" PRIu64 "\n", buffer_name, seconds, profiler_nodes[max_index].total_cycles);
			profiler_get_results_sorted(buffer, max_index, level + 1);
		}
		else
		{
			return;
		}
	}
#endif
}

void profiler_get_results(char* buffer)
{
#ifndef PROFILER_DISABLE
	sprintf(buffer, "%-40s%s  : %s\n", "Name", "Seconds", "CPU Cycles");
	sprintf(buffer + strlen(buffer), "-------------------------------------------------------------\n");
	profiler_get_results_sorted(buffer, -1, 0);
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
	static int __profiler_id_##NAME = __COUNTER__; \
	strcpy_s(profiler_nodes[__profiler_id_##NAME].name, strlen(#NAME)+1, #NAME); \
	profiler_nodes[__profiler_id_##NAME].parent_id = profiler_current_parent; \
	profiler_current_parent = __profiler_id_##NAME; \
	uint64_t __profiler_start_##NAME = get_cycles(); \

#define PROFILER_STOP(NAME) \
	profiler_nodes[__profiler_id_##NAME].total_cycles += get_cycles() - __profiler_start_##NAME; \
	profiler_current_parent = profiler_nodes[__profiler_id_##NAME].parent_id; \

#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

#endif //_PROFILER_
