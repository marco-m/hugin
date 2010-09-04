
#if _WINDOWS && defined Hugin_shared 

#if defined huginbase_EXPORTS || defined celeste_EXPORTS || defined makefilelib_EXPORTS || defined icpfindlib_EXPORTS
#define IMPEX __declspec(dllexport)
#else
#define IMPEX __declspec(dllimport)
#endif

#if defined huginbasewx_EXPORTS
#define WXIMPEX __declspec(dllexport)
#else
#define WXIMPEX __declspec(dllimport)
#endif

#pragma warning( disable: 4251 )

#else
#define IMPEX
#define WXIMPEX
#endif
