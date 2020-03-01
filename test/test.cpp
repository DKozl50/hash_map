#include <bits/stdc++.h>

using namespace std;

int main () {
    vector<int> a(5, 5);
    a[1] = 4;
    a[2] = 6;
    vector<int>::iterator c_it = a.begin();
    vector<int>::const_iterator it = c_it;
    cout << "start" << endl;
    cout << *++c_it;
    cout << "end";
}
