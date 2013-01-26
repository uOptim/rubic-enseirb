#include <stdio.h>
#include <stdint.h>

int32_t putsI(int32_t i) {
	return printf("%d\n", i);
}

int32_t putsF(double f) {
	return printf("%lf\n", f);
}

int32_t putsB(int8_t b) {
	if (b == 0) {
		return printf("false\n");
	}
	else {
		return printf("true\n");
	}
}

int32_t putsS(char * s) {
	return printf("%s\n", s);
}

