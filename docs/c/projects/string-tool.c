/*
Your goal is to create a command-line utility that accepts a string and a "command" flag, then outputs the transformed result.
The program should support the following flags:
-u: Uppercase (Convert all characters to uppercase).
-l: Lowercase (Convert all characters to lowercase).
-r: Reverse (Flip the string backwards).
-s: Stats (Print the length of the string and the count of vowels).
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
int main(int argc, char *argv[])
{
    char *errMsg = "Usage: [flag] [string]\n"
                   "-u: Uppercase\n"
                   "-l: Lowercase\n"
                   "-r: Reverse\n"
                   "-s: Stats\n";
    if (argc < 3)
    {
        printf("%s", errMsg);
        return 1;
    }
    char *flag = argv[1];
    char *input = argv[2];

    // uppercase:
    if (strcmp("-u", flag) == 0)
    {
        for (int i = 0; input[i] != '\0'; i++)
        {
            input[i] = toupper(input[i]);
        }
        printf("%s", input);
    }
    else if (strcmp("-l", flag) == 0)
    {
        for (int i = 0; input[i] != '\0'; i++)
        {
            input[i] = tolower(input[i]);
        }
        printf("%s", input);
    }
    else if (strcmp("-r", flag) == 0)
    {
        int len = strlen(input);
        for (int i = len - 1; i >= 0; i--) {
            putchar(input[i]);
        }
        printf("\n");
    }
    else if (strcmp(flag, "-s") == 0) {
        int len = strlen(input);
        int vowels = 0;
        for (int i = 0; i < len; i++) {
            char c = tolower(input[i]);
            if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
                vowels++;
            }
        }
        printf("Length: %d\nVowels: %d\n", len, vowels);
    }
    else
    {
        printf("Unknown flag: %s\n%s", flag, errMsg);
    }
}
