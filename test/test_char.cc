#include "test_char.hh"

using namespace std;
int main(int argc, char** argv){
    // Test t;
    // char* c_array=new char[10];
    // cout<<c_array[0]<<endl;
    // return 0;
    char* test=new char[5];
    test[0]='1';
    test[1]='2';
    test[2]='3';
    test[3]='4';
    test[4]='5';
    string str=string(test+2, test+5);
    test[2]='x';
    string str2=string(test+2, test+5);
    cout<<str<<endl<<str2<<endl;
}

Test::Test(){
    this->q.push(1);
    cout<<"test"<<endl;
}