#include<stdio.h>
int main(){
FILE *fp;
char *str="Hello\n";
/*char *usage="./bkpctl -[ld:v:r:] FILE\n
FILE: the file's name to operate on\n
-l: option to \"list versions\"\n
-d ARG: option to \"delete\" versions; ARG can be \"newest\", \"oldest\", or \"all\"\n
-v ARG: option to \"view\" contents of versions (ARG: \"newest\", \"oldest\", or N)\n
-r ARG: option to \"restore\" file (ARG: \"newest\" or N)\n
	(where N is a number such as 1, 2, 3, ...)\n";
*/
//print("usage %s", usage);
fp=fopen("/mnt/bkpfs/file.txt","w");
fwrite(str , 1 , sizeof(str) , fp );
fclose(fp);
return 0;
}
