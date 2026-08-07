static int func(int i, int j)
{
	printf("i = %d and func called with j = %d\n", i, j);
	return j;
}

main()
{
int i,j;

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			if (i && func(i, j)) {
				printf("%d && %d evaluates to TRUE\n", i, j);
			} else {
				printf("%d && %d evaluates to FALSE\n", i, j);
			}
		}
	}
}
