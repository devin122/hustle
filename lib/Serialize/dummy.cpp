/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BinaryStream.hpp"
#include "hustle/cell.hpp"
using namespace hustle;
struct Context {};

// Bogus symbol so that osx won't complain
int hustle_serialize_dummy() { return 0; }
