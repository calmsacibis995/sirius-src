
#include <stddef.h>

#pragma weak _memmove = memmove

void *memmove(void *dest, const void *src, size_t len)
{
	if(len == 0 || dest == src) 
		return(dest);	

	if (dest > src) {
		register const char *lasts = (const char *)src + (len-1);
		register char *lastd = (char *)dest + (len-1);
		while (len--)
			*lastd-- = *lasts--;
	}
	else {
		register const char *firsts = (const char *) src;
		register char *firstd = (char *) dest;
		while (len--)
			*firstd++ = *firsts++;
	}

	return(dest);
}
