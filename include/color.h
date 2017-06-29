#ifndef COLOR_H
#define COLOR_H

#if OAK_COLORIZE
	#define ERROR_MSG_COLOR	"\x1B[1m\x1B[31m"
	#define ERROR_HIGHLIGHT_COLOR	"\x1B[1m\x1B[33m"
	#define ERROR_NOTE_COLOR	"\x1B[1m\x1B[34m"
	#define ERROR_LOCATION_COLOR	"\x1B[1m\x1B[37m"
	#define RESET_COLOR		"\x1B[0m"
#else
	#define ERROR_MSG_COLOR	""
	#define ERROR_HIGHLIGHT_COLOR	""
	#define ERROR_NOTE_COLOR	""
	#define ERROR_LOCATION_COLOR	""
	#define RESET_COLOR		""
#endif /* IFDEF OAK_COLORIZE */

#endif /* IFNDEF COLOR_H */
