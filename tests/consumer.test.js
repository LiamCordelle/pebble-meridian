'use strict';
var assert=require('assert');
var options=require('../src/pkjs/app-config');
var schema=require('../src/pkjs/kit/config')(options);
var settings=require('../src/pkjs/kit/settings')(options);
var pkg=require('../package.json');
assert.strictEqual(schema[0].defaultValue,'Meridian');
assert.strictEqual(options.storageKey,'meridian-kit-settings-v1');
assert.strictEqual(options.largeLabels,false);
assert(!schema[1].items.some(function(v){return v.messageKey==='LARGE_LABELS';}));
var memory={};memory[options.storageKey]=JSON.stringify({TIME_MODE:2,TEMP_UNIT:1,STEP_GOAL:18000});
var store={getItem:function(k){return memory[k]||null;},setItem:function(k,v){memory[k]=v;}};
assert.strictEqual(settings.load(store).STEP_GOAL,18000);
settings.save(store,settings.load(store));
assert.strictEqual(JSON.parse(memory[options.storageKey]).TEMP_UNIT,1);
assert.deepStrictEqual(pkg.pebble.messageKeys,['READY','REQUEST_WEATHER','WEATHER_ID','WEATHER',
  'WEATHER_ERROR','TIME_MODE','TEMP_UNIT','STEP_GOAL','WORLD','WEATHER_ENABLED','DEMO_FALLBACK','LARGE_LABELS']);
console.log('Meridian shared-settings UI, namespace, persistence and protocol checks passed.');
