// Hamster config

#pragma once


// The size of each page
#define HAMSTER_PAGE_SIZE 256
static_assert((HAMSTER_PAGE_SIZE & (HAMSTER_PAGE_SIZE - 1)) == 0, "Page size must be a power of 2");

// The number of pages loaded in RAM at once
#define HAMSTER_CONCUR_PAGES 16

// The absolute maximum number of pages
// RAM pages + swapped pages
#define HAMSTER_MAX_PAGES 16384
