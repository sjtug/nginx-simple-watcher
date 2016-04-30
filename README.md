#Nginx-Simple-Watcher
A zero-extra-dependency C++14 nginx server watcher providing CPU usage, mem usage, latest nginx logs and hottest directories, all in JSON format.

A simple static web frontend is also provided.

*Linux3.2+ only.*
## Compile
Use any compiler that has support for C++14.

```
g++ -std=c++14 -O2 watcher.cpp -o watcher -pthread
```

## Usage
```
./watcher nginx_access_log_path sleep_second output_json_path
```

Then copy `status.html`, `updatedata.js`, `status.css`, `psd3.min.css`, `psd3.min.js` to the same directory as the json's.

### Output Format
```json
{
    "cpu":
    [
        ["time_1", 10.1],
        ["time_2", 29]
    ],
    "mem":
    {
        "realUsed":
            [
                ["time_1", 1234],
                ["time_2", 1300]
            ],
        "used":
                [
                    ["time_1", 2300],
                    ["time_2", 2400]
                ]
    },
    "nginx":
    {
        "data":
        [
            {
                "path": "/test/",
                "method": "GET",
                "status": 200,
                "time": "time_1"
            }
        ]
    },
    "hotDir":
    [
        {
            "count": 22,
            "name": "/",
            "drilldown":
                [
                    {
                        "count": 11,
                        "name": "archlinux/",
                        "drilldown": []
                    }
                ]
        }
    ]
}
```


Modify `NginxLogEntryParser` to meet your nginx access log format!
