#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include "sparse_format.h"

#define BLOCK_SIZE 4096
#define MB 1024 * 1024


char current_dir[150];
char name[10],prefix[10],line[100];
int count, total_chunk;
unsigned long int all_size;
sparse_header_t out_file_header;

int compar(const struct dirent **a, const struct dirent **b){
    char name1[10],name2[10];
    int count1, count2;

    sscanf((*a)->d_name, "%[^_]_%d", name1, &count1);
    sscanf((*b)->d_name, "%[^_]_%d", name2, &count2);

    return count2 - count1;
}

int filter(const struct dirent *a) {
    return strcmp(a->d_name, ".") && strcmp(a->d_name, "..") &&
        !strncmp(a->d_name, prefix, strlen(prefix));
}

void writeFile(FILE* out_fd, FILE* in_fd, FILE* ini_fd,long int size) {
    long int length, total_size = 0;
    int chunk = 0, ret;
    chunk_header_t chunk_header_f;

    // printf("this file size is %ld\n", size);

    while (total_size <= size) {
        if(fgets(line, sizeof(line), ini_fd) == NULL) {
            //printf("reach the last line, break!");
            break;
        }
        // printf("writeFile -- > line is %s", line);

        ret = sscanf(line, "(%hx,%hx,%x,%x)",
            &chunk_header_f.chunk_type,&chunk_header_f.reserved1,&chunk_header_f.chunk_sz,
            &chunk_header_f.total_sz);
        if (ret != 4) {
            // printf("error read ini file !!!\n");
            return;
        }

        if (chunk_header_f.chunk_type == CHUNK_TYPE_RAW) {
            fwrite(&chunk_header_f,sizeof(struct chunk_header),1,out_fd);
            length = chunk_header_f.chunk_sz * BLOCK_SIZE;
            // printf("length = %ld\n", length);
            while (length != 0) {
                if (length >= MB) {
                    chunk = MB;
                } else {
                    chunk = length;
                }
                // printf("chunk = %d\n", chunk);
                char *buff = malloc(chunk);
                fread(buff,1,chunk,in_fd);
                fwrite(buff,1,chunk,out_fd);
                length -= chunk;
                total_size += chunk;
                free(buff);
            }
            total_chunk++;
        } else if (chunk_header_f.chunk_type == CHUNK_TYPE_DONT_CARE) {
            fwrite(&chunk_header_f,sizeof(struct chunk_header),1,out_fd);
            total_chunk++;
            if (chunk_header_f.chunk_sz <= 2) {
                chunk = chunk_header_f.chunk_sz * BLOCK_SIZE;
                char *buff = malloc(chunk);
                fread(buff,1,chunk ,in_fd);
                // fwrite(buff,1,chunk_size,out_fd);
                total_size += chunk;
                // printf("chunk type don't care but chunk_size <= 2,\n");
                free(buff);
            } else {
                // printf("read the file end,break!\n");
                break;
            }
        } else {
            // printf("do nothing ...\n");
        }
        // printf("item_zie = %ld\n\n", total_size);
    };
    all_size += total_size;
    // printf("current_zie = %ld\n\n", total_size);
}

void main(int argc, const char *argv[]) {

    char full_name[160], ini_file[160];
    struct stat statbuff;
    struct dirent **namelist;
    FILE *fd, *list_fd, *ini_fd;
    int n,m,ret;
    unsigned long total_size;

    getcwd(current_dir,sizeof(current_dir));
    if (argc <= 1) {
        return;
    }

    sprintf(name,"%s", argv[1]);
    char *ext = strrchr(name,'.');
    if (ext) {
        *ext='\0';
        ext++;
    }

    if((access(argv[1], F_OK))!=-1) {
        return;
    }

    sprintf(prefix,"%s_", name);
    sprintf(ini_file, "%s/%s.ini", current_dir, name);

    if ((ini_fd = fopen(ini_file, "r"))  == NULL) {
        // printf("%s.ini is not exist !", name);
        return;
    }
    fgets(line, sizeof(line), ini_fd);
    // printf("header line is %s\n", line);

    printf("Generating %s ...\n", argv[1]);

    n = scandir(current_dir, &namelist, filter, compar);
    m = n;

    sprintf(full_name,"%s/%s.img", current_dir, name);
    fd = fopen(full_name, "wb");

    ret = sscanf(line, "(%x,%hx,%hx,%hx,%hx,%x,%x,%x,%x)",
        &out_file_header.magic,&out_file_header.major_version,&out_file_header.minor_version,
        &out_file_header.file_hdr_sz,&out_file_header.chunk_hdr_sz,&out_file_header.blk_sz,
        &out_file_header.total_blks,&out_file_header.total_chunks,&out_file_header.image_checksum);
    if (ret != 9) {
        // printf("format error for header! ret = %d\n",ret);
        return;
    }

    if (n < 0) {
        perror("not found\n");
    } else {
        while(n--) {
            // printf("%s\n", namelist[n]->d_name);
            sprintf(full_name,"%s/%s", current_dir, namelist[n]->d_name);
            stat(full_name, &statbuff);
            total_size += statbuff.st_size;
        }
        // printf("total_size is 0x%lx\n", total_size);
        fwrite(&out_file_header, sizeof(struct sparse_header), 1, fd);

        while(m--) {
            // printf("%s\n", namelist[m]->d_name);
            sprintf(full_name,"%s/%s", current_dir, namelist[m]->d_name);
            stat(full_name, &statbuff);
            // printf("full_name is %s, size is %ld\n", full_name, statbuff.st_size);
            list_fd = fopen(full_name, "rb");
            writeFile(fd,list_fd,ini_fd,statbuff.st_size);
            fclose(list_fd);
            free(namelist[m]);
            // if (m < 41) { break; }
        }
        // printf("all_zie = %ld\n\n", all_size);
        free(namelist);
    }
    fclose(fd);
    fclose(ini_fd);

    // printf("total_chunk is %d\n",total_chunk);

    printf("Done\n");
}
