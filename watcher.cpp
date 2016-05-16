#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <regex>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <cassert>
#include "json.hpp"

namespace Infos
{
    class MemInfoGetter
    {
        private:
            struct sysinfo memInfo;
            unsigned long long getTot()
            {
                sysinfo(&memInfo);
                return (memInfo.totalram) * memInfo.mem_unit;
            }

            unsigned long long getUsed()
            {
                sysinfo(&memInfo);
                return 
                    (memInfo.totalram - memInfo.freeram - memInfo.bufferram) * memInfo.mem_unit;
            }

            unsigned long long getRealUsed()
            {
                std::ifstream is("/proc/meminfo");
                unsigned long long res = getUsed();
                while (!is.eof())
                {
                    std::string fieldname;
                    is >> fieldname;
                    if (fieldname == "Cached:")
                    {
                        unsigned long long tmp;
                        is >> tmp;
                        return res - tmp*1024;
                    }
                }
                return -1;
            }

            double getLoad()
            {
                sysinfo(&memInfo);
                return memInfo.loads[0];
            }

        public:
            struct MemInfo
            {
                unsigned long long tot, used, realUsed;
                double load;
            };
            MemInfo getMemInfo()
            {
                return MemInfo { getTot(), getUsed(), getRealUsed(), getLoad() };
            }
    };

    class CPUInfoGetter
    {
        private:
            unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

        public:
            CPUInfoGetter(){
                std::ifstream is("/proc/stat");
                std::string dummy_cpu;
                is >> dummy_cpu;
                is >> lastTotalUser >> lastTotalUserLow >> lastTotalSys >> lastTotalIdle;
            }

            double getCurrentValue(){
                double percent;
                unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;
                using namespace std::chrono;
                std::this_thread::sleep_for(500ms);
                std::string dummy_cpu;
                std::ifstream is("/proc/stat");
                is >> dummy_cpu;
                is >> totalUser >> totalUserLow >> totalSys >> totalIdle;

                if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
                            totalSys < lastTotalSys || totalIdle < lastTotalIdle){
                    percent = -1.0;
                }
                else{
                    total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
                        (totalSys - lastTotalSys);
                    percent = total;
                    total += (totalIdle - lastTotalIdle);
                    percent /= total;
                    percent *= 100;
                }


            lastTotalUser = totalUser;
            lastTotalUserLow = totalUserLow;
            lastTotalSys = totalSys;
            lastTotalIdle = totalIdle;
            return percent;
        }
    };

    struct DiskSpaceInfo
    {
        // sizes are in bytes. Caution that total != used + avail
        std::size_t total, used, avail;
    };

    class DiskSpaceGetter
    {
        private:
            std::string _path;

        public:
            explicit DiskSpaceGetter(const std::string &path):
                _path(path)
            {
            }

            DiskSpaceInfo getDiskSpace()
            {
                struct statvfs stat; // c struct in posix
                DiskSpaceInfo result;
                int statres = statvfs(_path.c_str(), &stat);
                if (statres == -1)
                    return {};
                result.total = stat.f_frsize * stat.f_blocks;
                result.used = result.total - stat.f_bsize * stat.f_bfree;
                result.avail = stat.f_bsize * stat.f_bavail;
                return result;
            }
    };

    class NginxLogInfo
    {
        private:
            const std::string filename;
            std::ifstream fs;
        public:
            explicit NginxLogInfo(const std::string &s):
                filename(s), fs(s)
        {
        }

            std::vector<std::string> getIncrLines()
            {
                fs.sync();
                fs.clear();
                std::string buffer;
                std::vector<std::string> v;
                while (!fs.eof())
                {
                    std::getline(fs, buffer);
                    if (buffer.length()>1)
                        v.push_back(buffer);
                }
                return v;
            }
                    
    };

}

struct NginxLogEntry
{
    typedef std::chrono::system_clock::time_point ClockT;
    ClockT time_point;
    std::string path;
    int status;
    std::string type;
};


class NginxLogEntryParser
{
    static const std::regex
        rx ;
    public:
    NginxLogEntry parse(const std::string& s)
    {
        std::smatch m;
        NginxLogEntry e;
        if (std::regex_search(s, m, rx))
        {
            e.status = std::atoi(m.format("$5").c_str()); // 200
            e.type = m.format("$2"); //POST
            e.path = m.format("$3");
            std::tm t = {};

            std::stringstream ss(m.format("$1"));
            ss >> std::get_time(
                        &t,
                        "%d/%b/%Y:%H:%M:%S");

            std::time_t tt = std::mktime(&t);
            e.time_point = std::chrono::system_clock::from_time_t(tt);
            return e;


        } else 
            std::cout << "Not found!" << std::endl;
        return NginxLogEntry();
    }
};

const std::regex NginxLogEntryParser::rx { R"regex(\[(.*)\] "(\S+) (\S+) (\S+)" ([0-9]+))regex" };
    

class Status
{
    private:
        typedef std::chrono::system_clock::time_point ClockT;
        //! Change the maximum entries here!
        //! The actual size varies between MAXITEM/2 and MAXITEM
        const std::size_t MAXITEM = 30000;
        template<typename T>
            bool clearExceededItems(T &m)
            {
                if (m.size() > MAXITEM)
                {
                    while (m.size() > MAXITEM / 2)
                        m.erase(m.begin());
                    return true;
                }
                return false;
            }

    public:

        std::map<ClockT, double> cpuPecents;
        std::map<ClockT, Infos::MemInfoGetter::MemInfo > memInfos;
        std::map<ClockT, NginxLogEntry> nginxInfos;

        std::map<std::string, long long> nginxDirInfo;

        Infos::DiskSpaceInfo diskSpace;

        void updateDirInfoFromItem(const NginxLogEntry &e)
        {
            if (e.path[0]=='/' && (e.status == 200 || e.status == 304))
                for (size_t i=0; i<e.path.size(); ++i)
                    if (e.path[i] == '/')
                        nginxDirInfo[e.path.substr(0,i+1)]++;
        }

    public:

        void checkAndAddCPU(const double d)
        {
            cpuPecents.insert(std::make_pair(std::chrono::system_clock::now(), d));
            clearExceededItems(cpuPecents);
        }

        void checkAndAddMem(const Infos::MemInfoGetter::MemInfo &i)
        {
            memInfos.insert(std::make_pair(std::chrono::system_clock::now(), i));
            clearExceededItems(memInfos);
        }

        void checkAndAddNginxInfos(const NginxLogEntry& e)
        {
            nginxInfos.insert(make_pair(e.time_point, e));
            updateDirInfoFromItem(e);
            if (clearExceededItems(nginxInfos))
            {
                nginxDirInfo.clear();
                for (const auto &item: nginxInfos)
                    updateDirInfoFromItem(item.second);
            }
        }

        void updateDiskSpace(const Infos::DiskSpaceInfo& ds)
        {
            diskSpace = ds;
        }

        std::string stat()
        {
            std::stringstream ss;
            ss << "CPU Entry:" << cpuPecents.size() <<
                "\tMem Entry:" << memInfos.size() <<
                "\tNginxLogInfo Entry:" << nginxInfos.size() <<
                "\tNginxDirInfo Entry:" << nginxDirInfo.size();
            return ss.str();
        }
};



using namespace std;

string to_string(const chrono::system_clock::time_point &tp)
{
    std::time_t tt =  chrono::system_clock::to_time_t(tp);
    string tmp = std::to_string(tt);
    while(!tmp.empty() && (isblank(*tmp.rbegin()) || 
                    *tmp.rbegin() == '\n'))
        tmp.erase(tmp.size() - 1);
    return tmp;
}

void printJSON(std::ofstream& of, const Status& sta)
{
    using json=nlohmann::json;
    json j;
    j["cpu"] = json::array();
    for (const auto & item: sta.cpuPecents)
        j["cpu"].push_back(json::array({to_string(item.first), item.second}));

    for (const auto & item: sta.memInfos)
    {
        j["mem"]["used"].push_back(json::array({to_string(item.first), 
                        item.second.used / 1024 / 1024}));
        j["mem"]["realUsed"].push_back(json::array({to_string(item.first),
                        item.second.realUsed / 1024 / 1024}));
    }


    j["nginx"]["data"] = json::array();
    for (const auto & item: sta.nginxInfos)
    {
        json jj = 
        {
            { "path", item.second.path },
            { "method", item.second.type },
            {"status", item.second.status },
            { "time", to_string(item.first) }
        };
        j["nginx"]["data"].push_back(jj);
    }

    std::vector< pair<string, long long > > v;
    for (const auto & item:sta.nginxDirInfo)
        v.push_back(item);
    sort(v.begin(), v.end(), [](const auto& v1, const auto& v2)
                {
                if (v1.second != v2.second) return v1.second > v2.second;
                else return v1.first.length() < v2.first.length();
                });

    j["hotDir"] = json::array({});
    for (const auto &item : v)
    {
        json *cur = &j["hotDir"];
        int laststart = -1;
        for (size_t i=0; i<item.first.size(); ++i)
        {
            if (item.first[i] == '/')
            {
                if (i != item.first.size() - 1)
                {
                    auto it = std::find_if(cur->begin(), 
                                cur->end(),
                                [&](const json& ch)
                                {
                                return ch["name"] == item.first.substr(laststart+1, i-laststart);
                                });
                    assert(it != cur->end());
                    cur = &((*it)["drilldown"]);
                    laststart = i;
                }
            }
            else
            {
                std::string name = item.first.substr(laststart + 1);
                if (*name.rbegin() != '/') name += "/";
                cur->push_back(json({{"name", item.first.substr(laststart+1)}, {"count", item.second},
                                {"drilldown", json::array({})}
                                }));
            }
        }
    }

    j["diskSpace"] = {
        { "total", sta.diskSpace.total },
        { "used", sta.diskSpace.used },
        { "available", sta.diskSpace.avail },
    };

    of << j.dump(4);
}

inline std::string currentTime()
{
    return to_string(chrono::system_clock::now());
}

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        cout << R"USAGE(
        USAGE:
            watcher nginx_log_path sleep_second serve_content_root output_json_path

        This is a free software. lz96@foxmail.com)USAGE" << endl;
            return 0;
    }
    Infos::MemInfoGetter mg;
    Infos::CPUInfoGetter cg;
    Infos::NginxLogInfo nl(argv[1]);
    NginxLogEntryParser np;
    Infos::DiskSpaceGetter dg(argv[3]);
    Status s;
    for (long long cnt = 0;;++cnt)
    {
        cout << "[" << currentTime() << "]" <<
            "Iteration " << cnt << "  Begin" << endl;
        if (!std::isnan(cg.getCurrentValue())) s.checkAndAddCPU(cg.getCurrentValue());
        s.checkAndAddMem(mg.getMemInfo());
        auto newEntrys = nl.getIncrLines();
        for (const auto & item :newEntrys)
            s.checkAndAddNginxInfos(np.parse(item));
        s.updateDiskSpace(dg.getDiskSpace());
        std::ofstream of(argv[4]);
        printJSON(of, s);
        of.flush();
        of.close();

        cout << s.stat() << endl;
        cout << "[" << currentTime() << "]" <<
            "Iteration " << cnt << "  end. Sleep for " << atoi(argv[2]) << "s" << endl;
        this_thread::sleep_for(chrono::seconds(std::atoi(argv[2])));
    }

}

