/** @author Maya B. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../common/utils.h"
#include "lineReader.h"

/** @brief Acquires a line from the stdin and returns it.
 * @return a string containing the contents of the input.
 */
char* getLine() {
	int bufferSize = FILENAME_MAX;
	int pos = 0;
	char *line = malloc(bufferSize);
	int c;

	if (!line)
        FAIL_SUCCESFULLY("mAllocation Error");

	while (1) {
		c = getchar();	// Get a character.
		if (c == EOF || c == '\n') {		// Acquire end of line (Enter key)
			line[pos] = '\0';
			return line;
		} else
			line[pos] = c;	// Append to buffer

		if (pos >= bufferSize) {	// Reallocate if necessary
			bufferSize += FILENAME_MAX;
			line = realloc(line, bufferSize);
			if (!line) FAIL_SUCCESFULLY("mAllocation Error");
		}
		pos++;
	}
}