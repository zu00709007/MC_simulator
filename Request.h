#include<stdio.h>
#include<stdlib.h>

class Request
{
public:
    Request(int, int, double, double, int, double, double, int);

private:
    int request(FILE*);
    int request_get(int&, FILE*);
    int curr_user_num;
    int view_num;
    int max_user_num;
    int probability_setting;
    double enter_prob;
    double leave_prob;
    int* view;
    double zipf_left;
    double zipf_right;
};
