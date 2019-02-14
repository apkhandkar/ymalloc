/**
 * yfree-safe-test: demonstrates yfree-safe in [experimental/] ymalloc
 * 
 * the block to be freed is zeroed-out, resulting in a slower freeing 
 * but is arguably more secure
 */

#include <stdio.h>
#include <string.h>
#include "ymalloc.h"

int 
main(void)
{
	char *str = (char*) ymalloc(strlen("Hello world\n"));
	
	strcpy(str, "Hello world\n");

	printf("'str': %s", str);

	/* using yfree */
	
	yfree(str);
	
	/* can we still print the string? */

	printf("After yfree'ing 'str': %s", str);

	str = (char*) ymalloc(strlen("Hello world\n"));

	/* using yfree_safe */

	yfree_safe(str);

	/* can we still print the string? */

	printf("After yfree_safe'ing 'str': %s", str);

	printf("\n");

	return 0;
}
