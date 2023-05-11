#include "types.h"
#include "defs.h"

// create new system call that prints given char* type parameter.
// this system call returns int type value, which is 0xABCDABCD.
// this function is called from the wrapper function below.
int myfunction(char* str)
{
	cprintf("%s\n", str);
	return 0xABCDABCD;
}

// this is a wrapper function of myfunction.
// After this function extracts parameter, it calls myfunction with the parameter
int sys_myfunction(void)
{
	char* str;
	
	// argstr function from syscall.c file
	// argstr function extracts char* type system call parameter safely by checking 
	// if it ends with a null character, etc. 
	if(argstr(0, &str) < 0)
		return -1;
	// call myfunction with the extracted parameter and return the value that is returned by my_function.
	return myfunction(str);
}
