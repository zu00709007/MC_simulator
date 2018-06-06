#include "Request.h"

Request::Request(int view_num, int max_user_num, double enter_prob, double leave_prob, int probability_setting, double zipf_left, double zipf_right, int request_num) : view_num(view_num), max_user_num(max_user_num), enter_prob(enter_prob), leave_prob(leave_prob + enter_prob), probability_setting(probability_setting), zipf_left(zipf_left), zipf_right(zipf_left + zipf_right)
{
    curr_user_num = 0;
    view = new int[view_num];
    for(int i=0; i<view_num; ++i)
        view[i] = 0;

    FILE* output_file;
    output_file = fopen("request_seed", "w");
    for(int i=0; i<request_num-1; ++i)
        fprintf(output_file, "%d\n", request());
    fprintf(output_file, "%d", request());
    fclose(output_file);
}

int Request::request()
{
    //get the request until success
    int curr_request;
    while(!request_get(curr_request));
    return curr_request;
}

int Request::request_get(int& curr_request)
{
    double rand_request = (double)rand() / ((double)RAND_MAX + 1);
    int view_tmp[view_num], j;

    if(rand_request < enter_prob)
    {
        //if cache is full, the entering event is failed
        if(curr_user_num >= max_user_num)
            return 0;

        ++curr_user_num;
        j = (int)(view_num * (double)rand() / ((double)RAND_MAX + 1));
        ++view[j];
        curr_request = j;
    }
    else if(rand_request < leave_prob)
    {
        //if cache is empty, the leaving event is failed
        if(0 >= curr_user_num)
            return 0;

        //set the probability table here, we use CDF to get calculation
        view_tmp[0] = view[0];
        for(j=1; j<view_num; ++j)
            view_tmp[j] = view_tmp[j-1] + view[j];

        //choose the leaving view angle
        int view_choose = (int)(curr_user_num * (double)rand() / ((double)RAND_MAX + 1));
        for(j=0; j<view_num; ++j)
            if(view_choose < view_tmp[j])
            {
                --curr_user_num;
                --view[j];
                curr_request = j;
                break;
            }
    }
    else
    {
        //if cache is empty, the staying event is failed
        if(0 >= curr_user_num)
            return 0;

        //set the probability table here, we use CDF to get calculation
        view_tmp[0] = view[0];
        for(j=1; j<view_num; ++j)
            view_tmp[j] = view_tmp[j-1] + view[j];

        //choose the leaving view angle
        int view_choose = (int)(curr_user_num * (double)rand() / ((double)RAND_MAX + 1));
        for(j=0; j<view_num; ++j)
            if(view_choose < view_tmp[j])
            {
                //choose the left, center or right view in uniform
                if(0 == probability_setting)
                    switch((int)(3*(double)rand()/((double)RAND_MAX+1)))
                    {
                    case 0:
                        --view[j];
                        ++view[(j+view_num-1)%view_num];
                        curr_request = (j + view_num - 1) % view_num;
                        break;
                    case 1:
                        curr_request = j;
                        break;
                    case 2:
                        --view[j];
                        ++view[(j+1)%view_num];
                        curr_request = (j + 1) % view_num;
                        break;
                    }
                //choose the left, center or right view in zipf
                else
                {
                    double tmp = (double)rand()/((double)RAND_MAX+1);
                    if(tmp < zipf_left)
                    {
                        --view[j];
                        ++view[(j+view_num-1)%view_num];
                        curr_request = (j + view_num - 1) % view_num;
                    }
                    else if(tmp < zipf_right)
                    {
                        --view[j];
                        ++view[(j+1)%view_num];
                        curr_request = (j + 1) % view_num;
                    }
                    else
                        curr_request = j;
                }
                break;
            }
    }
    return 1;
}
