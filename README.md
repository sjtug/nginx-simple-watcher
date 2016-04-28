#Nginx-Simple-Watcher
A zero-extra-dependency C++14 nginx server watcher providing CPU usage, mem usage, latest nginx logs and hottest directories, all in JSON format.

*Linux3.2+ only.*
## Compile
Use any compiler that has support for C++14.

```
g++ -std=c++14 watcher.cpp -o watcher -pthread
```

## Usage
```
./watcher nginx_access_log_path sleep_second output_json_path
```

### Output Format
```json
{
    "cpu":
    {
        "time":
            [
                "time_1",
                "time_2"
            ],
        "rate":
            [
                2.3,
                29.2
            ]
    },
    "mem":
    {
        "time":
            [
                "time_1",
                "time_2"
            ],
            "rate":
                [
                    1022,
                    1122
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
            "dir": "/test/",
            "count": 2
        }
    ]
}
```


Modify `NginxLogEntryParser` to meet your nginx access log format!
