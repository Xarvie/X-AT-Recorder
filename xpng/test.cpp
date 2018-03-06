#include "encoder\enc\enc.h"
#include "stdio.h"
#include "stdlib.h"
int main(int argc, char** argv)
{
	char** argvptr = (char**)malloc(sizeof(char*)*(argc-1));
	argvptr[0] = argv[0];
	for (int i = 2; i < argc; i++)
		argvptr[i-1] = argv[i];
	argc--;
	//if(argc>0)
	switch (argv[1][0])
	{
	case '1': return apng2gif_main(argc, argvptr);
	case '2': return apngasm_main(argc, argvptr);
	case '3': return apngdis_main(argc, argvptr);
	case '4': return apngopt_main(argc, argvptr);
	case '5': return gif2apng_main(argc, argvptr);
	default:
		break;
	}
		
	return 0;
}