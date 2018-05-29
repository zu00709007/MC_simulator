#include<stdio.h>
#include<time.h>
#include "Request.h"
#include "LRU_Cache.h"
#include "TTL_Cache.h"

int main(int argc, char **argv)
{
    srand(time(NULL));
    FILE* output_file;
    int i, j, curr_request, view_num = 16, max_user_num = 4, request_num = 500000;
    double enter_prob = 0.6, leave_prob = 0.1;
    //0 is uniform, 1 is zipf
    Request request(view_num, max_user_num, enter_prob, leave_prob, 0, 0.502615, 0.288676);

    //save the request seed for test
    output_file = fopen("request_seed", "w");
    for(i=0; i<request_num-1; ++i)
        fprintf(output_file, "%d\n", request.request());
    fprintf(output_file, "%d", request.request());
    fclose(output_file);


    int cache_size = 3, range_size = 2, DIBR_range = 3;
    double cm = 10, cf = 5, cs = 4, a = 0.3;

    char file_name1[30] = "./result/LRU_cost_";
    char file_name2[30] = "./result/LRU_hit_";
    char file_name3[30] = "./result/LRU_synthesis_";
    char file_name4[30] = "./result/TTL_cost_";
    char file_name5[30] = "./result/TTL_hit_";
    char file_name6[30] = "./result/TTL_synthesis_";
    for(j=1; j<=1; ++j)
    {
        output_file = fopen("request_seed", "r");
        LRU_Cache lru_cache(view_num, range_size, cache_size, DIBR_range);
        TTL_Cache ttl_Cache(view_num, range_size, cache_size, DIBR_range, 2, 1);
        //get the request from seed file
        while(!feof(output_file))
        {
            fscanf(output_file, "%d", &curr_request);
            lru_cache.request_handle(curr_request);
            ttl_Cache.request_handle(curr_request);
        }
        fclose(output_file);

        //start save the result
        sprintf(file_name1+18, "%d%c", j, '\0');
        output_file = fopen(file_name1, "w");
        fprintf(output_file, "%lf\n", (cm + cf) * (lru_cache.get_view_request_total() - lru_cache.get_hit()) + cs * lru_cache.get_view_request_total() + a * lru_cache.get_total_synthesis());
        fclose(output_file);

        sprintf(file_name2+17, "%d%c", j, '\0');
        output_file = fopen(file_name2, "w");
        fprintf(output_file, "%lf\n", (double)lru_cache.get_hit() / lru_cache.get_view_request_total());
        fclose(output_file);

        sprintf(file_name3+23, "%d%c", j, '\0');
        output_file = fopen(file_name3, "w");
        fprintf(output_file, "%lf\n", (double)lru_cache.get_synthesis() / lru_cache.get_view_request_total());
        fclose(output_file);

        sprintf(file_name4+18, "%d%c", j, '\0');
        output_file = fopen(file_name4, "w");
        fprintf(output_file, "%lf\n", (cm + cf) * (ttl_Cache.get_view_request_total() - ttl_Cache.get_hit()) + cs * ttl_Cache.get_view_request_total() + a * ttl_Cache.get_total_synthesis());
        fclose(output_file);

        sprintf(file_name5+17, "%d%c", j, '\0');
        output_file = fopen(file_name5, "w");
        fprintf(output_file, "%lf\n", (double)ttl_Cache.get_hit() / ttl_Cache.get_view_request_total());
        fclose(output_file);

        sprintf(file_name6+23, "%d%c", j, '\0');
        output_file = fopen(file_name6, "w");
        fprintf(output_file, "%lf\n", (double)ttl_Cache.get_synthesis() / ttl_Cache.get_view_request_total());
        fclose(output_file);
    }
    printf("---Simulation finished---\n");
    return 0;
}
