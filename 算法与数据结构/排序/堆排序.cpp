#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
using namespace std;

class Solution {
public:
    void MaxHeapify(vector<int> &a, int position) {
        int lchild=position*2+1, rchild=lchild+1;
        int n=a.size();
        while(rchild<n){
            if(a[position]<=a[lchild]&&a[position]<=a[rchild]){
                return;
            } else if(a[lchild]<a[rchild]){
                swap(a[lchild],a[position]);
                position=lchild;
            } else {
                swap(a[rchild],a[position]);
                position=rchild;
            }
            lchild=position*2+1, rchild=lchild+1;
        }
        if(lchild<n&&a[lchild]<a[position]){
            swap(a[lchild],a[position]);
        }
    }
    vector<int> HeapSort(vector<int> a) {
        int n=a.size();
        for(int i=n-1;i>=0;i--)MaxHeapify(a,i);
        vector<int> ret;
        for(int i=n-1;i>=0;i--){
            swap(a[0],a[i]);
            ret.push_back(a[i]);
            a.pop_back();
            MaxHeapify(a,0);
        }
        return ret;
    }
};

int main()
{
    int t[5]={1,5,4,2,3};
    vector<int> a(t,t+5);
    Solution s;
    a = s.HeapSort(a);
    for(int i=0;i<a.size();i++)printf("%d%c",a[i],i==a.size()-1?'\n':' ');
    return 0;
}