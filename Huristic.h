#include<stdio.h>
#include<limits.h>
#include<vector>
#include<algorithm>
using namespace std;

class Huristic
{
public:
    Huristic(int, int, int, int, int, double, double);
    void request_handle(int);
    int get_hit();
    int get_synthesis();
    int get_view_request_total();
    int get_total_synthesis();
    int get_cf_view();

private:
    struct Cache_node
    {
        int view_content;
        int pv;
    };
    static bool compare_sort(struct Cache_node, struct Cache_node);
    double dibr_distance(int, vector<int>&);
    int hit;
    int synthesis;
    int view_request_total;
    int total_synthesis;
    int view_num;
    int range_size;
    int cache_size;
    int DIBR_range;
    int cf;
    int cf_view;
    double a;
    double b;
    FILE* output_file2;
    int* Nt;
    int* Cache;
};
