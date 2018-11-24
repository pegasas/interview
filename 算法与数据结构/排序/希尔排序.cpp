#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
using namespace std;

class Solution {
public:
    vector<int> HillSort(vector<int> a) {
        int n = a.size();
        for(int gap=n/2;gap>0;gap/=2) {
            for(int i=0;i<gap;i++) {
                for(int j=i;j<n;j+=gap) {
                    int value = a[j];
                    int k;
                    for(k=j-gap;k>=0&&a[k]>value;k-=gap){
                        a[k+gap]=a[k];
                    }
                    a[k+gap]=value;
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
    a = s.HillSort(a);
    for(int i=0;i<a.size();i++)printf("%d%c",a[i],i==a.size()-1?'\n':' ');
    return 0;
}