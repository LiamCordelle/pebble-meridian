#!/usr/bin/env python3
"""Exercise the production goal-time calculation with current-hour health data."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
text=(root/'src/c/design.c').read_text()
# Compile the actual pure calculation; no replacement copy of its implementation.
start=text.index('static int goal_minute(')
end=text.index('// ETA at today',start)
harness='''#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "kit_calc.h"
typedef struct {struct tm now; uint16_t steps_by_hour[24];bool steps_ok;int minute_of_day;} LabData;
'''+text[start:end]+'''
int main(void) {
 LabData d={0};d.now.tm_hour=6;d.now.tm_min=10;d.minute_of_day=370;d.steps_ok=true;d.steps_by_hour[6]=10000;
 assert(goal_minute(&d,5000)==365); // 06:05, not 06:30
 assert(goal_minute(&d,10000)==370); // never later than now
 d.steps_by_hour[5]=5000;
 assert(goal_minute(&d,5000)==360); // completed hour retains its full width
 assert(goal_minute(&d,20000)==-1); // goal not yet reached
 return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
 source=Path(temp)/'goal.c';source.write_text(harness)
 exe=Path(temp)/'goal'
 subprocess.run(['cc','-std=c99','-Wall','-Werror','-I'+str(root/'shared-kit/src/c/kit'),
                 str(source),str(root/'shared-kit/src/c/kit/kit_calc.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Production goal-time checks passed: current hour, boundary and unmet goal.')
