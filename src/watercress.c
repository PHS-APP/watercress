// c99 with 3 TC, POSIX 2001 with 2 TC
#define _POSIX_VERSION 200112L
#include <stdlib.h>
#include "./utils.h"
#include "./parsing.h"
#include "./intermediate.h"
static char* HelpText[] = {
    "General Debug:",
    "  debugging for all parts of the compiler",
    "  watercress --dsdbg-- [ignored]",
    "    runs test suite on the data structure debug tools",
    "  watercress [other options/commands/arguments] ?--dsdbg=(ll|dl|hm) [file]",
    "    --dsdbg=",
    "      extra debugging for data structures",
    "      (ll) - enables tracing for linked list operations",
    "      (dl) - enables tracing for dynamic list operations",
    "      (hm) - enables tracing for hash map operations",
    "Parser Debug:",
    "  debugging specifically for the parser",
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
        if (!strcmp(argv[1], "--dsdbg--")) {
            datastruct_debug_enable_base(DATASTRUCTDBG_LINKEDLIST);
            datastruct_debug_enable_base(DATASTRUCTDBG_DYNLIST);
            datastruct_debug_enable_base(DATASTRUCTDBG_HASHMAP);
            printf("\nRUNNING LINKED LIST TEST\n");
            char** tis = malloc(sizeof(char*)*5);
            char* msgs[] = {"FAIL", "PASS"};
            tis[0] = strmove("T1");
            tis[1] = strmove("T2");
            tis[2] = strmove("T3");
            tis[3] = strmove("T4");
            tis[4] = strmove("T5");
            LinkedList* llt = linkedlist_create(pointereq);
            linkedlist_push(llt, tis[0]);
            linkedlist_push(llt, tis[1]);
            linkedlist_insert_at(llt, 0, tis[2]);
            LinkedListNode* lln = linkedlist_get(llt, 0);
            linkedlist_insert_prev(llt, lln, tis[3]);
            linkedlist_insert_next(llt, lln, tis[4]);
            // for (int i = 0; i < llt->size; i ++) {
            //     void* d = linkedlist_get(llt, i)->data;
            //     printf("(%p=%s) ", d, (char*)d);
            // }
            // printf("\n");
            char* v1 = linkedlist_remove_prev(llt, lln);
            char* v2 = linkedlist_remove_next(llt, lln);
            char* v3 = linkedlist_remove_at(llt, 0);
            char* v4 = linkedlist_pop(llt);
            char* v5 = linkedlist_get(llt, 0)->data;
            printf("\n");
            printf("CHECK 1: %s=%s (%s)\n", v1, tis[3], msgs[!strcmp(tis[3], v1)]);
            printf("CHECK 2: %s=%s (%s)\n", v2, tis[4], msgs[!strcmp(tis[4], v2)]);
            printf("CHECK 3: %s=%s (%s)\n", v3, tis[2], msgs[!strcmp(tis[2], v3)]);
            printf("CHECK 4: %s=%s (%s)\n", v4, tis[1], msgs[!strcmp(tis[1], v4)]);
            printf("CHECK 5: %s=%s (%s)\n", v5, tis[0], msgs[!strcmp(tis[0], v5)]);
            linkedlist_destroy(llt);
            printf("LINKED LIST TEST FINISHED\n");
            printf("\nRUNNING DYNAMIC LIST TEST\n");
            tis[0] = strmove("T1");
            tis[1] = strmove("T2");
            tis[2] = strmove("T3");
            tis[3] = strmove("T4");
            tis[4] = strmove("T5");
            DynList* dlt = dynlist_create(pointereq, no_release);
            dynlist_push(dlt, tis[0]);
            dynlist_push(dlt, tis[1]);
            dynlist_insert(dlt, 0, tis[2]);
            dynlist_insert(dlt, 3, tis[3]);
            dynlist_insert(dlt, 2, tis[4]);
            v1 = dynlist_remove(dlt, 0);
            v2 = dynlist_remove(dlt, 3);
            v3 = dynlist_remove(dlt, 1);
            v4 = dynlist_pop(dlt);
            v5 = dynlist_get(dlt, 0);
            printf("\n");
            printf("CHECK 1: %s=%s (%s)\n", v1, tis[2], msgs[!strcmp(tis[2], v1)]);
            printf("CHECK 2: %s=%s (%s)\n", v2, tis[3], msgs[!strcmp(tis[3], v2)]);
            printf("CHECK 3: %s=%s (%s)\n", v3, tis[4], msgs[!strcmp(tis[4], v3)]);
            printf("CHECK 4: %s=%s (%s)\n", v4, tis[1], msgs[!strcmp(tis[1], v4)]);
            printf("CHECK 5: %s=%s (%s)\n", v5, tis[0], msgs[!strcmp(tis[0], v5)]);
            dynlist_destroy(dlt);
            printf("DYNAMIC LIST TEST FINISHED\n");
            printf("\nRUNNING HASH MAP TEST\n");
            free(tis[4]);
            tis[4] = strmove("T13");
            HashMap* hmt = hashmap_create(hashstr, hashstr2, pointereq);
            hmt->rel = no_release;
            hashmap_set(hmt, tis[0], tis[3]);
            hashmap_set(hmt, tis[1], tis[4]);
            hashmap_set(hmt, tis[2], tis[2]);
            hashmap_set(hmt, tis[3], tis[1]);
            hashmap_set(hmt, "T5", tis[0]);
            hashmap_set(hmt, tis[0], tis[4]);
            hashmap_set(hmt, tis[1], tis[3]);
            hashmap_set(hmt, "T6", "");
            hashmap_set(hmt, "T7", "");
            hashmap_set(hmt, "T8", "");
            hashmap_set(hmt, "T9", "");
            hashmap_set(hmt, "T10", "");
            hashmap_set(hmt, "T11", "");
            hashmap_set(hmt, "T12", "");
            hashmap_set(hmt, tis[4], tis[0]);
            hashmap_set(hmt, "T14", "");
            hashmap_set(hmt, "T15", "");
            hashmap_set(hmt, "T16", "");
            hashmap_set(hmt, "T17", "");
            hashmap_set(hmt, "T18", "");
            hashmap_set(hmt, "T19", "");
            v1 = hashmap_get(hmt, tis[0]);
            v2 = hashmap_get(hmt, tis[1]);
            v3 = hashmap_get(hmt, tis[2]);
            v4 = hashmap_get(hmt, tis[3]);
            v5 = hashmap_get(hmt, tis[4]);
            if (v5 == NULL) v5 = "(null)";
            printf("\n");
            printf("CHECK 1: %s=%s (%s)\n", v1, tis[4], msgs[!strcmp(tis[4], v1)]);
            printf("CHECK 2: %s=%s (%s)\n", v2, tis[3], msgs[!strcmp(tis[3], v2)]);
            printf("CHECK 3: %s=%s (%s)\n", v3, tis[2], msgs[!strcmp(tis[2], v3)]);
            printf("CHECK 4: %s=%s (%s)\n", v4, tis[1], msgs[!strcmp(tis[1], v4)]);
            printf("CHECK 5: %s=%s (%s)\n", v5, tis[0], msgs[!strcmp(tis[0], v5)]);
            hashmap_destroy(hmt);
            printf("HASH MAP TEST FINISHED\n");
            for (int i = 0; i < 5; i ++) free(tis[i]);
            free(tis);
            return EXIT_SUCCESS;
        }
    }
    if (argc > 3) {
        for (int i = 2; i < argc; i ++) {
            if (!strcmp(argv[i], "--dsdbg=ll")) {
                datastruct_debug_enable_base(DATASTRUCTDBG_LINKEDLIST);
            } else if (!strcmp(argv[i], "--dsdbg=dl")) {
                datastruct_debug_enable_base(DATASTRUCTDBG_DYNLIST);
            } else if (!strcmp(argv[i], "--dsdbg=hm")) {
                datastruct_debug_enable_base(DATASTRUCTDBG_HASHMAP);
            }
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
        if (!strcmp(argv[1], "--test-parsing=stokens")) {
            parser_test_stokens(argv[argc-1]);
        }
        if (!strcmp(argv[1], "--test-intermediate")) {
            LinkedList* tofollow = LINKED_OBJECTS;
            Token* output = parse_file(argv[argc-1], 0, tofollow);
            linkedlist_destroy(tofollow);

            Program *p = compile(output);
            print_program(p);
        }
    }
    
    /* HashMap *map = hashmap_create(&hashstr, &hashstr2, &streq); */
    
    /* hashmap_set(map, (void *)strmove("hello"), (void *)strmove("world")); */
    /* hashmap_set(map, (void *)strmove("hello2"), (void *)strmove("world2")); */
    /* hashmap_set(map, (void *)strmove("ahoy"), (void *)strmove("earth")); */
    /* hashmap_set(map, (void *)strmove("what's up"), (void *)strmove("planet")); */

    /* printf("%s\n", (char *)hashmap_get(map, (void *)strmove("hello"))); */
    /* printf("%s\n", (char *)hashmap_get(map, (void *)strmove("ahoy"))); */

    /* hashmap_set(map, (void *)strmove("jkjkdsf"), (void *)strmove("wieurqiure")); */
    /* hashmap_set(map, (void *)strmove("uiweuri"), (void *)strmove("cvxzm,cv")); */
    /* hashmap_set(map, (void *)strmove("nmncxvmn"), (void *)strmove("uiwerfds")); */
    /* hashmap_set(map, (void *)strmove("izuifx"), (void *)strmove("nrmenwqrm")); */
    /* hashmap_set(map, (void *)strmove("uixcuviz"), (void *)strmove("uijviox")); */
    /* hashmap_set(map, (void *)strmove("jkfdjsio"), (void *)strmove("jksjdkfweqer")); */

    /* printf("%s\n", (char *)hashmap_get(map, (void *)strmove("hello"))); */
    /* printf("%s\n", (char *)hashmap_get(map, (void *)strmove("ahoy"))); */

    /* hashmap_set(map, (void *)strmove("hello"), (void *)strmove("schartman")); */
    /* printf("%s\n", (char *)hashmap_get(map, (void *)strmove("hello"))); */

    /* hashmap_set(map, (void *)5, (void *)7); */
    /* hashmap_set(map, (void *)10, (void *)8); */
    /* hashmap_set(map, (void *)22, (void *)9); */

    /* printf("%ld\n", (long)(hashmap_get(map, (void *)5))); */
    /* printf("%ld\n", (long)(hashmap_get(map, (void *)10))); */
    /* printf("%ld\n", (long)(hashmap_get(map, (void *)22))); */
    
    /* hashmap_set(map, (void *)22, (void *)10); */
    /* printf("%ld\n", (long)(hashmap_get(map, (void *)22))); */
    
    /* hashmap_set(map, (void *)1, (void *)8); */
    /* hashmap_set(map, (void *)2, (void *)8); */
    /* hashmap_set(map, (void *)3, (void *)10); */
    /* hashmap_set(map, (void *)4, (void *)8); */
    /* hashmap_set(map, (void *)6, (void *)8); */
    /* hashmap_set(map, (void *)7, (void *)10); */

    /* printf("%ld\n", (long)(hashmap_get(map, (void *)5))); */
    /* printf("%ld\n", (long)(hashmap_get(map, (void *)10))); */
    /* printf("%ld\n", (long)(hashmap_get(map, (void *)22))); */
    
    /* hashmap_destroy(map); */

    return EXIT_SUCCESS;
}
