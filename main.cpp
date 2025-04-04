#include <iostream>
#include "api.cpp"

using namespace std;

int main() {
    auto status = words_wrapper();
    while (true) {
        for (const auto &w: status.words) {
            cout << w << endl;
        }
        cout << status.nextTurnSec << endl;

        sleep(status.nextTurnSec);
        status = words_wrapper();
    }
}
