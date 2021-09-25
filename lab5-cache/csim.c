#include"cachelab.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define ADDRESS_SIZE 32

int s,E,b,S,t;

struct node{
    int key;
    struct node* prev;
    struct node* next;
};

typedef struct queue{
    struct node* head;
    struct node* tail;
}my_queue;

void add_node_to_head(my_queue* queue,struct node* n){
    queue->head->next->prev = n;
    n->next = queue->head->next;
    queue->head->next = n;
    n->prev = queue->head; 
}

int update_node_and_set_head(my_queue* queue, int key){
    struct node* h = queue->head;
    while(h->next!=queue->tail){
        if(h->next->key == key){
            struct node* tmp = h->next;
            tmp->next->prev = h;
            h->next = tmp->next;
            tmp->next=NULL;
            tmp->prev=NULL;
            add_node_to_head(queue, tmp);
            return 1;
        }
        h = h->next;
    }
    return  0;
}

int delete_node(my_queue* queue, int key){
    struct node* h = queue->head;
    while(h->next!=queue->tail){
        if(h->next->key == key){
            struct node* tmp = h->next;
            tmp->next->prev = h;
            h->next = tmp->next;
            free(tmp);
            return 1;
        }
        h = h->next;
    }
    return  0;
}

void delete_bottom_node(my_queue* queue){
    if(queue->tail->prev == queue->head){
        return ;
    }
    struct node* tmp = queue->tail->prev;
    tmp->prev->next = queue->tail;
    queue->tail->prev = tmp->prev;
    free(tmp);
    return ;
}

typedef struct cache_line{
    int valid;
    int tag;
    int val;
}line;

typedef struct cache{
    line** content;
    int E;
    int S;
    my_queue* queues;
    int hits;
    int misses;
    int evictions;
}my_cache;

my_cache construct_cache(){
    my_cache cache;
    cache.E = E;
    cache.S = S;
    cache.queues=(my_queue*)malloc(S*sizeof(my_queue));
    for(int i=0;i<S;i++){
        cache.queues[i].head = (struct node*)malloc(sizeof(struct node));
        cache.queues[i].tail = (struct node*)malloc(sizeof(struct node));
        cache.queues[i].head->next = cache.queues[i].tail;
        cache.queues[i].head->prev = NULL;
        cache.queues[i].head->key = -1;
        cache.queues[i].tail->prev = cache.queues[i].head;
        cache.queues[i].tail->next = NULL;
        cache.queues[i].tail->key = -1;
    }
    cache.content = (line**)malloc(S*sizeof(line*));
    for(int i=0;i<S;i++){
        cache.content[i] = (line*)malloc(E*sizeof(line));
    }
    for(int i=0;i<S;i++){
        for(int p=0;p<E;p++){
            cache.content[i][p].tag=-2;
        }
    }
    cache.hits=0;
    cache.misses=0;
    cache.evictions=0;
    return cache;    
}

int get_offset(int addr){
    return addr<<(t+s)>>(t+s);
}

int get_index(int addr){
    return ((unsigned)(addr<<t))>>(t+b);
}

int get_tag(int addr){
    return addr>>(b+s);
}

void event_store_data(my_cache* cache, int addr, int val){
    int offset = get_index(addr);
    if(offset<0||offset>=cache->S){
        cache->misses++;
        printf("ev miss\n");
        return ;
    }
    for(int i=0;i<cache->E;i++){
        if(cache->content[offset][i].tag == get_tag(addr)){
            cache->hits++;
            update_node_and_set_head(&cache->queues[offset], i);
            printf("ev hits\n");
            return ;
        }
    } 
    for(int i=0;i<cache->E;i++){
            if(cache->content[offset][i].tag == -2){
            cache->content[offset][i].valid = 1;
            cache->content[offset][i].tag = get_tag(addr);
            cache->content[offset][i].val = val;
            cache->misses++;
            printf("ev miss\n");
            struct node* n=(struct node*)malloc(sizeof(struct node));
            n->key = i;
            add_node_to_head(&cache->queues[offset], n);
            return ;
            }
    }
    int target = cache->queues[offset].tail->prev->key;
    cache->content[offset][target].tag = get_tag(addr);
    cache->content[offset][target].val = val;
    cache->misses++;
    cache->evictions++;
    printf("ev miss evict\n");
    delete_bottom_node(&cache->queues[offset]);
    struct node* n=(struct node*)malloc(sizeof(struct node));
    n->key = target;
    add_node_to_head(&cache->queues[offset], n);
    return ;
}

void event_load_data(my_cache* cache, int addr, int val){
    int offset = get_index(addr);
    if(offset<0||offset>=cache->S){
        cache->misses++;
        printf("ev miss\n");
        return ;
    }
    int tag = get_tag(addr);
    printf("%d\n",tag);
    int index = get_offset(addr);
    for(int i=0;i<cache->E;i++){
        if(cache->content[offset][i].tag == tag){
            cache->hits++;
            printf("ev hit\n");
            update_node_and_set_head(&cache->queues[offset], i);
            return ;
        }
    }
    for(int i=0;i<cache->E;i++){
        if(cache->content[offset][i].tag == -2){
            cache->content[offset][i].valid = 1;
            cache->content[offset][i].tag = get_tag(addr);
            cache->content[offset][i].val = val;
            cache->misses++;
            printf("ev miss\n");
            struct node* n=(struct node*)malloc(sizeof(struct node));
            n->key = i;
            add_node_to_head(&cache->queues[offset], n);
            return ;
        }
    } 
    int target = cache->queues[offset].tail->prev->key;
    cache->content[offset][target].tag = get_tag(addr);
    cache->content[offset][target].val = val;
    cache->misses++;
    cache->evictions++;
    printf("ev miss evict\n");
    delete_bottom_node(&cache->queues[offset]);
    struct node* n=(struct node*)malloc(sizeof(struct node));
    n->key = target;
    add_node_to_head(&cache->queues[offset], n);
}

void event_modify_data(my_cache* cache, int addr, int val){
     int offset = get_index(addr);
    if(offset<0||offset>=cache->S){
        cache->misses++;
        printf("ev miss\n");
        return ;
    }
    int tag = get_tag(addr);
    for(int i=0;i<cache->E;i++){
        if(cache->content[offset][i].tag == tag){
            cache->hits++;
            cache->hits++;
            printf("ev hit\n");
            printf("ev hit\n");
            update_node_and_set_head(&cache->queues[offset], i);
            cache->content[offset][i].val = val;
            return ;
        }
    }
    cache->misses++;
    printf("ev miss\n");
    for(int i=0;i<cache->E;i++){
        if(cache->content[offset][i].tag == -2){
            cache->content[offset][i].valid = 1;
            cache->content[offset][i].tag = get_tag(addr);
            cache->content[offset][i].val = val;
            cache->hits++;
            printf("ev hit\n");
            struct node* n=(struct node*)malloc(sizeof(struct node));
            n->key = i;
            add_node_to_head(&cache->queues[offset], n);
            return ;
        }
    } 
    int target = cache->queues[offset].tail->prev->key;
    cache->content[offset][target].tag = get_tag(addr);
    cache->content[offset][target].val = val;
    cache->evictions++;
    cache->hits++;
    printf("ev hits evict\n");
    delete_bottom_node(&cache->queues[offset]);
    struct node* n=(struct node*)malloc(sizeof(struct node));
    n->key = target;
    add_node_to_head(&cache->queues[offset], n);
}

int depend_opt_type(char* opt){
    if(!strcmp(opt,"-s")){
        return 1;
    }else if(!strcmp(opt,"-E")){
        return 2;
    }else if(!strcmp(opt,"-b")){
        return 3;
    }else if(!strcmp(opt,"-t")){
        return 4;
    }else{
        return -1;
    }
}

int get_S(){
    return 1<<s;
}

void parse_trace(FILE* fp)
{
        printf("ok");
        if(fp == NULL)
        {
                printf("open error");
                exit(-1);
        }
        my_cache cache = construct_cache();
        char operation;         // 命令开头的 I L M S
        int address;   // 地址参数
        int size;               // 大小
        printf("ok");
        while(fscanf(fp, " %c %xu,%d\n", &operation, &address, &size) > 0)
        {
                
                switch(operation)
                {
                        //case 'I': continue;           // 不用写关于 I 的判断也可以
                        case 'L':
                                event_load_data(&cache, address, size);
                                break;
                        case 'M':
                                event_modify_data(&cache, address, size);  // miss的话还要进行一次storage
                                break;
                        case 'S':
                                event_store_data(&cache, address, size);
                                break;
                }
        }
        printf("hits:%d misses:%d evictions:%d\n", cache.hits, cache.misses, cache.evictions);
        return;
}

int main(int argc, char** argv)
{
    FILE *path = NULL;
    for(int i=1;i<argc;i++){
        char* opt = argv[i];
        int c = depend_opt_type(opt);
        switch(c){
            case 1:
                s = *argv[i+1]-'0';
                break;
            case 2:
                E = *argv[i+1]-'0';
                break;
            case 3:
                b = *argv[i+1]-'0';
                break;
            case 4:
                path = fopen(argv[i+1],"r");
                break;
            default:
                continue;
        }
    }
    t = ADDRESS_SIZE-s-b;
    S = get_S();
    my_cache cache = construct_cache();
    printf("%d %d %d %d\n",s,E,b,S);
    char operation;         // 命令开头的 I L M S
    int address;   // 地址参数
    int size;               // 大小
    printf("ok\n");
    while(fscanf(path, " %c %x,%d\n", &operation, &address, &size) > 0)
    {
                printf("%c %x %d\n", operation, address , size);
                switch(operation)
                {
                        //case 'I': continue;           // 不用写关于 I 的判断也可以
                        case 'L':
                                event_load_data(&cache, address, size);
                                break;
                        case 'M':
                                event_modify_data(&cache, address, size);  // miss的话还要进行一次storage
                                break;
                        case 'S':
                                event_store_data(&cache, address, size);
                                break;
                }
    }
    printf("hits:%d misses:%d evictions:%d\n", cache.hits, cache.misses, cache.evictions);
    return 0;
}