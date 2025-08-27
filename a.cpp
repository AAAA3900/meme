#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <bitset>
#include <stack>
#include <queue>
#include <map>
#include <cstring>
#include <set>

using namespace std;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

using uchar = uint8;
using ushort = uint16;
using uint = uint32;
using ll = int64;
using ull = uint64;

struct node {
    bitset<128> ip;
    ll data;
    ll dep;
};
struct fileinfo {
    ll size;
    ll recordsize;
    ll nodecount;
    ll databegin;
    ll metabegin;
};

inline uint32 dz32(uint32 x) {
    return ((x >> 24) & 0x000000ff) | ((x >> 8) & 0x0000ff00)
            | ((x << 8) & 0x00ff0000) | ((x << 24) & 0xff000000);
}

string hex(ushort x) {
    string ret;
    for(int i = 12; i >= 0; i -= 4) {
        int t = (x >> i) & 0xf;
        if(t <= 9) {
            ret += ('0' + t);
        }
        else {
            ret += ('a' - 10 + t);
        }
    }
    return ret;
}

class mstring;
class minterface {
public:
    virtual ll size() const {return 0;}
    virtual mstring find(const mstring& t) const = 0;
};

class mstring: public minterface {
public:
    static char* buf;
    static ll index;
private:
    ll len = 0;
    char* s = 0;
public:
    mstring(): len(0), s(buf) {}
    mstring(const char* const _s, ll _len) {
        len =  _len;
        s = buf + index;
        index += len + 1;
        memcpy(s, _s, len);
        s[len] = 0;
    }
    mstring(const mstring& t): s(t.s), len(t.len) {}
    const char* const str() const {return s;}
    ll size() const {return len;}
    bool operator==(const mstring& t) const {
        return (size() == t.size()) && (strcmp(s, t.s) == 0);
    }
    bool operator==(const char* const t) const {
        return 0 == strcmp(s, t);
    }
    bool operator!=(const mstring& t) const {
        return !(*this == t);
    }
    bool operator<(const mstring& t) const {
        return (strcmp(s, t.s) < 0);
    }
    bool operator>(const mstring& t) const {
        return strcmp(s, t.s) > 0;
    }
    ~mstring() {} // 不管了，或许可以先记下来，空闲时手动清理
    mstring find(const mstring& t) const {return mstring();}
};
char* mstring::buf = nullptr;
ll mstring::index = 1;

class marray: public minterface {
public:
    map<mstring, mstring> m;
    ll size() const {return m.size();}
    mstring find(const mstring& t) const {
        if(m.count(t) == 0) {
            return mstring();
        }
        return (this->m.at(t));
    }
};

queue<int64> errors;
queue<node> datas;
ll maxdep = 0, mindep = 9999;
ll globalj;
minterface** globalmap;

void dfs(int64 u, int64 dep, const uint32* const bufu32, const fileinfo& finfo, bitset<128> ip) {
    if(u - finfo.nodecount > finfo.size - finfo.nodecount * 8) {
        errors.push(u);
        return;
    }
    if(u >= finfo.nodecount + 16) {
        node node;
        node.ip = ip;
        node.dep = dep;
        node.data = u - finfo.nodecount + finfo.nodecount * 8;
        datas.push(node);
        if(dep > maxdep) {
            maxdep = dep;
        }
        if(dep < mindep) {
            mindep = dep;
        }
        return;
    }
    if(u > finfo.nodecount) {
        printf("%lld\t%lld\t0~16\n", dep, u);
        return;
    }
    if(u == finfo.nodecount) {
        return;
    }
    int64 left = dz32(bufu32[u * 2]);
    int64 right = dz32(bufu32[u * 2 + 1]);
    ip[127 - dep] = 0;
    dfs(left, dep + 1, bufu32, finfo, ip);
    ip[127 - dep] = 1;
    dfs(right, dep + 1, bufu32, finfo, ip);
}

minterface* jx(const ll u, ll& endpoint, const uchar* const buf, const fileinfo& finfo) {
    if(globalmap[u] != nullptr && globalmap[u]->size() != 0) {
        return globalmap[u];
    }
    ull type = (buf[u] >> 5) & 0x07;
    ull ctl = buf[u] & 0x1f;
    if(type == 1) { // point 001 00 000
        ll next = 0;
        ull a = (ctl >> 3) & 0x03, b = ctl & 0x07;
        if(a == 0) {
            next = (b << 8) + (buf[u + 1]);
            endpoint = u + 2;
        }
        else if(a == 1) {
            next = (b << 16) + ((ull)buf[u + 1] << 8) + (buf[u + 2]) + 2048;
            endpoint = u + 3;
        }
        else if(a == 2) {
            next = (b << 24) + ((ull)buf[u + 1] << 16) + ((ull)buf[u + 2] << 8) + (buf[u + 3]) + 526336;
            endpoint = u + 4;
        }
        else if(a == 3) {
            next = ((ull)buf[u + 1] << 24) + ((ull)buf[u + 2] << 16) + ((ull)buf[u + 3] << 8) + (buf[u + 4]);
            endpoint = u + 5;
        }
        next += finfo.databegin;
        ll retendpoint;
        if(globalmap[next] == nullptr
        || globalmap[next]->size() == 0) {
            globalmap[next] = jx(next, retendpoint, buf, finfo);
        }
        return globalmap[u] = globalmap[next];
    }
    ll len = 0;
    ll v = u;
    if(ctl < 29) {
        len = ctl;
        v += 1;
    }
    else if(ctl == 29) {
        len = ctl + buf[u + 1];
        v += 2;
    }
    else if(ctl == 30) {
        len = ctl + (buf[u + 1] << 8) + buf[u + 2];
        v += 3;
    }
    else if(ctl == 31) {
        len = ctl + (buf[u + 1] << 16) + (buf[u + 2] << 8) + buf[u + 3];
        v += 4;
    }
    if(type == 2) { // utf8 string 010 00000
        endpoint = v + len;
        return globalmap[u] = new mstring((const char* const)buf + v, len);
    }
    else if(type == 7) { // map 111 00000
        marray* array = new marray();
        for(ll i = 0; i < len; i++) {
            mstring* a = (mstring*)jx(v, v, buf, finfo);
            mstring* b = (mstring*)jx(v, v, buf, finfo);
            array->m[*a] = *b;
        }
        endpoint = v;
        return globalmap[u] = array;
    }
    printf("<<< %lld, %lld, u=%lld, i=%lld >>>\n", type, ctl, u, globalj);
    return nullptr;
}

int main(int argc, char** argv) {
    if(argc < 3) {
        exit(-1);
    }
    string filename = argv[1];
    string dir = argv[2];
    if(dir.back() != '\\' && dir.back() != '/') {
        dir.push_back('/');
    }
    _setmaxstdio(2048);
    printf("_getmaxstdio() = %lld\n", _getmaxstdio());
    auto clockbegin = clock();
    fileinfo finfo;
    FILE* fp = fopen(filename.c_str(), "rb");
    if(fp == NULL) {
        printf("fopen file error");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    int64 len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8* buf = (uint8*)malloc(len + 1024);
    auto ret = fread(buf, len, 1, fp);
    if(ret == 0) {
        printf("read file error\n");
        fclose(fp);
        exit(2);
    }
    fclose(fp);
    auto clockreadfile = clock();

    finfo.size = len;
    uint32* bufu32 = (uint32*)buf;
    const uint8* end = (const uint8*)("\xab\xcd\xefMaxMind.com");
    int64 endp = 0;
    for(int64 i = 0, j; i + 14 < len; i++) {
        for(j = 0; j < 14; j++) {
            if(buf[i + j] != end[j]) {
                break;
            }
        }
        if(j == 14) {
            endp = i + 14;
            // break; 不要break ， 因为可能有多个匹配成功的位置，需要取最后一个
        }
    }
    if(endp == 0) {
        printf("\"\\xab\\xcd\\xefMaxMind.com\"\n");
        exit(3);
    }
    finfo.metabegin = endp;

    const uint8* ssize = (const uint8*)("record_size");
    const uint8* scount = (const uint8*)("node_count");
    for(ll i = finfo.metabegin, j; i < finfo.size; i++) {
        for(j = 0; j < 11; j++) {
            if(buf[i + j] != ssize[j]) {
                break;
            }
        }
        if(j == 11) {
            finfo.recordsize = buf[i + j + 1];
        }
        for(j = 0; j < 10; j++) {
            if(buf[i + j] != scount[j]) {
                break;
            }
        }
        if(j == 10) {
            finfo.nodecount = ((ll)buf[i+j+1]<<16) + ((ll)buf[i+j+2]<<8) + ((ll)buf[i+j+3]);
        }
    }
    finfo.databegin = finfo.nodecount * finfo.recordsize * 2 / 8 + 16;

    if(finfo.recordsize != 32) {
        printf("finfo.recordsize != 32, is %lld\n", ll(finfo.recordsize));
        exit(999);
    }
    auto clockreadmeta = clock();

    dfs(0, 0, bufu32, finfo, bitset<128>{0});
    printf("error count = %lld\n", errors.size());
    printf("data count = %lld\n", datas.size());
    printf("maxdep = %lld, \t mindep = %lld\n", maxdep, mindep);
    auto clockdfs = clock();

    bitset<128> ipa("11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111100000000000000000000000000000000");
    bitset<128> ipb("00000000000000000000000000000000000000000000000000000000000000000000000000000000111111111111111100000000000000000000000000000000");
    bitset<128> ipc("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011111111111111111111111111111111");

    mstring::buf = (char*)malloc((finfo.metabegin - finfo.databegin) * sizeof(char));
    buf[0] = 0;
    globalmap = (minterface**)malloc((finfo.size - 0) * sizeof(minterface*));
    memset(globalmap, 0, (finfo.size - 0) * sizeof(minterface*));
    map<mstring, FILE*> countrycode;
    mstring sccode = mstring("country_code", 12);
    mstring sasn = mstring("asn", 3);
    mstring sasname = mstring("as_name", 7);
    FILE* filegoogle = fopen((dir + "google").c_str(), "wb+");
    FILE* filecloudflare = fopen((dir + "cloudflare").c_str(), "wb+");
    FILE* filetelegram = fopen((dir + "telegram").c_str(), "wb+");
    fprintf(filegoogle,"{\"version\": 2,\"rules\": [{\"ip_cidr\": [");
    fprintf(filecloudflare, "{\"version\": 2,\"rules\": [{\"ip_cidr\": [");
    fprintf(filetelegram, "{\"version\": 2,\"rules\": [{\"ip_cidr\": [");
    char abuf[1000];

    for(ll i = 0; datas.size() > 0 && i < 50000000000; i++) {
        node t = datas.front();
        datas.pop();
        ll endpoint;
        globalj = i;
        if(globalmap[t.data] == NULL || globalmap[t.data]->size() == 0) {
            globalmap[t.data] = jx(t.data, endpoint, buf, finfo);
        }
        mstring tccode = globalmap[t.data]->find(sccode);
        if(tccode.size() == 0) {
            printf("tccode.size() == 0\n");
            continue;
        }
        if(countrycode[tccode] == nullptr) {
//            countrycode[tccode] = stdout;
            countrycode[tccode] = fopen((dir + tccode.str()).c_str(), "wb+");
            fprintf(countrycode[tccode], "{\"version\": 2,\"rules\": [{\"ip_cidr\": [");
        }
//        fprintf(stdout, "%10lld %s ", t.data, tccode.str());
        if(t.dep >= 96 && (t.ip & ipa) == 0) {
            ull ip = t.ip.to_ullong();
            sprintf(abuf, "\"%lld.%lld.%lld.%lld/%lld\",",
                    (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff, t.dep - 96
                    );
        }
        else {
            auto ip = t.ip;
            sprintf(abuf, "\"%04llx:%04llx:%04llx:%04llx:%04llx:%04llx:%04llx:%04llx/%lld\",",
                    ((ip >> 112) & bitset<128>(0xffff)).to_ullong(), ((ip >> 96) & bitset<128>(0xffff)).to_ullong(),
                    ((ip >> 80)  & bitset<128>(0xffff)).to_ullong(), ((ip >> 64) & bitset<128>(0xffff)).to_ullong(),
                    ((ip >> 48)  & bitset<128>(0xffff)).to_ullong(), ((ip >> 32) & bitset<128>(0xffff)).to_ullong(),
                    ((ip >> 16)  & bitset<128>(0xffff)).to_ullong(), ((ip >> 0)  & bitset<128>(0xffff)).to_ullong(),
                    t.dep
                    );
        }
        fprintf(countrycode[tccode], abuf);
        auto tasname = globalmap[t.data]->find(sasname);
        if(strstr(tasname.str(), "Google") != nullptr) {
            fprintf(filegoogle, abuf);
        }
        else if(strstr(tasname.str(), "Cloudflare") != nullptr) {
            fprintf(filecloudflare, abuf);
        }
        else if(strstr(tasname.str(), "Telegram") != nullptr) {
            fprintf(filetelegram, abuf);
        }
    }
    fseek(filegoogle, -1, SEEK_END);
    fprintf(filegoogle, "]}]}");
    fseek(filecloudflare, -1, SEEK_END);
    fprintf(filecloudflare, "]}]}");
    fseek(filetelegram, -1, SEEK_END);
    fprintf(filetelegram, "]}]}");
    for(auto i: countrycode) {
        fseek(i.second, -1, SEEK_END);
        fprintf(i.second, "]}]}");
    }
    auto clockreaddata = clock();
    printf("%lld\n", clockreaddata - clockbegin);
    cout << mstring::index << endl;
    printf("clock\n readfile:%lld\n readmeta:%lld\n dfs:%lld\n readdata:%lld\n",
           clockreadfile - clockbegin, clockreadmeta - clockreadfile, clockdfs - clockreadmeta, clockreaddata - clockdfs);
//    cin >> ret;
    return 0;
}
