#include <vector>
#include <string>
#include <iostream>

using namespace std;

#define BIT(x, pos) ((x >> pos) & 1)

vector<int> from_utf8(string s) {
    vector<int> res;
    for(int i = 0; i < s.length(); i++) {
        char p = s[i];
        if (!BIT(p, 7)) {
            res.push_back(p);
        } else if (BIT(p, 7) && BIT(p, 6) && !BIT(p, 5)) {
            int x = ((p & 31) << 6) | (s[i + 1] & 63);
            res.push_back(x);
            i++;
        } else {
            cout << "unsupported\n";
        }
    }
    return res;
}