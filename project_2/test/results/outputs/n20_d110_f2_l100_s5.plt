set xrange [0:110]
set yrange [0:110]

set label "SRC" at 94,45 offset 0,-1.2 center tc rgb "red"
set label "S1" at 59,99 offset 0,-1.2 center tc rgb "red"
set label "S2" at 31,1 offset 0,-1.2 center tc rgb "red"
set label "S3" at 48,69 offset 0,-1.2 center tc rgb "red"
set label "S4" at 93,27 offset 0,-1.2 center tc rgb "red"
set label "S5" at 3,107 offset 0,-1.2 center tc rgb "red"
set label "S6" at 101,88 offset 0,-1.2 center tc rgb "red"
set label "S7" at 107,94 offset 0,-1.2 center tc rgb "red"
set label "S8" at 79,32 offset 0,-1.2 center tc rgb "red"
set label "S9" at 23,98 offset 0,-1.2 center tc rgb "red"
set label "S10" at 13,73 offset 0,-1.2 center tc rgb "red"
set label "S11" at 14,47 offset 0,-1.2 center tc rgb "red"
set label "S12" at 83,67 offset 0,-1.2 center tc rgb "red"
set label "S13" at 9,17 offset 0,-1.2 center tc rgb "red"
set label "S14" at 6,20 offset 0,-1.2 center tc rgb "red"
set label "S15" at 60,31 offset 0,-1.2 center tc rgb "red"
set label "S16" at 52,35 offset 0,-1.2 center tc rgb "red"
set label "S17" at 49,20 offset 0,-1.2 center tc rgb "red"
set label "S18" at 79,79 offset 0,-1.2 center tc rgb "red"
set label "S19" at 97,102 offset 0,-1.2 center tc rgb "red"
set label "S20" at 31,83 offset 0,-1.2 center tc rgb "red"
set label "B1" at 54,65 offset 0,-1.2 center tc rgb "red"
set label "B2" at 46,85 offset 0,-1.2 center tc rgb "red"
set label "B3" at 56,23 offset 0,-1.2 center tc rgb "red"
set label "B4" at 23,70 offset 0,-1.2 center tc rgb "red"
set label "B5" at 71,91 offset 0,-1.2 center tc rgb "red"
set label "B6" at 74,16 offset 0,-1.2 center tc rgb "red"
set label "B7" at 38,20 offset 0,-1.2 center tc rgb "red"

plot '-' with points pt 9 ps 1.5 notitle, \
     '-' with points pt 7 ps 1.5 notitle, \
     '-' with points pt 5 ps 1.5 notitle, \
     '-' with points pt 5 ps 2.0 notitle, \
     '-' with points pt 5 ps 2.5 notitle

94 45 # SRC
e
59 99 # T1
31 1 # T2
48 69 # T3
93 27 # T4
3 107 # T5
101 88 # T6
107 94 # T7
79 32 # T8
23 98 # T9
13 73 # T10
14 47 # T11
83 67 # T12
9 17 # T13
6 20 # T14
60 31 # T15
52 35 # T16
49 20 # T17
79 79 # T18
97 102 # T19
31 83 # T20
e
54 65 # B1
56 23 # B3
74 16 # B6
38 20 # B7
e
23 70 # B4
e
46 85 # B2
71 91 # B5
e
