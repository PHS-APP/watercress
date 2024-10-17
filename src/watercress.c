// c99
#define _POSIX_VERSION 200112L
#include <stdlib.h>
#include "./utils.h"
#include "./parsing.h"
static char* HelpText[] = {
    "Parser Debug:",
    "  watercress --test-parsing=(ptokens|stokens) ?--dbg=(escres) [file]",
    "    outputs the tokens generated by the parser",
    "    (ptokens) - outputs the parser tokens generated on first pass",
    "    (stokens) - outputs the semantic tokens generated by the parser",
    "    --dbg=",
    "      extra debugging for certain parts of the parser",
    "      (escres) - debug escape code resolution"
};
int main(int argc, char** argv) {
    if (argc > 1) {
        if (!strcmp(argv[1], "--help")) {
            int i = 0;
            while (HelpText[i]) {
                printf("%s\n", HelpText[i++]);
            }
            return EXIT_SUCCESS;
        }
    }
    if (argc > 2) {
        if (!strcmp(argv[1], "--test-parsing=ptokens")) {
            if (argc > 3) {
                for (int i = 2; i < argc; i ++) {
                    if (!strcmp(argv[i], "--dbg=escres")) {
                        DEBUG_ESCAPES = 1;
                    }
                }
            }
            parser_test_ptokens(argv[argc-1]);
        }
    }
    return EXIT_SUCCESS;
}
