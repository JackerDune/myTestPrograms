#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

int *productOfArrayExSelf(int *pArray, int len)
{
	int left = 1;
	int right = 1;
	int *pResult = NULL;
	int i = 0;
	pResult = (int *)malloc(len * sizeof(int));
	if (NULL == pResult) {
		printf("malloc failed\n");
		return NULL;
	}
	//calculate left value of this element
	pResult[0] = left;
	for (i=1; i < len; i++) 
		pResult[i] = pArray[i-1]* pResult[i-1]; //1,1,2,6
	//calculate right value of this element
	pResult[len-1] *= right;
	for (i=len -2; i>=0; i--)  {
		right *= pArray[i+1];
		pResult[i]  *= right; 
	}

	return pResult;	
}

int main(void)
{
	int *pRet = NULL;
	int aiTest[] = {1,2,3,4};
	int i = 0;

	pRet = productOfArrayExSelf(aiTest, 4);
	if (pRet) {
		for (i = 0; i < 4; i++) {
			printf("%d ", pRet[i]);
		}
		printf("\n");
	}
	
	if (pRet != NULL)
		free(pRet);

	return 0;
}
