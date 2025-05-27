#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/markdown.h"
#include "../libs/document.h"
#include "../libs/helper.h"

int main(){

    document* doc = markdown_init();

    char* str1 = malloc(sizeof(char)*(strlen("Wolrd")+1));
    char* str2 = malloc(sizeof(char)*(strlen("Hello ")+1));
    strcpy(str1, "World");
    strcpy(str2, "Hello ");
    markdown_insert(doc, 0, 0, str1);
    markdown_insert(doc, 0, 0, str2);

    char* res1 = markdown_flatten(doc);
    printf("%s\n", res1);

    markdown_increment_version(doc);

    char* res2 = markdown_flatten(doc);
    printf("%s\n", res2);

    free(str1);
    free(str2);
    free(res1);
    free(res2);

    markdown_free(doc);

}