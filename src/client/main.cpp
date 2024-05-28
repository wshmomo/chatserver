#include "json.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
#include<ctime>

using namespace std;
using json = nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include "group.hpp"

#include"user.hpp"

#include "public.hpp"


//记录当前系统登录的用户信息
User g_currentUser;
//记录当前登录用户的好友列表
vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表
vector<Group> g_currentUserGroupList;
//显示当前登录成功用户的基本信息
void showCurrentUserData();

//控制聊天页面程序
bool isMainMenuRunning = false; //这里主要是为了loginout设计的，将下面的（MainMenu）这块的for(;;)改一下while(isMainMenuRunning)

//接收线程
void readTaskHandler(int clientfd);

//获取系统时间（聊天信息需要添加时间信息）
string  getCurrentTime();  //用的C++11的一个方法

//主聊天页面程序
void mainMenu(int clientfd);



//聊天客户端程序实现， main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv){
    if(argc < 3){
        cerr <<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    //解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1){
        cerr <<"socket create error"<<endl;
        exit(-1);
    }

    //填写client需要链接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET,ip,&server.sin_addr.s_addr);
    
    //client和server进行连接
    if(-1 == connect(clientfd, (sockaddr *)&server,sizeof(server))){
        cerr << "connect server error"<<endl;
        close(clientfd);
        exit(-1);

    }
    int threadnumber = 0;
    
    // main线程用于接收用户输入，负责发送数据
    for(;;){
        //显示首页菜单登录、注册、退出
        cout<<"==========================="<<endl;
        cout<<"1.login"<<endl;
        cout<<"2.register"<<endl;
        cout<<"3.quit"<<endl;
        cout<<"==========================="<<endl;
        cout<<"choice";
        int choice =0;
        cin >> choice;
        cin.get();   //读掉缓冲区残留的回车,防止整数后面跟着字符串，出现错误

        switch(choice){
            case 1:   //1.Login
            {
                int id = 0;
                char pwd[50] = {0};
                cout<<"userid:";
                cin>>id;
                cin.get();
                cout<<"userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                if(len == -1){
                    cerr << "send login msg error:" <<request << endl;
                }else{
                    char buffer[1024] = {0};
                    len = recv(clientfd,buffer,1024,0);
                    if(len == -1){
                        cerr << "recv log response error" <<endl;
                    }else{
                        json responsejs = json::parse(buffer);
                        if(responsejs["erron"].get<int>() != 0 ){
                            cerr << responsejs["errmsg"] << endl;
                        }else{

                            //记录当前用户的消息
                            g_currentUser.setID(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);

                            //记录当前用户的好友列表信息
                            if(responsejs.contains("friends")){
                                
                                g_currentUserFriendList.clear();  //初始化操作，避免出现在loginout后，再次登入出现上次的信息
                                map<string, string> friends  = responsejs["friends"];
                                for(const auto & pair : friends){
                                    User user;
                                    user.setName(pair.first);
                                    user.setState(pair.second);
                                    g_currentUserFriendList.push_back(user);

                                }

                            }


                            //记录当前用户的群列表
                            if(responsejs.contains("group")){
                                g_currentUserGroupList.clear(); // 初始化，避免上次信息再次出现，同上

                                vector<string> vec = responsejs["group"];
                                for(const auto & i : vec){
                                    Group group;
                                    json js = json::parse(i);
                                    vector<string> userV = js["user"];
                                    group.setId(js["groupid"]);                    
                                    group.setName(js["groupname"]);                    
                                    group.setDesc(js["groupdesc"]);
                                    for(const auto & useri : userV){
                                        GroupUser user;
                                        json userjs = json::parse(useri);
                                        user.setID(userjs["id"].get<int>());                         
                                        user.setName(userjs["name"]);
                                        user.setState(userjs["state"]);
                                        user.setRole(userjs["role"]);
                                        group.getUser().push_back(user);  //这句话开始不会写，现在会了
                                    }
                                    g_currentUserGroupList.push_back(group);
                                    

                                }
                            }

                            //显示登录用户的信息
                            showCurrentUserData();

                            //显示当前用户的离线消息 个人聊天信息或者群组信息
                            if(responsejs.contains("offlinemsg")){
                                vector<string> vecMsg = responsejs["offlinemsg"];
                                for(const auto& msg : vecMsg){
                                    json js = json::parse(msg);
                                    if(js["msgid"].get<int>() == ONE_CHAT_MSG){
                                        cout << js["time"].get<string>() << " [" << js["id"] << "]" <<js["name"].get<string>()<< "said: " << js["msg"].get<string>() <<endl; 
                                    }

                                    
                                    else if(js["msgid"].get<int>() == GROUP_CHAT_MSG){
                                        cout << "群消息["<<js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() <<" said: " << js["msg"].get<string>() <<endl;
                                    }

                                }
                            }

                            //注意loginout之后，他的相关线程也需要结束，要不然就占据线程了，所以这里我们需要保证clientfd对应的线程只启动一次
                            
                            if(threadnumber == 0){
                                //登录成功，启动接收线程负责接收数据  C++11提供的线程库, std::thread readTask()可任意传参数，线程tid,线程安全属性的结构体信息，线程函数
                                thread readTask(readTaskHandler,clientfd);  //相当于pthread_create()一样
                                readTask.detach();  //相当于pthread_detach()一样，设置分离线程，线程运行完了，内核空间会自动回收，避免内存泄漏
                                threadnumber++;
                            }   

                            //如何登出之后，让这个线程也结束，重新开辟一个线程
                            //这里的解决方案是让这个线程只起一次，也就是说，你登出之后，再次登入，还是同一个线程在运行

                            //有个问题为什么最开始的时候，如果开始登出一个账号，然后再有一个其他的账号登入，这里没什么事情呢？

                            //进入聊天主菜单页面
                            isMainMenuRunning = true;

                            mainMenu(clientfd);


                        }
                    }
                }
                
            }
            break;
            case 2: //2.register 注册业务
            {

                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username:";
                cin.getline(name,50);
                cout << "userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"] =REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd,request.c_str(),strlen(request.c_str())+1,0);

                if(len == -1){
                    cerr << "send reg msg error:" << request << endl;
                }else{
                    char buffer[1024] = {0};
                    len = recv(clientfd,buffer,1024,0);
                    if(len == -1){
                        cerr << "recv reg response error" <<endl;
                    }else{
                        json responsejs = json::parse(buffer);
                        if(responsejs["erron"].get<int>() != 0){
                            cerr << name << "is already exist, register error!" <<endl;
                        }else{
                            cout << name << "register success, userid is" << responsejs["id"] << ", do noy forget it!" <<endl;
                        }

                    }
                }

            }
            break; //因为你这个已经结束了，所以需要跳出循环了
            case 3: // quit业务
                close(clientfd);
                exit(0);
            default:
                cerr << "invalid input!" <<endl;
                break;   //输入错误需要跳出这个case，然后继续在for循环中去继续输入choice的值看处理什么业务
        }
    }

    return 0;





}


void readTaskHandler(int clientfd){
    for(;;){
        char buffer[1024] ={0};
        int len = recv(clientfd, buffer,1024,0);  //意味着 如果登出，那么再次登入，将会堵塞在这里，所以我们必须保证该线程只会被启动一次，所以上面那块需要修改
        if(len == -1 || len ==0){
            close(clientfd);
            exit(-1);
        }

        //接受ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        if(js["msgid"].get<int>() == ONE_CHAT_MSG){
            cout << js["time"].get<string>() << " [" << js["id"] << "]" <<js["name"].get<string>()<< "said: " << js["msg"].get<string>() <<endl; 
            continue;
        }
        
        else if(js["msgid"].get<int>() == GROUP_CHAT_MSG){
            cout << "群消息["<<js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() <<" said: " << js["msg"].get<string>() <<endl;
            continue;
        }
    }


}

//" help command Handler "
void help(int fd = 0, string str = "");

//" chat command Handler "
void chat(int, string);

//" addfriend command Handler "
void addfriend(int, string);

//" creategroup command Handler "
void creategroup(int, string);

//" addgroup command Handler "
void addgroup(int, string);

//" groupchat command Handler "
void groupchat(int, string);

//" loginout command Handler "
void loginout(int, string);

//系统支持的客户端命令列表
unordered_map<string,string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}

};


//注册系统支持的客户端命令处理
unordered_map<string, function<void(int,string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}

};

void mainMenu(int clientfd){
    help();

    char buffer[1024] = {0};
    while(isMainMenuRunning){
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":");   //find会从第一个位置进行查找，比如string str ="abcda" str.find(c)返回的就是2
        if(idx == -1){

            command = commandbuf;

        }else{
            command = commandbuf.substr(0,idx);
        }

        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()){
            cerr << "invalid input command!" <<endl;
            continue;
        }

        //调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size() - idx)); // 调用命令处理方法
    }


}

//“help" command handler
void help(int,string){
    cout << "show commandlist >>>" << endl;
    for(auto &p : commandMap){
        cout << p.first <<" : " << p.second <<endl;
    }
    cout << endl;
}


void addfriend(int clientfd, string str){
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MES;
    js["id"] = g_currentUser.getID(); 
    js["friendid"] = friendid;
    string buffer = js.dump();
    
    int len = send(clientfd, buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1){
        cerr << "send addfriend msg error -> " << buffer <<endl;
    }
}

void chat(int clientfd, string str){
    int idx = str.find(":");
    if(idx == -1){
        cerr << "chat command invalid!" << endl;
    }else{
        // string s = str.substr(0,idx);
        // int toid = atoi(s.c_str());
        json js;
        js["msgid"] = ONE_CHAT_MSG;
        js["id"] = g_currentUser.getID();
        js["name"] = g_currentUser.getName();
        js["to"] = atoi((str.substr(0,idx)).c_str());  //那最开始究竟因为什么出错
        js["msg"] = str.substr(idx+1, str.size() - idx);
        js["time"] = getCurrentTime();
        string buffer = js.dump();

        int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1,0);
        if(len == -1){
            cerr << "send chat msg error -> " <<buffer <<endl;
        }
    }

}



//" creategroup command Handler "
void creategroup(int clientfd, string str){

    int idx = str.find(":");
    if(idx == -1){
        cerr << "creategroup command invalid!" <<endl;
    }else{    
        json js;
        js["msgid"] = GREATE_GROUP_MSG;
        js["id"] = g_currentUser.getID(); 
        js["groupname"] = str.substr(0,idx);
        js["groupdesc"] = str.substr(idx+1, str.size() - idx);
        string buffer = js.dump();

        int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
        if(len == -1){
            cerr << "send creategroup msg error ->" << buffer << endl;
        }
    }

}

//" addgroup command Handler "
void addgroup(int clientfd, string str){
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getID();
    js["groupid"] = atoi(str.c_str());
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1){
        cerr << "send addgroup msg error ->" << buffer <<endl;
    }

}

//" groupchat command Handler "
void groupchat(int clientfd, string str){
    int idx = str.find(":");
    if(idx == -1){
        cerr << "groupchat command valid!" <<endl;
    }else{

        string s = str.substr(0,idx);
        int gid = atoi(s.c_str());   //最开始出问题就是直接把js["groupid"] = atoi(s.c_str());
        json js;
        js["msgid"] = GROUP_CHAT_MSG;
        js["id"] = g_currentUser.getID();
        js["name"] =g_currentUser.getName();
        js["groupid"] = gid;
        js["msg"] = str.substr(idx+1, str.size()-idx);
        js["time"] = getCurrentTime();
        string buffer = js.dump();
        int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
        if(len == -1){
            cerr << "send groupchat msg error ->" <<buffer <<endl;
        }
        
    }

}

//" loginout command Handler "
void loginout(int clientfd, string){
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getID();
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1){
        cerr << "send loginout msg error ->" <<buffer <<endl;
    }
    else{
        isMainMenuRunning = false;
    }


}



//显示当前登录成功用户的基本信息
void showCurrentUserData(){
    cout<<"================================login user================================"<<endl;
    cout<<"current login user => id:"<<g_currentUser.getID()<<" "<<"name:"<<g_currentUser.getName()<<endl;
    cout<<"--------------------------------friend list-------------------------------"<<endl;
    if(!g_currentUserFriendList.empty()){
        for(User &user : g_currentUserFriendList){
            cout<<user.getID()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }

    cout<<"--------------------------------group list--------------------------------"<<endl;
    if(!g_currentUserGroupList.empty()){
        for(Group &group : g_currentUserGroupList){
            cout <<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user : group.getUser()){
                cout<<user.getID()<<" "<<user.getName()<<" "<<user.getState()<<" "<<user.getRole()<<endl;
            }
        }
        
    }
    cout<<"=========================================================================="<<endl;

}

string  getCurrentTime(){

    // time_t timep;
    // time(&timep); //获取从1970至今过了多少秒，存入time_t类型的timep
    // return ctime(&timep);
    // // printf("%s", ctime(&timep));//用ctime将秒数转化成字符串格式，输出：Thu Feb 28 14:14:17 2019

    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year+1900,(int)ptm->tm_mon+1, (int)ptm->tm_mday,
            (int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec);

    return std::string(date);

}