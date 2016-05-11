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
./watcher nginx_access_log_path sleep_second serve_content_root output_json_path
```

Then copy `status.html`, `updatedata.js`, `status.css`, `psd3.min.css`, `psd3.min.js` to the same directory as the json's.

## Screenshot

![Screenshot](https://github.com/htfy96/nginx-simple-watcher/raw/master/screenshot.png)

## Output Format
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

## License
This project is under GPLv3 license, see `LICENSE` for details.

`json.hpp` is from https://github.com/nlohmann/json, which is licensed under the MIT license.

`psd3.min.css` and `psd3.min.js` are from https://github.com/pshivale/psd3, which is also licensed under MIT license.

> This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

> This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

> You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
