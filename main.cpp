#include<stdio.h>
#include<time.h>
#include "Request.h"
#include "LRU_Cache.h"

int main(int argc, char **argv)
{
    srand(time(NULL));
    int i, j, view_num = 16, max_user_num = 4, request_num = 1;
    double enter_prob = 0.6, leave_prob = 0.1;
    Request request(view_num, max_user_num, enter_prob, leave_prob);

    int cache_size = 3, range_size = 2, DIBR_range = 3;
    double cm = 10, cf = 5, cs = 4, a = 0.3;

    FILE* output_file;
    char file_name1[20] = "./result/LRU_cost";
    char file_name2[20] = "./result/LRU_hit";
    for(j=1; j<=10; ++j)
    {
        LRU_Cache lru_cache(view_num, range_size, cache_size, DIBR_range);
        for(i=0; i<request_num; ++i)
        {
            int curr_request = request.request();
            lru_cache.request_handle(curr_request);
        }

        sprintf(file_name1+17, "%d%c", j, '\0');
        output_file = fopen(file_name1, "w");
        fprintf (output_file, "%lf\n", (cm + cf) * (lru_cache.get_view_request_total() - lru_cache.get_hit()) + cs * lru_cache.get_view_request_total() + a * lru_cache.get_total_synthesis());
        fclose(output_file);

        sprintf(file_name2+16, "%d%c", j, '\0');
        output_file = fopen(file_name2, "w");
        fprintf (output_file, "%lf\n", (double)lru_cache.get_hit() / lru_cache.get_view_request_total());
        fclose(output_file);

    }
    printf("---Simulation finished---\n");
    return 0;
}
