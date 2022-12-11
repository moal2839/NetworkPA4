server: dfs.c dfc.c
	gcc -Wextra -Wall -pedantic -g dfs.c md5.c -o dfs;
	gcc -Wextra -Wall -pedantic -g dfc.c md5.c -o dfc;

test: test.c md5.c
	gcc -Wextra -Wall -pedantic -g test.c md5.c -o test

clean:
	rm *.o