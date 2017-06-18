#include <stdio.h>
#include <string.h>



void SameStringOutput(char *a, char *b, int *index)
{
	int i = 0;
	int iLenA = strlen(a);
	int iLenB = strlen(b);
	char tmpStr[256] = {0};

	*index = 0;
	while (a[i] == b[i] && ((i < iLenB) && (i < iLenA-1))) {
		i++;
	}

	if (i > 0) {
		snprintf(tmpStr, i+1, a);
		printf("%s\n", tmpStr);
		*index = i;
	}

	return;
}


void getLongestCommonString(char *a, char *b)
{
	int i,j,k;
	int iLenA = strlen(a);
	int iLenB = strlen(b);
	int index = 0;
	int iLenL = ((iLenA > iLenB) ? iLenA : iLenB);
	int iLenS = ((iLenA < iLenB) ? iLenA : iLenB);
	
	for (i = 0; i < iLenS; i++) {
		k = i;
		for (j = 0; j < iLenL; j++) {
			SameStringOutput(&a[k], &b[j], &index);
			i += index;
			j += index;
		}	
	}

}

int main(int argc, char **argv) {
	char a[256] = {"abcdefgabcd"};
	char b[256] = {"cabcd11abcd11abcd11abcd11abcd11abcd11abcd11abcd11abcd11abcd11abcd11abcd"};

	getLongestCommonString(a, b);
	return 0;
}
