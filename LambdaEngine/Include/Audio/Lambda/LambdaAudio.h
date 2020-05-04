#pragma once
#include <portaudio.h>

#define NOMINMAX 1

#ifdef realloc
	#undef realloc
#endif

#ifdef free
	#undef free
#endif

#include <CDSPResampler.h>

#ifdef MessageBox
	#undef MessageBox
#endif
		
#ifdef OutputDebugString
	#undef OutputDebugString
#endif

#ifdef ERROR
	#undef ERROR
#endif

#ifdef CreateWindow
	#undef CreateWindow
#endif

#ifdef UpdateResource
	#undef UpdateResource
#endif

#ifdef UNREFERENCED_PARAMETER
	#undef UNREFERENCED_PARAMETER
#endif