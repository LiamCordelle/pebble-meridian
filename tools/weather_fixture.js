'use strict';
// Synthetic sensor data, current timestamps: exercises the production protocol.
var fs=require('fs'),weather=require('../src/pkjs/kit/weather');
var data=require('../shared-kit/tests/fixture')();
var now=Math.floor(Date.now()/1000),offset=3600;
var start=Math.floor((now+offset)/86400)*86400-offset;
var shift=start-data.daily.time[0];
data.current.time=now;data.current.temperature_2m=14;
['time','sunrise','sunset'].forEach(function(key){
  data.daily[key]=data.daily[key].map(function(t){return t+shift;});
});
data.hourly.time=data.hourly.time.map(function(t){return t+shift;});
data.daily.uv_index_max=[6,5];
fs.mkdirSync('build/tests',{recursive:true});
fs.writeFileSync('build/tests/emulator-weather.bin',Buffer.from(weather.encode(data,now)));
fs.writeFileSync('shots/emulator-weather-fixture.json',JSON.stringify(data,null,2)+'\n');
