#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
using namespace std;

class Solution {
public:
    vector<int> InsertSort(vector<int> a) {
        int n = a.size();
        for(int i=0;i<n;i++) {
            for(int j=i;j>0&&a[j]<a[j-1];j--){
                swap(a[j],a[j-1]);
            }
        }
        return a;
    }
};

int main()
{
    int t[5]={1,5,4,2,3};
    vector<int> a(t,t+5);
    Solution s;
    a = s.InsertSort(a);
    for(int i=0;i<a.size();i++)printf("%d%c",a[i],i==a.size()-1?'\n':' ');
    return 0;
}