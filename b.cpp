#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <future>
#include <queue>
#include <semaphore>
#include <chrono>
using namespace std;

string f(const string& t) {
    auto res = t;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
    return res;
}

int flag = 0;
queue<string> q;
mutex mu;
binary_semaphore sem{0};

void thf() {
    string s;
    while(1) {
        sem.acquire();
        mu.lock();
        s = q.front();
        q.pop();
        mu.unlock();
        if(s == "end") {
            return;
        }
        system(s.c_str());
    }
}

int main(int argc, char** argv) {
    if(argc < 4) {
        exit(-1);
    }
    string program = argv[1];
    if(system((program + " version").c_str()) != 0) {
        printf("%s not found\n", program.c_str());
        exit(-2);
    }
    auto clocka = std::chrono::steady_clock::now();
    string srcdir = argv[2];
    string dir = argv[3];
    filesystem::create_directories(dir);
    if(dir.back() != '\\' && dir.back() != '/') {
        dir.push_back('/');
    }
    dir += "geoip_";
    vector<thread> v;
    auto threadcnt = thread::hardware_concurrency();
    threadcnt = max(1u, threadcnt);
    threadcnt *= 2;
    cout << "using " << threadcnt << " threads" << endl;
    auto clockb = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clockb - clocka).count();
    cout << "create thread begin: " << ms << "ms" << endl;
    for(int i = 0; i < threadcnt; i++) {
        v.emplace_back(thf);
    }
    auto clockbb = std::chrono::steady_clock::now();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(clockbb - clockb).count();
    cout << "create thread end: " << ms << "ms" << endl;
    // mu.lock();
    for(auto i: filesystem::directory_iterator(srcdir)) {
        string filename = i.path().filename().string();
        string com =  program + " rule-set compile --output " +  dir+f(filename)+".srs" + " " + i.path().string();
        mu.lock();
        q.push(com);
        mu.unlock();
        sem.release();
    }
    for(int i = 0; i < threadcnt; i++) {
        mu.lock();
        q.push(string("end"));
        mu.unlock();
        sem.release();
    }
    // mu.unlock();
    auto clockjoinbegin = std::chrono::steady_clock::now();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(clockjoinbegin - clockbb).count();
    cout << "thread join begin: " << ms << "ms" << endl;
    for(auto& t : v) {
        t.join();
        auto clockt = std::chrono::steady_clock::now();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(clockt - clockbb).count();
        cout << "thread join: " << ms << "ms" << endl;
    }
    auto clockc = std::chrono::steady_clock::now();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(clockc - clocka).count();
    cout << ms << "ms" << endl;
    return 0;
}
