#include "../libsponge/cycle_array.hh"

#include <iostream>
#include <queue>

using namespace std;

int main(int argc, char **argv) {
    // CycleArray c = CycleArray(3);
    // cout << c.peek_head_length() << endl;
    // c.set_at_index(0, 'a');
    // cout << c.peek_head_length() << endl;
    // c.set_at_index(1, 'b');
    // cout << c.peek_head_length() << endl;
    // cout << c.pop_head() << endl;
    // cout << c.get_head_index_in_stream() << endl;
    // cout << c.peek() << endl;
    // c.set_at_index(0, 'c');
    // cout << c.peek() << endl;
    // cout << c.get_at_index(1) << endl << "d\n";
    queue<string> q;
    string s1="123456789";
    q.push(s1);
    auto ref_s1=q.front();
    ref_s1=ref_s1.substr(0, 5);
    cout<<"ref: "<<ref_s1<<endl;
    cout<<"origin: "<<q.front()<<endl;
}