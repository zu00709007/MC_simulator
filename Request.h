#include<stdlib.h>

class Request
{
public:
    Request(int, int, double, double);
    int request();

private:
    int request_get(int&);
    int curr_user_num;
    int view_num;
    int max_user_num;
    double enter_prob;
    double leave_prob;
    int* view;
};
