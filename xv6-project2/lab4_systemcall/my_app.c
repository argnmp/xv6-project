#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
	// gets the string argument given at program startup. 
	char *buf = argv[1];

	int ret_val;
	// call myfunction which is the newly create system call with the given string
	ret_val = myfunction(buf);
	// print the returned value
	printf(1, "Return value : 0x%x\n", ret_val);
	exit();
};
