#include"json.hpp"

using json = nlohmann::json;

#include<iostream>
#include<vector>
#include<map>
#include<string>
using namespace std;

//json序列化事例1
void func1(){
    json js;
    js["msg_typr"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now";
    js["id"] = {1,2,3,4,5};
    // js["msg"]["zhang san"] = "hello world";
    // js["msg"]["li si"] = "hello china";

    string sendbuf = js.dump();  //数据序列化后的东西给sendbuf
    cout<<js<<endl;
    cout << sendbuf<<endl;  //sendbuf和sendbuf.c_str()有啥区别


}

//json序列化事例2
void func2(){
        json js;
    js["msg_typr"] = 2;
    js["name"] = "zhang san";
    js["id"] = {1,2,3,4,5};

    js["msg"]["zhang san"] = "hello world";
    js["msg"]["li si"] = "hello china";
    //上面两句等同于
    js["msg"] ={{"zhang san","hello world"},{"li si","hello china"}};

    string sendbuf = js.dump();  //数据序列化后的东西给sendbuf
    cout<<js<<endl;
    cout << sendbuf<<endl;  //sendbuf和sendbuf.c_str()有啥区别

}

//json序列化事例3 容器事例化
void func3(){


    json js;

    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    js["list"] = vec;

    map<int,string> m;
    m.insert({1,"黄山"});
    m.insert({2,"泰山"});
    m.insert({3,"华山"});

    js["path"] = m;


    string sendbuf = js.dump(); // json数据对象=》序列化json字符串
    cout<<js<<endl;


}


string func4(){
    json js;
    js["msg_typr"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now";
    js["id"] = {1,2,3,4,5};
    // js["msg"]["zhang san"] = "hello world";
    // js["msg"]["li si"] = "hello china";

    string sendbuf = js.dump();  //数据序列化后的东西给sendbuf
    // cout<<js<<endl;
    // cout << sendbuf<<endl;  //sendbuf和sendbuf.c_str()有啥区别
    return sendbuf;

}


int main(){
    func1();
    func2();
    func3();
    string recvbuf = func4();
    json jsbuf = json::parse(recvbuf);

    cout<<jsbuf<<endl;
    return 0;
}