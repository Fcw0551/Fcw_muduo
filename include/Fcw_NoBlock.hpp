#pragma once
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "Fcw_Log.hpp"

// 设置非阻塞
void setNoBlock(size_t fd);