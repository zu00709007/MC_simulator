#include<stdio.h>
#include<time.h>
#include "Request.h"
#include "LRU_Cache.h"
#include "TTL_Cache.h"
#include "Huristic.h"

int main(int argc, char **argv)
{
    srand(time(NULL));
    FILE* output_file;
    int i, curr_request, view_num = 16, max_user_num = 6;
    int cache_size = 3, range_size = 2, DIBR_range = 3;
    double enter_prob = 0.6, leave_prob = 0.1;
    double cm = 10, cf = 9, cs = 3, a = 0.3, b = 9;

    //0 is uniform, 1 is zipf, save the request seed for test
    Request request(view_num, max_user_num, enter_prob, leave_prob, 0, 0.502615, 0.288676, 5);

    char file_name1[30] = "./result/heucost";
    char file_name2[30] = "./result/heuhit";
    char file_name3[30] = "./result/heusyn";
    for(i=1; i<=1; ++i)
    {
        output_file = fopen("request_seed", "r");

        LRU_Cache lru_cache(view_num, range_size, cache_size, DIBR_range);
        TTL_Cache ttl_Cache(view_num, range_size, cache_size, DIBR_range, 2, 1);
        Huristic huristic(view_num, range_size, cache_size, DIBR_range, cf, a, b);
        //get the request from seed file
        while(!feof(output_file))
        {
            fscanf(output_file, "%d", &curr_request);
            lru_cache.request_handle(curr_request);
            huristic.request_handle(curr_request);
        }
        fclose(output_file);

        //start save the result
        sprintf(file_name1+16, "%d%c", i, '\0');
        output_file = fopen(file_name1, "w");
        fprintf(output_file, "%lf\n", cf * huristic.get_cf_view() + cm * (huristic.get_view_request_total() - huristic.get_hit()) + cs * huristic.get_view_request_total() + a * huristic.get_total_synthesis());
        fclose(output_file);

        sprintf(file_name2+15, "%d%c", i, '\0');
        output_file = fopen(file_name2, "w");
        fprintf(output_file, "%lf\n", (double)huristic.get_hit() / huristic.get_view_request_total());
        fclose(output_file);

        sprintf(file_name3+15, "%d%c", i, '\0');
        output_file = fopen(file_name3, "w");
        fprintf(output_file, "%lf\n", (double)huristic.get_synthesis() / huristic.get_view_request_total());
        fclose(output_file);
    }
    printf("---Simulation finished---\n");
    return 0;
}
