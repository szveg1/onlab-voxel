#include "SVDAGBuilder.h"

int main(void)
{
	SVDAGBuilder builder(1024, 512, 64);
	builder.build();
	return 0;
}