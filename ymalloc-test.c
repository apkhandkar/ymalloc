/** 
 * ymalloc-test: demonstrates the simple header integrity
 * check mechanism built into ymalloc
 */

#include <stdio.h>
#include <string.h>
#include "ymalloc.h"

int 
main(void) 
{
	/* using the allocated space properly */

	char * str = (char*) ymalloc(strlen("hello, world!\n"));

	strcpy(str, "hello, world!\n");

	printf("%s", str);

	yfree(str);

	/* now, let's use it poorly by writing more than what was allocated */

	str = (char*) ymalloc(strlen("hello, world!\n"));

	strcpy(str, "hello, cruel world!\n");

	printf("%s", str);

	/* an assertion should fail at this point */

	yfree(str);

	return 0;
}
