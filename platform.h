#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
//#define snprintf _snprintf 
//#define vsnprintf _vsnprintf 
#define strcasecmp _stricmp 
#define strncasecmp _strnicmp
#define strdup _strdup
#endif

#endif 