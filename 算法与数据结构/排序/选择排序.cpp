#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
using namespace std;

class Solution {
public:
    vector<int> SelectSort(vector<int> a) {
        int n = a.size();
        for(int i=0;i<n;i++) {
            int position = i;
            for(int j=i+1;j<n;j++){
                if (a[j]<a[position]) {
                    position = j;
                }
            }
            swap(a[position],a[i]);
        }
        return a;
    }
};

int main()
{
    int t[5]={1,5,4,2,3};
    vector<int> a(t,t+5);
    Solution s;
    a = s.SelectSort(a);
    for(int i=0;i<a.size();i++)printf("%d%c",a[i],i==a.size()-1?'\n':' ');
    return 0;
}