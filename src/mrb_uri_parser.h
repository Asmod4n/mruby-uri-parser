#ifndef MRB_URI_PARSER_H
#define MRB_URI_PARSER_H

#include <mruby/string.h>
#include <errno.h>
#include <mruby/error.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <netdb.h>
#include <arpa/inet.h>
#endif

#endif
