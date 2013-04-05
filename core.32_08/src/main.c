/* Add in this module code that is specific to this core */

#include "matmul.h"

int matmul_unit(void);

int main(void)
{
	int status;

	/* jump to multicore common code */
	status = matmul_unit();

	return status;
}
