#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
using namespace std;

class Solution {
public:
    vector<int> BubbleSort(vector<int> a) {
        int n = a.size();
        for(int i=0;i<n;i++) {
            int position = i;
            for(int j=i+1;j<n;j++){
                if (a[j-1]>a[j]) {
                    swap(a[j-1],a[j]);
                }
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
    a = s.BubbleSort(a);
    for(int i=0;i<a.size();i++)printf("%d%c",a[i],i==a.size()-1?'\n':' ');
    return 0;
}