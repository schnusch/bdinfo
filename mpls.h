/* Copyright (C) 2016 Schnusch

   This file is part of bdinfo.

   bdinfo is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   bdinfo is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with bdinfo.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef MPLS_H_INCLUDED
#define MPLS_H_INCLUDED

#include <stdint.h>

int mpls_memparse(uint32_t *clips, uint8_t *data, size_t n, uint8_t na,
		uint32_t nc);

#endif
