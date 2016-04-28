#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <sys/types.h>
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
                return (memInfo.totalram - memInfo.freeram - memInfo.bufferram) * memInfo.mem_unit;
            }

            double getLoad()
            {
                sysinfo(&memInfo);
                return memInfo.loads[0];
            }

        public:
            struct MemInfo
            {
                unsigned long long tot, used;
                double load;
            };
            MemInfo getMemInfo()
            {
                return MemInfo { getTot(), getUsed(), getLoad() };
            }
    };

    class CPUInfoGetter
    {
        private:
            unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

        public:
            CPUInfoGetter(){
                FILE* file = fopen("/proc/stat", "r");
                fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
                            &lastTotalSys, &lastTotalIdle);
                fclose(file);
            }

            double getCurrentValue(){
                double percent;
                FILE* file;
                unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;
                using namespace std::chrono;
                std::this_thread::sleep_for(300ms);
                file = fopen("/proc/stat", "r");
                fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
                            &totalSys, &totalIdle);
                fclose(file);

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
        const std::size_t MAXITEM = 30000;
        template<typename T>
            void clearExceededItems(T &m)
            {
                if (m.size() > MAXITEM)
                {
                    while (m.size() > MAXITEM / 2)
                        m.erase(m.begin());
                }
            }
    public:

        std::map<ClockT, double> cpuPecents;
        std::map<ClockT, Infos::MemInfoGetter::MemInfo > memInfos;
        std::map<ClockT, NginxLogEntry> nginxInfos;

        std::map<std::string, long long> nginxDirInfo;

        void checkAndAddCPU(const double d)
        {
            clearExceededItems(cpuPecents);
            cpuPecents.insert(std::make_pair(std::chrono::system_clock::now(), d));
        }

        void checkAndAddMem(const Infos::MemInfoGetter::MemInfo &i)
        {
            clearExceededItems(memInfos);
            memInfos.insert(std::make_pair(std::chrono::system_clock::now(), i));
        }

        void checkAndAddNginxInfos(const NginxLogEntry& e)
        {
            clearExceededItems(nginxInfos);
            nginxInfos.insert(make_pair(e.time_point, e));
            if (e.status == 200 || e.status == 304)
                for (size_t i=0; i<e.path.size(); ++i)
                    if (e.path[i] == '/')
                        nginxDirInfo[e.path.substr(0,i+1)]++;
        }
};



using namespace std;

string to_string(const chrono::system_clock::time_point &tp)
{
    std::time_t tt =  chrono::system_clock::to_time_t(tp);
    string tmp = ctime(&tt);
    while(!tmp.empty() && (isblank(*tmp.rbegin()) || 
                    *tmp.rbegin() == '\n'))
        tmp.erase(tmp.size() - 1);
    return tmp;
}

void printJSON(std::ofstream& of, const Status& sta)
{
    of << "{" << endl;

    of << "\"cpu\":" << endl;
    of << "{" << endl;

    of << "\"time\":" << endl;
    of << "[" << endl;
    size_t cnt = 0;
    for (const auto & item: sta.cpuPecents)
    {
        of << "\"" << to_string(item.first) << "\"";
        if (++cnt == sta.cpuPecents.size())
            of << endl;
        else
            of << ",";
    }
    of << "]," << endl;

    of << "\"rate\":" << endl;
    of << "[" << endl;
    cnt = 0;
    for (const auto & item: sta.cpuPecents)
    {
        of << item.second;
        if (++cnt == sta.cpuPecents.size())
            of << endl;
        else
            of << ",";
    }
    of << "]" << endl; //rate
    of << "}," << endl; //cpu

    of << "\"mem\":" << endl;
    of << "{" << endl;

    of << "\"time\":" << endl;
    of << "[" << endl;
    cnt = 0;
    for (const auto & item: sta.memInfos)
    {
        of <<  "\"" << to_string(item.first) << "\"";
        if (++cnt == sta.memInfos.size())
            of << endl;
        else
            of << ",";
    }
    of << "]," << endl;

    of << "\"rate\":" << endl;
    of << "[" << endl;
    cnt = 0;
    for (const auto & item: sta.memInfos)
    {
        of << item.second.used / 1024 / 1024;
        if (++cnt == sta.memInfos.size())
            of << endl;
        else
            of << ",";
    }
    of << "]" << endl; //rate
    of << "}," << endl; //mem

    of << "\"nginx\":" << endl;
    of << "{" << endl;

    of << "\"data\":" << endl;
    of << "[" << endl;
    cnt = 0;
    for (const auto & item: sta.nginxInfos)
    {
        of << "{" << endl;
        of << "\"path\": \"" << item.second.path << "\"," << endl;
        of << "\"method\": \"" << item.second.type << "\"," << endl;
        of << "\"status\": "<<item.second.status << "," << endl;
        of << "\"time\": \"" << to_string(item.first) << "\"" << endl;
        of << "}";
        if (++cnt == sta.nginxInfos.size())
            of << endl;
        else
            of << "," << endl;
    }
    of << "]" << endl; //data
    of << "}," << endl; //nginx

    std::vector< pair<string, long long > > v;
    for (const auto & item:sta.nginxDirInfo)
        v.push_back(item);
    sort(v.begin(), v.end(), [](const auto& v1, const auto& v2)
                {
                return v1.second > v2.second;
                });

    of << "\"hotDir\":" << endl;
    of << "[" << endl;
    cnt = 0;
    for (const auto &item : v)
    {
        of << "{" << endl;
        of << "\"dir\": \""<< item.first << "\"," << endl;
        of << "\"count\": \"" << item.second << "\"" << endl;
        of << "}";
        if (++cnt == v.size())
            of << endl;
        else
            of << "," << endl;
    }
    of << "]" << endl; //hotDir

    of << "}" << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        cout << R"USAGE(
        USAGE:
            watcher nginx_log_path sleep_second output_json_path

        This is a free software. lz96@foxmail.com)USAGE" << endl;
            return 0;
    }
    Infos::MemInfoGetter mg;
    Infos::CPUInfoGetter cg;
    Infos::NginxLogInfo nl(argv[1]);
    NginxLogEntryParser np;
    Status s;
    for (long long cnt = 0;;++cnt)
    {
        cout << "Iteration " << cnt << "  Begin" << endl;
        if (!isnan(cg.getCurrentValue())) s.checkAndAddCPU(cg.getCurrentValue());
        s.checkAndAddMem(mg.getMemInfo());
        auto newEntrys = nl.getIncrLines();
        for (const auto & item :newEntrys)
            s.checkAndAddNginxInfos(np.parse(item));
        std::ofstream of(argv[3]);
        printJSON(of, s);

        cout << "Iteration " << cnt << "  end. Sleep for " << atoi(argv[2]) << "s" << endl;
        this_thread::sleep_for(chrono::seconds(std::atoi(argv[2])));
    }

}

