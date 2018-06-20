#include<stdio.h>
#include<time.h>
#include<vector>
#include "Request.h"
#include "LRU_Cache.h"
#include "TTL_Cache.h"

using namespace std;

class Huristic
{
public:
    Huristic(int view_num, int range_size, int cache_size, int DIBR_range) : output_file2(output_file2), view_num(view_num), range_size(range_size), cache_size(cache_size), DIBR_range(DIBR_range)
    {
        output_file2 = fopen("Nt_seed", "r");
        hit = 0;
        synthesis = 0;
        view_request_total = 0;
        total_synthesis = 0;
        Cache = new int[cache_size];
        for(int i=0; i<cache_size; ++i)
            Cache[i] = i;
        Nt = new int[view_num];
        for(int i=0; i<view_num; ++i)
            Nt[i] = 0;
    }

    void request_handle(int curr_request)
    {
        int action, view_request_count[view_num] = {0};
        fscanf(output_file2, "%d", &action);
        switch(action)
        {
        case 0:
            fscanf(output_file2, "%d", &action);
            ++Nt[action];
            break;
        case 1:
            fscanf(output_file2, "%d", &action);
            --Nt[action];
            break;
        case 2:
            fscanf(output_file2, "%d", &action);
            --Nt[action];
            fscanf(output_file2, "%d", &action);
            ++Nt[action];
            break;
        }
        for(int i=0; i<view_num; ++i)
            printf("%d ", Nt[i]);
        printf("\n");
        for(int i=0; i<view_num; ++i)
        {
            if(Nt[i])
            {
                ++view_request_count[Nt[i]];
                for(int j=1; j<=range_size*2; ++j)
                {
                    ++view_request_count[(Nt[i]+view_num-j)%view_num];
                    ++view_request_count[(Nt[i]+j)%view_num];
                }
            }
        }
        for(int i=0; i<view_num; ++i)
            printf("%d ", view_request_count[i]);
        printf("\n-----------------\n");
    }

private:
    int hit;
    int synthesis;
    int view_request_total;
    int total_synthesis;
    int view_num;
    int range_size;
    int cache_size;
    int DIBR_range;
    FILE* output_file2;
    int* Nt;
    int* Cache;
};

int main(int argc, char **argv)
{
    srand(time(NULL));
    FILE* output_file;
    int i, curr_request, view_num = 8, max_user_num = 5;
    int cache_size = 3, range_size = 1, DIBR_range = 5;
    double enter_prob = 0.6, leave_prob = 0.1;
    double cm = 10, cf = 9, cs = 3, a = 0.3;

    //0 is uniform, 1 is zipf, save the request seed for test
    Request request(view_num, max_user_num, enter_prob, leave_prob, 0, 0.502615, 0.288676, 10);

    char file_name1[30] = "./result/LRU_cost_";
    char file_name2[30] = "./result/LRU_hit_";
    char file_name3[30] = "./result/LRU_synthesis_";
    for(i=1; i<=1; ++i)
    {
        output_file = fopen("request_seed", "r");

        LRU_Cache lru_cache(view_num, range_size, cache_size, DIBR_range);
        TTL_Cache ttl_Cache(view_num, range_size, cache_size, DIBR_range, 2, 1);
        Huristic huristic(view_num, range_size, cache_size, DIBR_range);
        //get the request from seed file
        while(!feof(output_file))
        {
            fscanf(output_file, "%d", &curr_request);
            lru_cache.request_handle(curr_request);
            huristic.request_handle(curr_request);
        }
        fclose(output_file);

        //start save the result
        sprintf(file_name1+18, "%d%c", i, '\0');
        output_file = fopen(file_name1, "w");
        fprintf(output_file, "%lf\n", (cm + cf) * (lru_cache.get_view_request_total() - lru_cache.get_hit()) + cs * lru_cache.get_view_request_total() + a * lru_cache.get_total_synthesis());
        fclose(output_file);

        sprintf(file_name2+17, "%d%c", i, '\0');
        output_file = fopen(file_name2, "w");
        fprintf(output_file, "%lf\n", (double)lru_cache.get_hit() / lru_cache.get_view_request_total());
        fclose(output_file);

        sprintf(file_name3+23, "%d%c", i, '\0');
        output_file = fopen(file_name3, "w");
        fprintf(output_file, "%lf\n", (double)lru_cache.get_synthesis() / lru_cache.get_view_request_total());
        fclose(output_file);
    }
    printf("---Simulation finished---\n");
    return 0;
}
