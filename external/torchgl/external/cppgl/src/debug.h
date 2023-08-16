#pragma once

#include <iostream>
#include <memory>
#include "platform.h"

CPPGL_NAMESPACE_BEGIN

void enable_strack_trace_on_crash();
void disable_stack_trace_on_crash();

void enable_gl_debug_output();
void disable_gl_debug_output();

void enable_gl_notifications();
void disable_gl_notifications();

CPPGL_NAMESPACE_END
