#pragma once
#include "common.h"

typedef struct
{
	vu32* table; // L2 table
	semaphore_t* mutex; // VM mutex
	
	vu32* l1Entries; // Paired L1 entries for this L2 table page
	pageinfo_t* page; // Page that holds the L2 table
} l2info_t;
