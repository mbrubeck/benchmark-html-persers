//
//  main.c
//  benchmark
//
//  Created by Alexander Borisov on 06.03.16.
//  Copyright © 2016 Alexander Borisov. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>

#include "perf.h"
#include "proc_stat.h"
#include "work.h"

#include "myhtml/api.h"
// #include "gumbo.h"
#include "html2html_lib.h"

void benchmark_myhtml_single(const char *filename, const char *html, size_t size, struct benchmark_ctx *ctx)
{
    myhtml_t* myhtml = myhtml_create();
    myhtml_init(myhtml, MyHTML_OPTIONS_PARSE_MODE_SINGLE, 1, 0);
    
    // first tree init
    myhtml_tree_t* tree = myhtml_tree_create();
    myhtml_tree_init(tree, myhtml);
    
    myhtml_parse(tree, MyHTML_ENCODING_UTF_8, html, size);
    
    myhtml_tree_destroy(tree);
    myhtml_destroy(myhtml);
}

void benchmark_myhtml(const char *filename, const char *html, size_t size, struct benchmark_ctx *ctx)
{
    myhtml_t* myhtml = myhtml_create();
    myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);
    
    // first tree init
    myhtml_tree_t* tree = myhtml_tree_create();
    myhtml_tree_init(tree, myhtml);
    
    myhtml_parse(tree, MyHTML_ENCODING_UTF_8, html, size);
    
    myhtml_tree_destroy(tree);
    myhtml_destroy(myhtml);
}

void benchmark_myhtml_single_real_live(const char *filename, const char *html, size_t size, struct benchmark_ctx *ctx)
{
    if(ctx->data == NULL)
    {
        myhtml_t* myhtml = myhtml_create();
        myhtml_init(myhtml, MyHTML_OPTIONS_PARSE_MODE_SINGLE, 1, 0);
        
        // first tree init
        myhtml_tree_t* tree = myhtml_tree_create();
        myhtml_tree_init(tree, myhtml);
        
        ctx->data = tree;
    }
    
    myhtml_parse(ctx->data, MyHTML_ENCODING_UTF_8, html, size);
}

void benchmark_myhtml_real_live(const char *filename, const char *html, size_t size, struct benchmark_ctx *ctx)
{
    if(ctx->data == NULL)
    {
        myhtml_t* myhtml = myhtml_create();
        myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);
        
        // first tree init
        myhtml_tree_t* tree = myhtml_tree_create();
        myhtml_tree_init(tree, myhtml);
        
        ctx->data = tree;
    }
    
    myhtml_parse(ctx->data, MyHTML_ENCODING_UTF_8, html, size);
}

/*
void benchmark_gumbo(const char *filename, const char *html, size_t size, struct benchmark_ctx *ctx)
{
    GumboOutput* output = gumbo_parse_with_options(&kGumboDefaultOptions, html, size);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}
*/

void benchmark_html5ever(const char *filename, const char *html, size_t size, struct benchmark_ctx *ctx)
{
    parse_document(html, size, 0);
}

void bentchmark_fork(const char *dirpath, const char *filename, benchmark_work_callback_f callback)
{
    FILE *fh = fopen(filename, "w");
    if (fh == NULL) {
        printf("Error opening file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    benchmark_work_readdir_fork(dirpath, callback, fh);
    
    fclose(fh);
}

void bentchmark(const char *dirpath, const char *filename, benchmark_work_callback_f callback)
{
    FILE *fh = fopen(filename, "w");
    if (fh == NULL) {
        printf("Error opening file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    struct benchmark_ctx ctx = {0, 0, NULL, 0};
    benchmark_work_readdir(dirpath, &ctx, callback, fh);
    
    fclose(fh);
}

int main(int argc, char *argv[])
{
    const char *dirpath = "list/";
    
    bentchmark(dirpath, "result/myhtml_single_object.csv", benchmark_myhtml_real_live);
    bentchmark(dirpath, "result/myhtml_single_single_object.csv", benchmark_myhtml_single_real_live);
    
    bentchmark_fork(dirpath, "result/myhtml_single.csv", benchmark_myhtml_single);
    bentchmark_fork(dirpath, "result/myhtml.csv", benchmark_myhtml);
    bentchmark_fork(dirpath, "result/html5ever.csv", benchmark_html5ever);
    // bentchmark_fork(dirpath, "/new/html_parsers/result/gumbo.csv", benchmark_gumbo);
    
    return 0;
}

//int main(int argc, const char * argv[]) {
//    // insert code here...
//    
//    uint64_t start = myhtml_hperf_clock();
//    usleep(1000000);
//    uint64_t stop = myhtml_hperf_clock();
//    
//    myhtml_hperf_print("Test", start, stop, stdout);
//
//    return 0;
//}
