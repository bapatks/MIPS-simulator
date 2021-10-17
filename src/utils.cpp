#include "utils.h"

int CheckBuff2Empty(unsigned long int* buffer)
{
	int i;
	//printf("buffer size: %d",sizeof(buffer));
	for (i = 0; i < 2; i++)
	{
		if (buffer[i] == 0x0000)
		{
			//printf("\n%d\n",i);
			return i;
		}
	}
	return -1;

}