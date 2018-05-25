#include<stdlib.h>

class TTL_Cache
{
public:
    TTL_Cache(int, int, int, int, int, int);
    void request_handle(int);
    int check_cache(int);
    void renew_cache(int);
    void expired();
    void view_dead();
    int get_hit();
    int get_synthesis();
    int get_view_request_total();
    int get_total_synthesis();

private:
    struct Cache_node
    {
        int view_content;
        int TTL;
        struct Cache_node* next;
    };
    int hit;
    int synthesis;
    int total_synthesis;
    int view_num;
    int range_size;
    int range_full_size;
    int DIBR_range;
    int view_request_total;
    int high_quality;
    int low_quality;
    int* view_request_count;
    int* view_quality_count;
    int cache_size;
    struct Cache_node* cache_node;
};
