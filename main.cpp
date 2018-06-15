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
    MDP_train(int cache_size, int view_num, int max_user_num, int range_size, int DIBR_range, double cm, double cf, double cs, double a) : cache_size(cache_size), view_num(view_num), max_user_num(max_user_num), range_size(range_size), DIBR_range(DIBR_range), cm(cm), cf(cf), cs(cs), a(a)
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
                    ++tmp[0];
                }
                tmp4 = tmp4->next;
            }
            tmp3 = tmp3->next;
        }
    }

    void start()
    {
        int i, j, k, curr_user, other_user;
        for(i=0; i<view_state_count*cache_state_count*view_num; ++i)
        {
            curr_user = -1;
            for(j=0; j<max_user_num; ++j)
                if(-1 == data[i].view_state_index->view[j])
                    ++curr_user;
                else
                    break;

            for(j=0; j<view_state_count*cache_state_count*view_num; j+=view_num)
            {
                other_user = -1;
                for(k=0; k<max_user_num; ++k)
                    if(-1 == data[j].view_state_index->view[k])
                        ++other_user;
                    else
                        break;
                add_event(curr_user, other_user, i, j);
                leave_event(curr_user, other_user, i, j);
                stay_event(curr_user, other_user, i, j);
            }
        }

        for(i=0; i<view_state_count*cache_state_count*view_num; ++i)
        {
            vector<int> is_hit;
            node_hit(i, is_hit);
            int e,b,c,d;
            e=b=c=d=0;
            printf("%d %d %d\n",is_hit[0],is_hit[1],is_hit[2]);
            if(is_hit[0]==1 && is_hit[1]==1 && is_hit[2]==1)
            {
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        data[i].cost.push_back(3*cs);
                        //printf("%d\n", data[i].cost[data[i].cost.size()-1]);
                        break;
                    case 1:
                        data[i].cost.push_back(100000);
                        break;
                    case 2:
                        data[i].cost.push_back(100000);
                        break;
                    case 3:
                        data[i].cost.push_back(100000);
                        break;
                    }
                }
            }
            if(is_hit[0]==1 && is_hit[1]==1 && is_hit[2]==0)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        if((3*cs+a*syn_rr)>(cf+3*cs))
                            data[i].cost.push_back(cf+3*cs);
                        else
                            data[i].cost.push_back(3*cs+a*syn_rr);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[2])
                        {
                            data[i].cost.push_back(cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[2])
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rr)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rr);
                                continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[2])
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[2])
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[2])
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[2])
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==1 && is_hit[1]==1 && is_hit[2]==-1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        data[i].cost.push_back(cf+3*cs);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[2])
                        {
                            data[i].cost.push_back(cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[2])
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rr)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if(-1 == tmphit[2])
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[2])
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[2])
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if(-1 == tmphit[2])
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[2])
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[2])
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if(-1 == tmphit[2])
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==1 && is_hit[1]==0 && is_hit[2]==1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        syn_rc = dibr_distance(i, r, dis);
                        dis.clear();
                        if((3*cs+a*syn_rc)>(cf+3*cs))
                            data[i].cost.push_back(cf+3*cs);
                        else
                            data[i].cost.push_back(3*cs+a*syn_rc);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[1])
                        {
                            data[i].cost.push_back(cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[1])
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rc)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rc);
                                continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[1])
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[1])
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[1])
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[1])
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==1 && is_hit[1]==0 && is_hit[2]==0)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1;
                    double cost2;
                    double cost3;
                    double cost4;
                   // printf("+++%d %d %d\n", data[*it].cache_state_index->cache[0], data[*it].cache_state_index->cache[1],data[*it].cache_state_index->cache[2]);
                    //printf("%d %d %d\n", diff[0], diff[1], diff[2]);
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis)+a*dibr_distance(i, r, dis);
                        dis.clear();
                        dis.push_back((r+1)%view_num);
                        cost2 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                        dis.clear();
                        dis.push_back(r);
                        cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        cost4 = 3*cf+3*cs;
                        if(((cost1)<=(cost2)) && ((cost1)<=(cost3)) && ((cost1)<=(cost4)))
                        {
                            data[i].cost.push_back(cost1);
                            continue;
                        }
                        else if(((cost2)<=(cost1)) && ((cost2)<=(cost3)) && ((cost2)<=(cost4)))
                        {
                            data[i].cost.push_back(cost2);
                            continue;
                        }
                        else if(((cost3)<=(cost1)) && ((cost3)<=(cost2)) && ((cost3)<=(cost4)))
                        {
                            data[i].cost.push_back(cost3);
                            continue;
                        }
                        else if(((cost4)<=(cost1)) && ((cost4)<=(cost2)) && ((cost4)<=(cost3)))
                        {
                            data[i].cost.push_back(cost4);
                            continue;
                        }
                        break;
                    case 1:
                        //printf("%d %d\n",find_difference(i, *it),diff.size());
                        //printf("%d %d %d\n", diff[0], diff[1], diff[2]);
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        //printf("%d %d\n",find_difference(i, *it),diff.size());
                            //printf("%d %d %d\n", diff[0], diff[1], diff[2]);
                        if((1 == tmphit[1]) && (1 == tmphit[2]) )
                        {
                            data[i].cost.push_back(100000000);
                                continue;
                        }
                        if((1 == tmphit[1]) && (0 == tmphit[2]) )
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rr)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rc)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        if((0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rc);
                            continue;
                        }
                        if((1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((0 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }

            if(is_hit[0]==1 && is_hit[1]==-1 && is_hit[2]==-1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    //printf("+++++++");
                    double cost1 ;
                    double cost2 ;
                    double cost3 ;
                    double cost4 ;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        data[i].cost.push_back(2*cf+3*cs);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[1]) && (-1 == tmphit[2]) )
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        else if((1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rr)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rr);
                                continue;
                        }
                        else if((0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rc)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rc);
                                continue;

                        }
                        else if((0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                 data[i].cost.push_back(cost2);
                                 continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        else if((-1 == tmphit[1]) && (-1 == tmphit[2]) )
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        else if((1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        else if((1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        else if((0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        else if((0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        else if((-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        if((0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rc);
                            continue;
                        }
                        if((1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((0 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==0 && is_hit[1]==1 && is_hit[2]==1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        syn_rl = dibr_distance(i, (r+view_num-1)%view_num, dis);
                        dis.clear();
                        if((3*cs+a*syn_rl)>(cf+3*cs))
                            data[i].cost.push_back(cf+3*cs);
                        else
                            data[i].cost.push_back(3*cs+a*syn_rl);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[0])
                        {
                            data[i].cost.push_back(cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[0])
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rl)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rl);
                                continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[0])
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[0])
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[0])
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[0])
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==0 && is_hit[1]==1 && is_hit[2]==0)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1 ;
                    double cost2 ;
                    double cost3 ;
                    double cost4 ;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        cost1 = 3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        dis.push_back((r+view_num-1)%view_num);
                        cost2 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        dis.push_back((r+1)%view_num);
                        cost3 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                        dis.clear();
                        cost4 = 2*cf+3*cs;
                        if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                        {
                            data[i].cost.push_back(cost1);
                            continue;
                        }
                        else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                        {
                            data[i].cost.push_back(cost2);
                            continue;
                        }
                        else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                        {
                            data[i].cost.push_back(cost3);
                            continue;
                        }
                        else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                        {
                            data[i].cost.push_back(cost4);
                            continue;
                        }
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (0 == tmphit[2]) )
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rr)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rl)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rl);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==0 && is_hit[1]==1 && is_hit[2]==-1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1;
                    double cost2;
                    double cost3;
                    double cost4;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        dis.clear();
                        dis.push_back((r+1)%view_num);
                        syn_rl = dibr_distance(i, (r+view_num-1)%view_num, dis);
                        dis.clear();
                        if((cf+3*cs+a*syn_rl)>(2*cf+3*cs))
                            data[i].cost.push_back(2*cf+3*cs);
                        else
                            data[i].cost.push_back(cf+3*cs+a*syn_rl);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (-1 == tmphit[2]) )
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rl)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*syn_rl;
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rl);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==0 && is_hit[1]==0 && is_hit[2]==1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1;
                    double cost2;
                    double cost3;
                    double cost4;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        dis.clear();
                        cost1 = 3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, r, dis);
                        dis.clear();
                        dis.push_back(r);
                        cost2 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                        dis.clear();
                        dis.push_back((r+view_num-1)%view_num);
                        cost3 = cf+3*cs+a*dibr_distance(i, r, dis);
                        dis.clear();
                        cost4 = 2*cf+3*cs;
                        if(((cost1)<=(cost2)) && ((cost1)<=(cost3)) && ((cost1)<=(cost4)))
                        {
                            data[i].cost.push_back(cost1);
                            continue;
                        }
                        else if(((cost2)<=(cost1)) && ((cost2)<=(cost3)) && ((cost2)<=(cost4)))
                        {
                            data[i].cost.push_back(cost2);
                            continue;
                        }
                        else if(((cost3)<=(cost1)) && ((cost3)<=(cost2)) && ((cost3)<=(cost4)))
                        {
                            data[i].cost.push_back(cost3);
                            continue;
                        }
                        else if(((cost4)<=(cost1)) && ((cost4)<=(cost2)) && ((cost4)<=(cost3)))
                        {
                            data[i].cost.push_back(cost4);
                            continue;
                        }
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]) )
                        {
                            data[i].cost.push_back(100000000);
                                continue;
                        }
                        if((1 == tmphit[0]) && (1 == tmphit[1]) )
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rl)>(cf+3*cs+a*syn_rc))
                                data[i].cost.push_back(cf+3*cs+a*syn_rc);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) )
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rl)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rc)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl+a*syn_rc);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==0 && is_hit[1]==0 && is_hit[2]==0)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1;
                    double cost2;
                    double cost3;
                    double cost4;
                    double cost5;
                    double cost6;
                    double cost7;
                    double cost8;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        dis.clear();
                        cost1 = 3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, r, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        dis.push_back((r+1)%view_num);
                        cost2 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, r, dis);
                        dis.clear();
                        dis.push_back(r);
                        cost3 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        dis.push_back((r+view_num-1)%view_num);
                        cost4 = cf+3*cs+a*dibr_distance(i, r, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        dis.push_back(r);
                        dis.push_back((r+1)%view_num);
                        cost5 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                        dis.clear();
                        dis.push_back((r+view_num-1)%view_num);
                        dis.push_back((r+1)%view_num);
                        cost6 = 2*cf+3*cs+a*syn_rc;
                        dis.clear();
                        dis.push_back((r+view_num-1)%view_num);
                        dis.push_back(r);
                        cost7 = 2*cf+3*cs+a*syn_rr;
                        dis.clear();
                        cost8 = 3*cf+3*cs;
                        if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4) && (cost1<=cost5) && (cost1<=cost6) && (cost1<=cost7) && (cost1<=cost8))
                        {
                            data[i].cost.push_back(cost1);
                            continue;
                        }
                        else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4) && (cost2<=cost5) && (cost2<=cost6) && (cost2<=cost7) && (cost2<=cost8))
                        {
                            data[i].cost.push_back(cost2);
                            continue;
                        }
                        else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4) && (cost3<=cost5) && (cost3<=cost6) && (cost3<=cost7) && (cost3<=cost8))
                        {
                            data[i].cost.push_back(cost3);
                            continue;
                        }
                        else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3) && (cost4<=cost5) && (cost4<=cost6) && (cost4<=cost7) && (cost4<=cost8))
                        {
                            data[i].cost.push_back(cost4);
                            continue;
                        }
                        else if((cost5<=cost1) && (cost5<=cost2) && (cost5<=cost3) && (cost5<=cost4) && (cost5<=cost6) && (cost5<=cost7) && (cost5<=cost8))
                        {
                            data[i].cost.push_back(cost5);
                            continue;
                        }
                        else if((cost6<=cost1) && (cost6<=cost2) && (cost6<=cost3) && (cost6<=cost4) && (cost6<=cost5) && (cost6<=cost7) && (cost6<=cost8))
                        {
                            data[i].cost.push_back(cost6);
                            continue;
                        }
                        else if((cost7<=cost1) && (cost7<=cost2) && (cost7<=cost3) && (cost7<=cost4) && (cost7<=cost5) && (cost7<=cost6) && (cost7<=cost8))
                        {
                            data[i].cost.push_back(cost7);
                            continue;
                        }
                        else if((cost8<=cost1) && (cost8<=cost2) && (cost8<=cost3) && (cost8<=cost4) && (cost8<=cost5) && (cost8<=cost6) && (cost8<=cost7))
                        {
                            data[i].cost.push_back(cost8);
                            continue;
                        }
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]) )
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }

                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, r, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff)+a*dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost4 = 2*cf+3*cs+a*dibr_distance(i, r, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost5 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost6 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost7 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4) && (cost1<=cost5) && (cost1<=cost6) && (cost1<=cost7) )
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4) && (cost2<=cost5) && (cost2<=cost6) && (cost2<=cost7) )
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4) && (cost3<=cost5) && (cost3<=cost6) && (cost3<=cost7) )
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3) && (cost4<=cost5) && (cost4<=cost6) && (cost4<=cost7) )
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                            else if((cost5<=cost1) && (cost5<=cost2) && (cost5<=cost3) && (cost5<=cost4) && (cost5<=cost6) && (cost5<=cost7) )
                            {
                                data[i].cost.push_back(cost5);
                                continue;
                            }
                            else if((cost6<=cost1) && (cost6<=cost2) && (cost6<=cost3) && (cost6<=cost4) && (cost6<=cost5) && (cost6<=cost7) )
                            {
                                data[i].cost.push_back(cost6);
                                continue;
                            }
                            else if((cost7<=cost1) && (cost7<=cost2) && (cost7<=cost3) && (cost7<=cost4) && (cost7<=cost5) && (cost7<=cost6) )
                            {
                                data[i].cost.push_back(cost7);
                                continue;
                            }
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, r, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, r, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost4 = 3*cf+3*cs+a*dibr_distance(i, r, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rc);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rc = dibr_distance(i, r, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl+a*syn_rc+a*syn_rr);
                            continue;
                        }
                        break;
                    }
                }
            }


            if(is_hit[0]==-1 && is_hit[1]==1 && is_hit[2]==1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        data[i].cost.push_back(cf+3*cs);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[0])
                        {
                            data[i].cost.push_back(cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[0])
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rl)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if(-1 == tmphit[0])
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[0])
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[0])
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if(-1 == tmphit[0])
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if(1 == tmphit[0])
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if(0 == tmphit[0])
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if(-1 == tmphit[0])
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==-1 && is_hit[1]==1 && is_hit[2]==0)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1;
                    double cost2;
                    double cost3;
                    double cost4;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        dis.clear();
                        dis.push_back((r+view_num-1)%view_num);
                        syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                        dis.clear();
                        if((cf+3*cs+a*syn_rr)>(2*cf+3*cs))
                            data[i].cost.push_back(2*cf+3*cs);
                        else
                            data[i].cost.push_back(cf+3*cs+a*syn_rr);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((-1 == tmphit[0]) && (1 == tmphit[2]) )
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rr)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rl);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==-1 && is_hit[1]==1 && is_hit[2]==-1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1;
                    double cost2;
                    double cost3;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        data[i].cost.push_back(2*cf+3*cs);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (-1 == tmphit[2]) )
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[2]))
                        {

                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rl);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }

            if(is_hit[0]==-1 && is_hit[1]==-1 && is_hit[2]==1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1 = cf+3*cs+a*syn_rr+a*syn_rc;
                    double cost2 = 2*cf+3*cs+a*syn_rc;
                    double cost3 = 2*cf+3*cs+a*syn_rr;
                    double cost4 = 3*cf+3*cs;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        data[i].cost.push_back(2*cf+3*cs);
                        break;
                    case 1:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) )
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rl)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((cf+3*cs+a*syn_rc)>(2*cf+3*cs))
                                data[i].cost.push_back(2*cf+3*cs);
                            else
                                data[i].cost.push_back(cf+3*cs+a*syn_rc);
                                continue;

                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) )
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        break;
                    case 2:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            data[i].cost.push_back(2*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rc = dibr_distance(i, r, dis);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        break;
                    case 3:
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl+a*syn_rc);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[1]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[1]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[1]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }
            if(is_hit[0]==-1 && is_hit[1]==-1 && is_hit[2]==-1)
            {
                int r = i%view_num;
                for(vector<int>::iterator it=data[i].neighborhood.begin(); it!=data[i].neighborhood.end(); ++it)
                {
                    vector<int> diff;
                    diff.clear();
                    vector<int> tmphit;
                    tmphit.clear();
                    vector<int> dis;
                    dis.clear();
                    compare(i, *it, diff);
                    while(find_difference(i, *it) != diff.size())
                    {
                        diff.pop_back();
                    }
                    double syn_rl;
                    double syn_rc;
                    double syn_rr;
                    double cost1;
                    double cost2;
                    double cost3;
                    double cost4;
                    double cost5;
                    double cost6;
                    double cost7;
                    switch(find_difference(i, *it))
                    {
                    case 0:
                        e++;
                        data[i].cost.push_back(3*cf+3*cs);
                        break;
                    case 1:
                        b++;
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]) )
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            syn_rc = dibr_distance(i, r, dis);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            syn_rl = dibr_distance(i, r, dis);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            cost4 = 3*cf+3*cs;
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff)+a*dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost2 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost3 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            cost4 = 2*cf+3*cs+a*dibr_distance(i, r, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost5 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            cost6 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            cost7 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();

                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4) && (cost1<=cost5) && (cost1<=cost6) && (cost1<=cost7) )
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4) && (cost2<=cost5) && (cost2<=cost6) && (cost2<=cost7) )
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4) && (cost3<=cost5) && (cost3<=cost6) && (cost3<=cost7) )
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3) && (cost4<=cost5) && (cost4<=cost6) && (cost4<=cost7) )
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                            else if((cost5<=cost1) && (cost5<=cost2) && (cost5<=cost3) && (cost5<=cost4) && (cost5<=cost6) && (cost5<=cost7) )
                            {
                                data[i].cost.push_back(cost5);
                                continue;
                            }
                            else if((cost6<=cost1) && (cost6<=cost2) && (cost6<=cost3) && (cost6<=cost4) && (cost6<=cost5) && (cost6<=cost7) )
                            {
                                data[i].cost.push_back(cost6);
                                continue;
                            }
                            else if((cost7<=cost1) && (cost7<=cost2) && (cost7<=cost3) && (cost7<=cost4) && (cost7<=cost5) && (cost7<=cost6) )
                            {
                                data[i].cost.push_back(cost7);
                                continue;
                            }
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    case 2:
                        c++;
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rr)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rr);
                                continue;
                        }
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rc)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rc);
                                continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, r, diff)+a*dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            if((2*cf+3*cs+a*syn_rl)>(3*cf+3*cs))
                                data[i].cost.push_back(3*cf+3*cs);
                            else
                                data[i].cost.push_back(2*cf+3*cs+a*syn_rl);
                                continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, r, diff);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            cost1 = 2*cf+3*cs+a*dibr_distance(i, r, diff)+a*dibr_distance(i, (r+view_num-1)%view_num, diff)+a*dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            dis.push_back((r+1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost2 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, r, dis);
                            dis.clear();
                            dis.push_back(r);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost3 = 3*cf+3*cs+a*dibr_distance(i, (r+view_num-1)%view_num, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            cost4 = 3*cf+3*cs+a*dibr_distance(i, r, dis)+a*dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            if((cost1<=cost2) && (cost1<=cost3) && (cost1<=cost4))
                            {
                                data[i].cost.push_back(cost1);
                                continue;
                            }
                            else if((cost2<=cost1) && (cost2<=cost3) && (cost2<=cost4))
                            {
                                data[i].cost.push_back(cost2);
                                continue;
                            }
                            else if((cost3<=cost1) && (cost3<=cost2) && (cost3<=cost4))
                            {
                                data[i].cost.push_back(cost3);
                                continue;
                            }
                            else if((cost4<=cost1) && (cost4<=cost2) && (cost4<=cost3))
                            {
                                data[i].cost.push_back(cost4);
                                continue;
                            }
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            dis.clear();
                            dis.push_back((r+view_num-1)%view_num);
                            dis.push_back(diff[0]);
                            dis.push_back(diff[1]);
                            syn_rr = dibr_distance(i, (r+1)%view_num, dis);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((0 == tmphit[0]) && (-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    case 3:
                        d++;
                        for(int j=1; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, diff));
                        for(int j=0; j<=range_size; ++j)
                            tmphit.push_back(check_cache(i, (i%view_num+j)%view_num, diff));
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(3*cf+3*cs);
                            continue;
                        }
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rc);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rr+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rc+a*syn_rl);
                            continue;
                        }
                        if((0 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            syn_rc = dibr_distance(i, r, diff);
                            syn_rl = dibr_distance(i, (r+view_num-1)%view_num, diff);
                            syn_rr = dibr_distance(i, (r+1)%view_num, diff);
                            dis.clear();
                            data[i].cost.push_back(3*cf+3*cs+a*syn_rl+a*syn_rc+a*syn_rr);
                            continue;
                        }
                        if((1 == tmphit[0]) && (1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((1 == tmphit[0]) && (0 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((1 == tmphit[0]) && (-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (0 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (0 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        if((-1 == tmphit[0]) && (-1 == tmphit[1]) && (-1 == tmphit[2]))
                        {
                            data[i].cost.push_back(100000000);
                            continue;
                        }
                        break;
                    }
                }
            }
        }

		for(int i=0; i<view_state_count*cache_state_count*view_num; ++i)
		{
			printf("State:%d neighborhood:%d cost:%d\n", i, data[i].neighborhood.size(), data[i].cost.size());
		}
        /*for(int k=0; k<cache_size; ++k)
            printf("%d ", data[805].cache_state_index->cache[k]);
            printf("%d\n",(i-1)%view_num);
        printf("\n");
        vector<int> diff;
        diff.push_back(6);
        diff.push_back(7);
        diff.push_back(14);
        printf("\n%d\n", check_cache(20, 2, diff));
        printf("%d %d\n", data[805].neighborhood.size(), data[805].cost.size());
        for(vector<int>::iterator it=data[805].cost.begin(); it!=data[805].cost.end(); ++it)
        {
	    printf("%d\n", *it);
            for(int k=0; k<max_user_num; ++k)
                printf("%d ", data[*it].view_state_index->view[k]);
            printf("\n");
            for(int k=0; k<cache_size; ++k)
                printf("%d ", data[*it].cache_state_index->cache[k]);
            printf("\n%d\n", (*it) % view_num);
        }
        printf("%d\n", dibr_distance(38, 15, diff));*/
    }

private:
    void node_hit(int i, vector<int> &diff)
    {
        vector<int> tmp;
        for(int j=1; j<=range_size; ++j)
            diff.push_back(check_cache(i, (i%view_num+view_num-j)%view_num, tmp));

        for(int j=0; j<=range_size; ++j)
            diff.push_back(check_cache(i, (i%view_num+j)%view_num, tmp));
    }

    double dibr_distance(int i, int curr_request, vector<int> &addition)
    {
        int left_min = view_num, left_max = -1, right_min = view_num, right_max = -1, cost1 = DIBR_range + 1, cost2 = DIBR_range + 1, cost3 = DIBR_range + 1;
        for(int j=0; j<cache_size; ++j)
        {
            //get the synthesis view for checking the DIBR_range
            if(curr_request < data[i].cache_state_index->cache[j])
            {
                if(data[i].cache_state_index->cache[j] < right_min)
                    right_min = data[i].cache_state_index->cache[j];
                if(data[i].cache_state_index->cache[j] > right_max)
                    right_max = data[i].cache_state_index->cache[j];
            }
            else
            {
                if(data[i].cache_state_index->cache[j] < left_min)
                    left_min = data[i].cache_state_index->cache[j];
                if(data[i].cache_state_index->cache[j] > left_max)
                    left_max = data[i].cache_state_index->cache[j];
            }
        }
        for(vector<int>::iterator it=addition.begin(); it!=addition.end(); ++it)
        {
            //get the synthesis view for checking the DIBR_range
            if(curr_request < (*it))
            {
                if((*it) < right_min)
                    right_min = (*it);
                if((*it) > right_max)
                    right_max = (*it);
            }
            else
            {
                if((*it) < left_min)
                    left_min = (*it);
                if((*it) > left_max)
                    left_max = (*it);
            }
        }
        if((right_min != view_num && left_max != -1 && DIBR_range >= (cost1 = right_min - left_max)) ||
                (left_min * left_max >= 0 && left_min != left_max && DIBR_range >= (cost2 = left_min - left_max + view_num)) ||
                (right_min * right_max >= 0 && right_min != right_max && DIBR_range >= (cost3 = right_min - right_max + view_num)))
        {
            cost1 = cost1 < cost2 ? cost1 : cost2;
            return (double)(cost1 < cost3 ? cost1 : cost3);
        }
        return -1;
    }

    int check_cache(int i, int curr_request, vector<int> &addition)
    {
        int left_min = view_num, left_max = -1, right_min = view_num, right_max = -1;
        for(int j=0; j<cache_size; ++j)
        {
            //check if the view is hit
            if(curr_request == data[i].cache_state_index->cache[j])
                return 1;

            //get the synthesis view for checking the DIBR_range
            if(curr_request < data[i].cache_state_index->cache[j])
            {
                if(data[i].cache_state_index->cache[j] < right_min)
                    right_min = data[i].cache_state_index->cache[j];
                if(data[i].cache_state_index->cache[j] > right_max)
                    right_max = data[i].cache_state_index->cache[j];
            }
            else
            {
                if(data[i].cache_state_index->cache[j] < left_min)
                    left_min = data[i].cache_state_index->cache[j];
                if(data[i].cache_state_index->cache[j] > left_max)
                    left_max = data[i].cache_state_index->cache[j];
            }
        }
        for(vector<int>::iterator it=addition.begin(); it!=addition.end(); ++it)
        {
            //check if the view is hit
            if(curr_request == (*it))
                return 1;
            //get the synthesis view for checking the DIBR_range
            if(curr_request < (*it))
            {
                if((*it) < right_min)
                    right_min = (*it);
                if((*it) > right_max)
                    right_max = (*it);
            }
            else
            {
                if((*it) < left_min)
                    left_min = (*it);
                if((*it) > left_max)
                    left_max = (*it);
            }
        }
        if((right_min != view_num && left_max != -1 && DIBR_range >= right_min - left_max) ||
                (left_min * left_max >= 0 && left_min != left_max && DIBR_range >= left_min - left_max + view_num) ||
                (right_min * right_max >= 0 && right_min != right_max && DIBR_range >= right_min - right_max + view_num))
            return 0;
        return -1;
    }

    void compare(int i, int j, vector<int> &diff)
    {
        int curr_cache = 0, other_cache = 0;
        while(curr_cache < cache_size && other_cache < cache_size)
        {
            if(data[i].cache_state_index->cache[curr_cache] > data[j].cache_state_index->cache[other_cache])
            {
                diff.push_back(data[j].cache_state_index->cache[other_cache++]);
            }
            else if(data[i].cache_state_index->cache[curr_cache] == data[j].cache_state_index->cache[other_cache])
            {
                ++curr_cache;
                ++other_cache;
            }
            else
                ++curr_cache;
        }
        while(other_cache < cache_size)
            diff.push_back(data[j].cache_state_index->cache[other_cache++]);
    }

    void stay_event(int curr_user, int other_user, int i, int j)
    {
        int same;
        if(curr_user != other_user)
            return;
        ++other_user;
        ++curr_user;
        same = curr_user;
        while(curr_user < max_user_num)
        {
            if(data[i].view_state_index->view[curr_user] != data[j].view_state_index->view[other_user])
                break;
            ++other_user;
            ++curr_user;
        }
        if(other_user < max_user_num && -1 != data[i].view_state_index->view[curr_user] - data[j].view_state_index->view[other_user] && 1 != data[i].view_state_index->view[curr_user] - data[j].view_state_index->view[other_user])
            return;
        for(int k=other_user+1; k<max_user_num; ++k)
            if(data[i].view_state_index->view[k] != data[j].view_state_index->view[k])
                return;

        //check cache state
        if(find_difference(i, j) > (range_size << 1)+1)
            return;

        if(other_user < max_user_num)
            data[i].neighborhood.push_back(j + data[j].view_state_index->view[other_user]);
        else
            for(; same<max_user_num; ++same)
                data[i].neighborhood.push_back(j + data[j].view_state_index->view[same]);
    }

    void add_event(int curr_user, int other_user, int i, int j)
    {
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

    void leave_event(int curr_user, int other_user, int i, int j)
    {
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
        vector<int> neighborhood;
        vector<int> cost;
    };
    struct node_data *data;

    int cache_size;
    int view_num;
    int max_user_num;
    int range_size;
    int DIBR_range;
    double cm, cf, cs, a;
};

int main(int argc, char **argv)
{
    srand(time(NULL));
    FILE* output_file;
    int i, curr_request, view_num = 16, max_user_num = 2;
    int cache_size = 3, range_size = 1, DIBR_range = 5;
    double enter_prob = 0.6, leave_prob = 0.1;
    double cm = 10, cf = 9, cs = 3, a = 0.3;

    //0 is uniform, 1 is zipf, save the request seed for test
    Request request(view_num, max_user_num, enter_prob, leave_prob, 0, 0.502615, 0.288676, 500000);
    MDP_train mdp_train(cache_size, view_num, max_user_num, range_size, DIBR_range, cm, cf, cs, a);
    mdp_train.start();

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
