#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int mycmp(const void *p1, const void *p2)
{
	return (*(const int *)p1 > *(const int *)p2);
}

void sortArray(int * array, int size)
{
	qsort(array, size, sizeof(int), mycmp);
}

/**
 * ID 1
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* twoSum(int* nums, int numsSize, int target) {
    int *s = &nums[0];
    int *e = &nums[numsSize - 1];
    int sum = 0;
    int *ret_array = NULL;
    
    ret_array = (int *)malloc(sizeof(int) * 2);

    while (s < e) {
        sum = *s + *e;
        if (sum > target)
            e = e -1;
        else if (sum < target)
            s = s+1;
        else 
            break;
    }
    
    ret_array[0] = s-&nums[0];
    ret_array[1] = e-&nums[0];
    
    return ret_array;   
    
}

double findMedianSortedArrays(int* nums1, int nums1Size, int* nums2, int nums2Size) {
     int * pTemp = NULL;
    int i = 0, j = 0;
    int TotalCnt = (nums1Size+nums2Size);
    pTemp = (int *)malloc(TotalCnt * 4);
    int iTmp = 0;
    double result = 0.0;

    for (iTmp = 0; iTmp < TotalCnt; iTmp++) {
        if (i == nums1Size)
            pTemp[iTmp] = nums2[j++];
        else if (j == nums2Size)
             pTemp[iTmp] = nums1[i++];
        else {
            if (nums1[i] <= nums2[j])
                pTemp[iTmp] = nums1[i++];
            else
                pTemp[iTmp] = nums2[j++];
        }
    }
	
	printf("pTemp:\n");
	for (i = 0; i < TotalCnt; i++) {
		printf("%d ", pTemp[i]);
	}
	printf("\n");

    if ((TotalCnt %2) == 0) {
        result = (((double)pTemp[(TotalCnt -1)/2] + (double)pTemp[(TotalCnt-1)/2+1])/2.0);
    }
    else
        result = pTemp[(TotalCnt-1)/2];

    free(pTemp);
    return result;
}

int main(int argc, char **argv) {
#if 0
	int a[8] = {8,7,6,5,4,3,2,1};
	int i = 0;

	sortArray(a, 8);
	printf("a:\n");
	for (i = 0; i < 8; i++)
		printf("%d ", a[i]);
#else
	int a[] = {3};
	int b[] = {-2, -1};
	double ret = 0;

	ret = findMedianSortedArrays(a, 1, b ,2);
	printf("ret: %f\n", ret);
	
#endif
	return 0;
}
