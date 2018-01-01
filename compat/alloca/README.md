# Compatibility Module for alloca

This module contains a replacement header for alloca. To use this module
simply make sure to `#include <alloca.h>` in the file where alloca
should be used. If the header is missing the path to the replacement
header is added to the global include paths, so nothing more needs to be
done.
