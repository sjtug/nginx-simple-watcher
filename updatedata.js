var cpuChart, memChart;
function fetchDataHelper(cb) {
    $.ajax({
        url: "test.json",
        data: "",
        success: function(result, status, xhr) {
            cb(result, xhr.getResponseHeader("Last-Modified"));
        },
        dataType: "json",
        error: function (result) {
            setTimeout(function() { fetchDataHelper(cb); }, 3000);
        },
    });
}

function updateData() {
    fetchDataHelper(function(data, lastupdatetime) {
        $("#lastupdatetime").text(lastupdatetime);
        console.log(lastupdatetime);


        data.cpu = data.cpu.map(function(val) {
            return [parseInt(val[0]) * 1000, val[1]];
        });
        cpuChart.series[0].setData(data.cpu);

        data.mem.used = data.mem.used.map(function(val) {
            return [parseInt(val[0]) * 1000, val[1]];
        });
        memChart.series[0].setData(data.mem.used);
        data.mem.realUsed = data.mem.realUsed.map(function(val) {
            return [parseInt(val[0]) * 1000, val[1]];
        });
        memChart.series[1].setData(data.mem.realUsed);

        $("#diskspaceused").text((data.diskSpace.used / 1073741824).toFixed(2));
        $("#diskspaceavail").text((data.diskSpace.available / 1073741824).toFixed(2));

        $("#hotdir-content").html("");
        var config = {
            containerId : "hotdir-content",
            width: 500,
            height: 500,
            data: data.hotDir,
            heading: {
                text : "Hottest Directories",
                pos: "top"
            },
            tooltip: function(d) {
                return d.name + ": " + d.count + "times";
            },
            label: function(d) {
                return d.data.name + ": " + d.data.count +" times";
            },
            value: "count",
            inner: "drilldown",
            transitionDuration: 300
        };
        data.nginx.data = data.nginx.data.map(function(val) {
            return {
                "method": val.method,
                "path": val.path,
                "status": val.status,
                "time": new Date(parseInt(val.time) * 1000)
            };
        });
        var hotListPie = new psd3.Pie(config);

             var nginxloglist = new List("nginxlog-content", { item: "nginxlog-item", valueNames:
                                         ["method", "path", "status", "time"],
                                    page: 10,
                                    plugins: [
                                        ListPagination({})
                                    ]
             });
             nginxloglist.clear();
             nginxloglist.add(data.nginx.data);

    });
}

$(function() {
    cpuChart = new Highcharts.Chart(
        {
            chart: {
                zoomType: "x",
                renderTo: 'cpuChart'
            },
            title: {
                text: "CPU Usage"
            },
            xAxis: {
                type: "datetime"
            },
            yAxis: {
                title: {
                    text: "CPU Usage(%)"
                }
            },
            plotOptions: {
                area: {
                    fillColor: {
                        linearGradient: {
                            x1: 0,
                            y1: 0,
                            x2: 0,
                            y2: 1
                        },
                        stops: [
                            [0, Highcharts.getOptions().colors[0]],
                            [1, Highcharts.Color(Highcharts.getOptions().colors[0]).setOpacity(0).get('rgba')]
                        ]
                    },
                    marker: {
                        radius: 2
                    },
                    lineWidth: 1,
                    states: {
                        hover: {
                            lineWidth: 1
                        }
                    },
                    threshold: null
                }
            },
            series: [{
                type: 'area',
                name: 'CPU Usage(%)'
            }]
        });

        memChart = new Highcharts.Chart(
            {
                chart: {
                    zoomType: "x",
                    renderTo: "memChart"
                },
                title: {
                    text: "Memory Usage"
                },
                xAxis: {
                    type: "datetime"
                },
                yAxis: {
                    title: {
                        text: "mem Usage(MB)"
                    }
                },
                plotOptions: {
                    area: {
                        fillColor: {
                            linearGradient: {
                                x1: 0,
                                y1: 0,
                                x2: 0,
                                y2: 1
                            },
                            stops: [
                                [0, Highcharts.getOptions().colors[0]],
                                [1, Highcharts.Color(Highcharts.getOptions().colors[0]).setOpacity(0).get('rgba')]
                            ]
                        },
                        marker: {
                            radius: 2
                        },
                        lineWidth: 1,
                        states: {
                            hover: {
                                lineWidth: 1
                            }
                        },
                        threshold: null
                    }
                },
                series: [{
                    type: 'area',
                    name: 'All usage'
                }, {
                    type: 'area',
                    name: 'pure usage'
                }]
            });
    updateData();
    setInterval(updateData, 60000);
});
