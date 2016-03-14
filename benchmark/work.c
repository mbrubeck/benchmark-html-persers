//
//  work.c
//  benchmark
//
//  Created by Alexander Borisov on 06.03.16.
//  Copyright © 2016 Alexander Borisov. All rights reserved.
//

#include <stdint.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include "work.h"

struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

struct benchmark_res_html {
    char  *html;
    size_t size;
};

struct benchmark_res_html benchmark_load_html_file(const char* filename)
{
    FILE *fh = fopen(filename, "rb");
    if(fh == NULL) {
        fprintf(stderr, "Can't open html file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    fseek(fh, 0L, SEEK_END);
    long size = ftell(fh);
    fseek(fh, 0L, SEEK_SET);
    
    char *html = (char*)malloc(size + 1);
    if(html == NULL) {
        DIE("Can't allocate mem for html file: %s\n", filename);
    }
    
    size_t nread = fread(html, 1, size, fh);
    if (nread != size) {
        DIE("could not read %ld bytes (%zu bytes done)\n", size, nread);
    }
    
    fclose(fh);
    
    if(size < 0) {
        size = 0;
    }
    
    struct benchmark_res_html res = {html, (size_t)size};
    return res;
}

void benchmark_work(const char *filepath, const char *filename, benchmark_work_callback_f callback, struct benchmark_ctx *ctx, FILE *out_fh)
{
    struct benchmark_res_html res = benchmark_load_html_file(filepath);
    
    struct timespec time_start, time_end;
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    
    callback(filename, res.html, res.size, ctx);
    
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    
    free(res.html);
    
    struct timespec dt = diff(time_start, time_end);
    
    ctx->count++;
    //ctx->sum += work_time;
    ctx->total_file_size += res.size;
    
    fprintf(out_fh, "\"%s\";%zu;%lld.%09lld;%lld\n", filename, res.size, dt.tv_sec, dt.tv_nsec, 0LL);
}

void benchmark_work_fork(const char *filepath, const char *filename, benchmark_work_callback_f callback, FILE *out_fh)
{
    struct benchmark_res_html res = benchmark_load_html_file(filepath);
    struct benchmark_ctx ctx = {0, 0, NULL, 0};
    
    size_t mem_start    = proc_stat_getPeakRSS();
    struct timespec time_start, time_end;
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    
    callback(filename, res.html, res.size, &ctx);
    
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    size_t mem_end    = proc_stat_getPeakRSS();
    
    free(res.html);
    
    long long mem_used = mem_end - mem_start;
    struct timespec dt = diff(time_start, time_end);
    
    fprintf(out_fh, "\"%s\";%zu;%lld.%09lld;%lld\n", filename, res.size, dt.tv_sec, dt.tv_nsec, mem_used);
}

void benchmark_work_readdir_fork(const char *dirpath, benchmark_work_callback_f callback, FILE *out_fh)
{
    DIR *dir;
    struct dirent *ent;
    struct stat path_stat;
    
    size_t dirpath_len = strlen(dirpath);
    
    char path[4096];
    strncpy(path, dirpath, dirpath_len);
    
    if((dir = opendir(dirpath)) != NULL)
    {
        while((ent = readdir(dir)) != NULL)
        {
            sprintf(&path[dirpath_len], "%s", ent->d_name);
            
            stat(path, &path_stat);
            
            if(ent->d_name[0] != '.' && !S_ISDIR(path_stat.st_mode))
            {
                pid_t cpid = fork();
                
                if(cpid == 0)
                {
                    benchmark_work_fork(path, ent->d_name, callback, out_fh);
                    fclose(out_fh);
                    
                    exit(EXIT_SUCCESS);
                }
                else {
                    while ((cpid = waitpid(-1, NULL, 0))) {
                        if (errno == ECHILD) {
                            break;
                        }
                    }
                }
            }
        }
        
        closedir (dir);
    }
}

void benchmark_work_readdir(const char *dirpath, struct benchmark_ctx *ctx, benchmark_work_callback_f callback, FILE *out_fh)
{
    setbuf(out_fh, NULL);
    
    DIR *dir;
    struct dirent *ent;
    struct stat path_stat;
    
    size_t dirpath_len = strlen(dirpath);
    
    char path[4096];
    strncpy(path, dirpath, dirpath_len);
    
    if((dir = opendir(dirpath)) != NULL)
    {
        while((ent = readdir(dir)) != NULL)
        {
            sprintf(&path[dirpath_len], "%s", ent->d_name);
            
            stat(path, &path_stat);
            
            if(ent->d_name[0] != '.' && !S_ISDIR(path_stat.st_mode)) {
                benchmark_work(path, ent->d_name, callback, ctx, out_fh);
            }
        }
        
        closedir (dir);
    }
}

void benchmark_work_print_total(struct benchmark_ctx *ctx, FILE *fh)
{
    fprintf(fh, "Time: %0.5f\n", ctx->sum);
    fprintf(fh, "Size of all files: %lu\n", ctx->total_file_size);
    
    size_t mem_cur = proc_stat_getCurrentRSS();
    
    fprintf(fh, "Start RSS size: %lu\n", ctx->start_mem);
    fprintf(fh, "Current RSS size: %lu\n", mem_cur);
}



