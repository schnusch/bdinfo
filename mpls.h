#ifndef MPLS_H_INCLUDED
#define MPLS_H_INCLUDED

#include <stdint.h>

int mpls_memparse(uint32_t *clips, uint8_t *data, size_t n, uint8_t na,
		uint32_t nc);

#endif
