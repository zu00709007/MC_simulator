#include<stdio.h>
#include<time.h>
#include "Request.h"

class LRU_Cache
{
public:
    LRU_Cache(int view_num, int range_size, int cache_size, int DIBR_range) : view_num(view_num), range_size(range_size), cache_size(cache_size), DIBR_range(DIBR_range)
    {
        hit = 0;
        synthesis = 0;
        view_request_total = 0;
        total_synthesis = 0;
        view_request_count = new int[view_num];
        for(int i=0; i<view_num; ++i)
            view_request_count[i] = 0;
        cache_node = new struct Cache_node;
        cache_node->view_content = 0;
        cache_node->next = NULL;
    }

    void request_handle(int curr_request)
    {
        //count the total view request to calculate the sacrifice condition
        range_full_size = (range_size << 1) + 1;
        view_request_total += range_full_size;
        ++view_request_count[curr_request];
        int tmp, index = 0, view_range_rand[range_full_size];
        view_range_rand[index++] = curr_request;
        //renew the view table for count every view request
        for(int i=1; i<=range_size; ++i)
        {
            ++view_request_count[(curr_request+view_num-i)%view_num];
            ++view_request_count[(curr_request+i)%view_num];
            view_range_rand[index++] = (curr_request+view_num-i)%view_num;
            view_range_rand[index++] = (curr_request+i)%view_num;
        }

        for(int i=0; i<range_full_size; ++i)
        {
            int t1 = (int)(range_full_size * (double)rand() / (RAND_MAX + 1));
            int t2 = (int)(range_full_size * (double)rand() / (RAND_MAX + 1));
            tmp = view_range_rand[t1];
            view_range_rand[t1] = view_range_rand[t2];
            view_range_rand[t2] = tmp;
        }

        //record the cache hit, synthesis or miss and renew view in the cache
        for(int i=0; i<range_full_size; ++i)
            if(check_cache(view_range_rand[i]))
                view_range_rand[i] = -1;

        for(int i=0; i<range_full_size; ++i)
            if(0 <= view_range_rand[i])
                renew_cache(view_range_rand[i]);
    }

    int check_cache(int curr_request)
    {
        struct Cache_node* tmp = cache_node->next, **tail = &cache_node;
        int left_min = view_num, left_max = -1, right_min = view_num, right_max = -1, cost1 = DIBR_range + 1, cost2 = DIBR_range + 1, cost3 = DIBR_range + 1;
        while(NULL != tmp)
        {
            //check if the view is hit
            if(curr_request == tmp->view_content)
            {
                ++hit;
                return 1;
            }
            //get the synthesis view for checking the DIBR_range
            if(curr_request < tmp->view_content)
            {
                if(tmp->view_content < right_min)
                    right_min = tmp->view_content;
                if(tmp->view_content > right_max)
                    right_max = tmp->view_content;
            }
            else
            {
                if(tmp->view_content < left_min)
                    left_min = tmp->view_content;
                if(tmp->view_content > left_max)
                    left_max = tmp->view_content;
            }

            tmp = tmp->next;
            tail = &((*tail)->next);
        }
        if((right_min != view_num && left_max != -1 && DIBR_range >= (cost1 = right_min - left_max)) ||
                (left_min * left_max >= 0 && left_min != left_max && DIBR_range >= (cost2 = left_min - left_max + view_num)) ||
                (right_min * right_max >= 0 && right_min != right_max && DIBR_range >= (cost3 = right_min - right_max + view_num)))
        {
            ++hit;
            ++synthesis;
            cost1 = cost1 < cost2 ? cost1 : cost2;
            total_synthesis += cost1 < cost3 ? cost1 : cost3;
            return 1;
        }
        return 0;
    }

    void renew_cache(int curr_request)
    {
        struct Cache_node* tmp = cache_node->next, **tail = &cache_node;
        //traverse the cache
        while(NULL != tmp)
        {
            tmp = tmp->next;
            tail = &((*tail)->next);
        }

        if(cache_size <= cache_node->view_content)
        {
            if(view_request_count[curr_request] > (double)view_request_total / view_num)
            {
                (*tail)->view_content = curr_request;
                (*tail)->next = cache_node->next;
                cache_node->next = (*tail);
                if(1 == cache_size)
                    (*tail)->next = NULL;
                else
                    (*tail) = NULL;
            }
        }
        else
        {
            ++cache_node->view_content;
            tmp = new struct Cache_node;
            tmp->view_content = curr_request;
            tmp->next = cache_node->next;
            cache_node->next = tmp;
        }
    }

    int get_hit()
    {
        return hit;
    }

    int get_synthesis()
    {
        return synthesis;
    }

    int get_view_request_total()
    {
        return view_request_total;
    }

    int get_total_synthesis()
    {
        return total_synthesis;
    }

private:
    struct Cache_node
    {
        int view_content;
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
    int* view_request_count;
    int cache_size;
    struct Cache_node* cache_node;
};


int main(int argc, char **argv)
{
    srand(time(NULL));
    int i, j, view_num = 16, max_user_num = 4, request_num = 50000;
    double enter_prob = 0.6, leave_prob = 0.1;
    Request request(view_num, max_user_num, enter_prob, leave_prob);


    int cache_size = 3, range_size = 2, DIBR_range = 3;
    double cm = 10, cf = 5, cs = 4, a = 0.3;

    FILE* output_file;
    char file_name1[20] = "lrucost";
    char file_name2[20] = "lruhit";
    for(j=2; j<=4; ++j)
    {
        LRU_Cache lru_cache(j, range_size, cache_size, DIBR_range);
        for(i=0; i<request_num; ++i)
        {
            int curr_request = request.request();
            lru_cache.request_handle(curr_request);
        }

        sprintf(file_name1+7, "%d\0", j);
        output_file = fopen(file_name1, "w");
        fprintf (output_file, "%lf\n", (cm + cf) * (lru_cache.get_view_request_total() - lru_cache.get_hit()) + cs * lru_cache.get_view_request_total() + a * lru_cache.get_total_synthesis());
        fclose(output_file);

        sprintf(file_name2+6, "%d\0", j);
        output_file = fopen(file_name2, "w");
        fprintf (output_file, "%lf\n", (double)lru_cache.get_hit() / lru_cache.get_view_request_total());
        fclose(output_file);
    }
    return 0;
}
