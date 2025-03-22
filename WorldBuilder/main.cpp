#include "SVOBuilder.h"

int main(void)
{
	SVOBuilder svoBuilder(4096, 512);
	svoBuilder.build();
	return 0;
}