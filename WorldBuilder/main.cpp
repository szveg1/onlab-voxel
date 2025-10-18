#include "SVDAGBuilder.h"

int main(void)
{
	SVDAGBuilder builder(256, 256, 128);
	builder.build();
	return 0;
}