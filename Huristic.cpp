#include "Huristic.h"

Huristic::Huristic(int view_num, int range_size, int cache_size, int DIBR_range, int cf, double a, double b) : output_file2(output_file2), view_num(view_num), range_size(range_size), cache_size(cache_size), DIBR_range(DIBR_range), cf(cf), a(a), b(b)
{
    output_file2 = fopen("Nt_seed", "r");
    hit = 0;
    synthesis = 0;
    view_request_total = 0;
    total_synthesis = 0;
    cf_view = 0;
    Cache = new int[cache_size];
    for(int i=0; i<cache_size; ++i)
        Cache[i] = i;
    Nt = new int[view_num];
    for(int i=0; i<view_num; ++i)
        Nt[i] = 0;
}

void Huristic::request_handle(int curr_request)
{
    view_request_total += (range_size << 1) + 1;
    int action, request_total = 0, view_request_count[view_num] = {0};
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
    {
        int tmp = Nt[i];
        while(tmp>0)
        {
            ++request_total;
            int tmp2 = (range_size << 1) + 1;
            view_request_count[i] += tmp2;
            for(int j=1; j<=range_size*2; ++j)
            {
                --tmp2;
                view_request_count[(i+view_num-j)%view_num] += tmp2;
                view_request_count[(i+j)%view_num] += tmp2;
            }
            --tmp;
        }
    }
    curr_request = (curr_request+view_num-range_size)%view_num;
    double table[(range_size << 1) + 1] = {0};
    int choose[(range_size << 1) + 1] = {-1};
    int choose_index[(range_size << 1) + 1] = {-1};
    for(int i=0; i<(range_size << 1) + 1; ++i)
    {
        choose_index[i] = curr_request;
        for(int j=0; j<cache_size; ++j)
        {
            if(curr_request == Cache[j])
            {
                table[i] = INT_MIN;
                ++hit;
                break;
            }
            table[i] = cf - (double)b * view_request_count[curr_request] / (request_total*((range_size << 1) + 1)*((range_size << 1) + 1)+1);
        }
        double min_cost = INT_MAX;
        for(int j=i-DIBR_range>=0 ? i-DIBR_range : 0; j<i; ++j)
        {
            if(table[j]+a*(i-j)*(i-j-1) < min_cost)
            {
                min_cost = table[j] + a * (i - j) * (i - j - 1);
                choose[i] = j;
            }
        }
        if(0 != i)
            table[i] += min_cost;
        curr_request = (curr_request+1)%view_num;
    }
    vector<int> Vf, Vsyn;
    for(int i=range_size << 1; i>=0;)
    {
        Vf.push_back(choose_index[i]);
        i = choose[i];
    }
    for(int i=0; i<(range_size << 1) + 1; ++i)
    {
        for(vector<int>::iterator it=Vf.begin(); it!= Vf.end(); ++it)
        {
            if(choose_index[i] == (*it))
                goto hit_;
        }
        Vsyn.push_back(choose_index[i]);
hit_:
        continue;
    }
    for(int i=0; i<Vf.size(); ++i)
    {
        for(int j=0; j<cache_size; ++j)
        {
            if(Vf[i] == Cache[j])
            {
                Vf.erase(Vf.begin()+i);
                --i;
                break;
            }
        }
    }
    cf_view += Vf.size();
    for(vector<int>::iterator it=Vsyn.begin(); it!= Vsyn.end(); ++it)
        total_synthesis += dibr_distance(*it, Vf);
    synthesis += Vsyn.size();
    hit += Vsyn.size();
}

int Huristic::get_hit()
{
    return hit;
}

int Huristic::get_synthesis()
{
    return synthesis;
}

int Huristic::get_view_request_total()
{
    return view_request_total;
}

int Huristic::get_total_synthesis()
{
    return total_synthesis;
}

int Huristic::get_cf_view()
{
    return cf_view;
}

double Huristic::dibr_distance(int curr_request, vector<int> &Vf)
{
    int left_min = view_num, left_max = -1, right_min = view_num, right_max = -1, cost1 = DIBR_range + 1, cost2 = DIBR_range + 1, cost3 = DIBR_range + 1;
    for(int j=0; j<cache_size; ++j)
    {
        //get the synthesis view for checking the DIBR_range
        if(curr_request < Cache[j])
        {
            if(Cache[j] < right_min)
                right_min = Cache[j];
            if(Cache[j] > right_max)
                right_max = Cache[j];
        }
        else
        {
            if(Cache[j] < left_min)
                left_min = Cache[j];
            if(Cache[j] > left_max)
                left_max = Cache[j];
        }
    }
    for(vector<int>::iterator it=Vf.begin(); it!=Vf.end(); ++it)
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
