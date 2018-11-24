#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
using namespace std;

class Solution {
public:
    vector<int> QuickSort(vector<int> a) {
        if (a.size()<=1) return a;

        srand((unsigned)time(NULL));
        int n=a.size();
        int position=rand()%n;
        swap(a[position],a[n-1]);
        vector<int> left;
        vector<int> right;
        for(int i=0;i<n-1;i++) {
            if (a[i]<a[n-1])left.push_back(a[i]);
            else right.push_back(a[i]);
        }

        left = QuickSort(left);
        right = QuickSort(right);

        vector<int> ret;
        for(int i=0;i<left.size();i++)ret.push_back(left[i]);
        ret.push_back(a[n-1]);
        for(int i=0;i<right.size();i++)ret.push_back(right[i]);
        a.swap(ret);
        return a;
    }
};

int main()
{
    int t[5]={1,5,4,2,3};
    vector<int> a(t,t+5);
    Solution s;
    a = s.QuickSort(a);
    for(int i=0;i<a.size();i++)printf("%d%c",a[i],i==a.size()-1?'\n':' ');
    return 0;
}