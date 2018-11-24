#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
using namespace std;

class Solution {
public:
    vector<int> MergeSort(vector<int> a) {
        if (a.size() <= 1) {
            return a;
        }
        int n = a.size();
        int mid = (n-1) / 2;
        vector<int> left;
        vector<int> right;
        for(int i=0;i<n;i++) {
            if(i<=mid) {
                left.push_back(a[i]);
            } else {
                right.push_back(a[i]);
            }
        }
        left = MergeSort(left);
        right = MergeSort(right);
        vector<int> ret;
        int i=0,j=0;
        while(i<(int)left.size()||j<(int)right.size()){
            if(j>=right.size()||left[i]<right[j]){
                ret.push_back(left[i++]);
            } else {
                ret.push_back(right[j++]);
            }
        }
        return ret;
    }
};

int main()
{
    int t[5]={1,5,4,2,3};
    vector<int> a(t,t+5);
    Solution s;
    a = s.MergeSort(a);
    for(int i=0;i<a.size();i++)printf("%d%c",a[i],i==a.size()-1?'\n':' ');
    return 0;
}