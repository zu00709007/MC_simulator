#include<stdio.h>
#include<time.h>
#include<vector>
#include "Request.h"
#include "LRU_Cache.h"
#include "TTL_Cache.h"

using namespace std;

class MDP_train
{
public:
    MDP_train(int cache_size, int view_num, int max_user_num, int range_size) : cache_size(cache_size), view_num(view_num), max_user_num(max_user_num), range_size(range_size)
    {
        int tmp[cache_size];
        //build the cache state list
        tail = (void**)&cache_state_head;
        cache_state_count = 0;
        build_state(0, 0, tmp);
        (*tail) = NULL;

        int tmp2[max_user_num];
        view_state_count = 0;
        tail = (void**)&view_state_head;
        build_view(-1, 0, tmp2);
        (*tail) = NULL;


        data = new struct node_data[view_state_count*cache_state_count*view_num];
        tmp[0] = 0;
        struct view_state *tmp3 = view_state_head;
        while(NULL != tmp3)
        {
            struct cache_state *tmp4 = cache_state_head;
            while(NULL != tmp4)
            {
                for(int i=0; i<view_num; ++i)
                {
                    data[tmp[0]].view_state_index = tmp3;
                    data[tmp[0]].cache_state_index = tmp4;
                    data[tmp[0]].cost = 0;
                    ++tmp[0];
                }
                tmp4 = tmp4->next;
            }
            tmp3 = tmp3->next;
        }

        /*for(int i=1370000; i<view_state_count*cache_state_count*view_num; ++i)
        {
            for(int j=0; j<max_user_num; ++j)
                printf("%d ", data[i].view_state_index->view[j]);

            for(int j=0; j<cache_size; ++j)
                printf("%d ", data[i].cache_state_index->cache[j]);

            printf("%d\n", i%view_num);
        }*/
    }

    void start()
    {
        for(int i=500; i<501; ++i)
        {
            int curr_user = -1;
            for(int j=0; j<max_user_num; ++j)
                if(-1 == data[i].view_state_index->view[j])
                    ++curr_user;
                else
                    break;

            for(int k=0; k<max_user_num; ++k)
                printf("%d ", data[i].view_state_index->view[k]);
            printf("\n");
            for(int k=0; k<cache_size; ++k)
                printf("%d ", data[i].cache_state_index->cache[k]);
            printf("\n-------------\n");

            for(int j=0; j<view_state_count*cache_state_count*view_num; j+=view_num)
            {
                add_event(curr_user, i, j);
                leave_event(curr_user, i, j);
                stay_event(curr_user, i, j);
            }
        }

        for(vector<int>::iterator it=data[500].neighborhood.begin(); it!=data[500].neighborhood.end(); ++it)
        {
            for(int k=0; k<max_user_num; ++k)
                printf("%d ", data[*it].view_state_index->view[k]);
            printf("\n");
            for(int k=0; k<cache_size; ++k)
                printf("%d ", data[*it].cache_state_index->cache[k]);
            printf("\n%d\n", (*it) % view_num);
        }
    }

    void stay_event(int curr_user, int i, int j)
    {

    }

    void add_event(int curr_user, int i, int j)
    {
        int other_user = -1;
        for(int k=0; k<max_user_num; ++k)
            if(-1 == data[j].view_state_index->view[k])
                ++other_user;
            else
                break;
        //check Nt state
        if(curr_user-1 != other_user)
            return;
        ++other_user;
        for(int k=curr_user+1; k<max_user_num; ++k)
        {
            if(data[i].view_state_index->view[k] != data[j].view_state_index->view[other_user])
                break;
            ++other_user;
        }
        for(int k=other_user+1; k<max_user_num; ++k)
            if(data[i].view_state_index->view[k] != data[j].view_state_index->view[k])
                return;

        //check cache state
        if(find_difference(i, j) > (range_size << 1)+1)
            return;

        data[i].neighborhood.push_back(j + data[j].view_state_index->view[other_user]);
    }

    void leave_event(int curr_user, int i, int j)
    {
        int other_user = -1;
        for(int k=0; k<max_user_num; ++k)
            if(-1 == data[j].view_state_index->view[k])
                ++other_user;
            else
                break;
        //check Nt state
        if(curr_user+1 != other_user)
            return;
        ++curr_user;
        for(int k=other_user+1; k<max_user_num; ++k)
        {
            if(data[i].view_state_index->view[curr_user] != data[j].view_state_index->view[k])
                break;
            ++curr_user;
        }
        for(int k=curr_user+1; k<max_user_num; ++k)
            if(data[i].view_state_index->view[k] != data[j].view_state_index->view[k])
                return;

        //check cache state
        if(find_difference(i, j) > (range_size << 1)+1)
            return;

        data[i].neighborhood.push_back(j + data[i].view_state_index->view[curr_user]);
    }

    int find_difference(int a1, int b1)
    {
        int i = 0, j = 0;
        while(i<cache_size && j<cache_size)
        {
            if(data[a1].cache_state_index->cache[i] < data[b1].cache_state_index->cache[j])
                ++i;
            else if(data[b1].cache_state_index->cache[j] < data[a1].cache_state_index->cache[i])
                ++j;
            else
            {
                ++i;
                ++j;
            }
        }
        i -= j;
        if(i<=0)
            return -i;
        return i;
    }

private:
    void build_state(int start, int layer, int tmp[])
    {
        if(layer >= cache_size)
        {
            struct cache_state *tmp_tail = new struct cache_state;
            tmp_tail->cache = new int[cache_size];
            for(int i=0; i<cache_size; ++i)
                tmp_tail->cache[i] = tmp[i];
            (*tail) = (void*)tmp_tail;
            tail = (void**)&(tmp_tail->next);
            ++cache_state_count;
            return;
        }

        for(int i=start; i<view_num; ++i)
        {
            tmp[layer] = i;
            build_state(i+1, layer+1, tmp);
        }
    }

    void build_view(int start, int layer, int tmp[])
    {
        if(layer >= max_user_num)
        {
            struct view_state *tmp_tail = new struct view_state;
            tmp_tail->view = new int[max_user_num];
            for(int i=0; i<max_user_num; ++i)
                tmp_tail->view[i] = tmp[i];
            (*tail) = (void*)tmp_tail;
            tail = (void**)&(tmp_tail->next);
            ++view_state_count;
            return;
        }

        for(int i=start; i<view_num; ++i)
        {
            tmp[layer] = i;
            build_view(i, layer+1, tmp);
        }
    }

    void **tail;
    struct cache_state
    {
        int* cache;
        struct cache_state *next;
    };
    struct cache_state *cache_state_head = NULL;
    int cache_state_count;

    struct view_state
    {
        int* view;
        struct view_state *next;
    };
    struct view_state *view_state_head = NULL;
    int view_state_count;

    struct node_data
    {
        struct view_state *view_state_index;
        struct cache_state *cache_state_index;
        int cost;
        vector<int> neighborhood;
    };
    struct node_data *data;

    int cache_size;
    int view_num;
    int max_user_num;
    int range_size;
};

int main(int argc, char **argv)
{
    srand(time(NULL));
    FILE* output_file;
    int i, curr_request, view_num = 5, max_user_num = 2;
    int cache_size = 3, range_size = 0, DIBR_range = 3;
    double enter_prob = 0.6, leave_prob = 0.1;

    //0 is uniform, 1 is zipf, save the request seed for test
    Request request(view_num, max_user_num, enter_prob, leave_prob, 0, 0.502615, 0.288676, 500000);
    MDP_train mdp_train(cache_size, view_num, max_user_num, range_size);
    mdp_train.start();


    double cm = 10, cf = 5, cs = 4, a = 0.3;
    char file_name1[30] = "./result/LRU_cost_";
    char file_name2[30] = "./result/LRU_hit_";
    char file_name3[30] = "./result/LRU_synthesis_";
    char file_name4[30] = "./result/TTL_cost_";
    char file_name5[30] = "./result/TTL_hit_";
    char file_name6[30] = "./result/TTL_synthesis_";
    for(i=1; i<=1; ++i)
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

        sprintf(file_name4+18, "%d%c", i, '\0');
        output_file = fopen(file_name4, "w");
        fprintf(output_file, "%lf\n", (cm + cf) * (ttl_Cache.get_view_request_total() - ttl_Cache.get_hit()) + cs * ttl_Cache.get_view_request_total() + a * ttl_Cache.get_total_synthesis());
        fclose(output_file);

        sprintf(file_name5+17, "%d%c", i, '\0');
        output_file = fopen(file_name5, "w");
        fprintf(output_file, "%lf\n", (double)ttl_Cache.get_hit() / ttl_Cache.get_view_request_total());
        fclose(output_file);

        sprintf(file_name6+23, "%d%c", i, '\0');
        output_file = fopen(file_name6, "w");
        fprintf(output_file, "%lf\n", (double)ttl_Cache.get_synthesis() / ttl_Cache.get_view_request_total());
        fclose(output_file);
    }
    printf("---Simulation finished---\n");
    return 0;
}
