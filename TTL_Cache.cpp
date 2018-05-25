#include "TTL_Cache.h"

TTL_Cache::TTL_Cache(int view_num, int range_size, int cache_size, int DIBR_range, int high_quality, int low_quality) : view_num(view_num), range_size(range_size), cache_size(cache_size), DIBR_range(DIBR_range), high_quality(high_quality), low_quality(low_quality)
{
    hit = 0;
    synthesis = 0;
    view_request_total = 0;
    total_synthesis = 0;
    view_request_count = new int[view_num];
    view_quality_count = new int[view_num];
    for(int i=0; i<view_num; ++i)
    {
        view_request_count[i] = 0;
        view_quality_count[i] = low_quality;
    }
    cache_node = new struct Cache_node;
    cache_node->view_content = 0;
    cache_node->next = NULL;
}

void TTL_Cache::request_handle(int curr_request)
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
    //random the sequence of view range
    for(int i=0; i<range_full_size; ++i)
    {
        int t1 = (int)(range_full_size * (double)rand() / ((double)RAND_MAX + 1));
        int t2 = (int)(range_full_size * (double)rand() / ((double)RAND_MAX + 1));
        tmp = view_range_rand[t1];
        view_range_rand[t1] = view_range_rand[t2];
        view_range_rand[t2] = tmp;
    }
    //TTL minus 1
    expired();
    //record the cache hit, synthesis or miss and renew view in the cache
    for(int i=0; i<range_full_size; ++i)
        if(check_cache(view_range_rand[i]))
            view_range_rand[i] = -1;
    //remove the expired view from cache
    view_dead();

    for(int i=0; i<range_full_size; ++i)
        if(0 <= view_range_rand[i])
            renew_cache(view_range_rand[i]);
}

void TTL_Cache::expired()
{
    struct Cache_node* tmp = cache_node->next;
    while(NULL != tmp)
    {
        --tmp->TTL;
        tmp = tmp->next;
    }
}

int TTL_Cache::check_cache(int curr_request)
{
    struct Cache_node* tmp = cache_node->next;
    int left_min = view_num, left_max = -1, right_min = view_num, right_max = -1, cost1 = DIBR_range + 1, cost2 = DIBR_range + 1, cost3 = DIBR_range + 1;
    while(NULL != tmp)
    {
        //check if the view is hit and reset TTL in cache
        if(curr_request == tmp->view_content)
        {
            ++hit;
            tmp->TTL = view_quality_count[curr_request];
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
    }
    if((right_min != view_num && left_max != -1 && DIBR_range >= (cost1 = right_min - left_max)) ||
            (left_min * left_max >= 0 && left_min != left_max && DIBR_range >= (cost2 = left_min - left_max + view_num)) ||
            (right_min * right_max >= 0 && right_min != right_max && DIBR_range >= (cost3 = right_min - right_max + view_num)))
    {
        ++hit;
        ++synthesis;
        cost1 = cost1 < cost2 ? cost1 : cost2;
        total_synthesis += cost1 < cost3 ? cost1 : cost3;
    }
    return 0;
}

void TTL_Cache::view_dead()
{
    struct Cache_node** tmp = &(cache_node->next);
    while(NULL != (*tmp))
    {
        if(0 == (*tmp)->TTL)
        {
            struct Cache_node* release = (*tmp);
            (*tmp) = release->next;
            --cache_node->view_content;
            delete release;
        }
        else
            tmp = &((*tmp)->next);
    }
}

void TTL_Cache::renew_cache(int curr_request)
{
    struct Cache_node* tmp = cache_node->next;
    if(cache_size > cache_node->view_content)
    {
        //check the TTL initialization
        if(view_request_count[curr_request] > (double)view_request_total / view_num)
            view_quality_count[curr_request] = high_quality;
        else
            view_quality_count[curr_request] = low_quality;

        ++cache_node->view_content;
        tmp = new struct Cache_node;
        tmp->view_content = curr_request;
        tmp->TTL = view_quality_count[curr_request];
        tmp->next = cache_node->next;
        cache_node->next = tmp;
    }
}

int TTL_Cache::get_hit()
{
    return hit;
}

int TTL_Cache::get_synthesis()
{
    return synthesis;
}

int TTL_Cache::get_view_request_total()
{
    return view_request_total;
}

int TTL_Cache::get_total_synthesis()
{
    return total_synthesis;
}
