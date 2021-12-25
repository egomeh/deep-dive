#pragma once

#include <vector>

bool connect_to_buoy();
bool read_from_buoy(std::vector<uint8_t>& data);

