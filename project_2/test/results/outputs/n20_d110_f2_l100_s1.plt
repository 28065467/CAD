set xrange [0:110]
set yrange [0:110]

set label "SRC" at 34,92 offset 0,-1.2 center tc rgb "red"
set label "S1" at 98,0 offset 0,-1.2 center tc rgb "red"
set label "S2" at 49,55 offset 0,-1.2 center tc rgb "red"
set label "S3" at 97,8 offset 0,-1.2 center tc rgb "red"
set label "S4" at 77,97 offset 0,-1.2 center tc rgb "red"
set label "S5" at 83,69 offset 0,-1.2 center tc rgb "red"
set label "S6" at 2,3 offset 0,-1.2 center tc rgb "red"
set label "S7" at 32,15 offset 0,-1.2 center tc rgb "red"
set label "S8" at 89,57 offset 0,-1.2 center tc rgb "red"
set label "S9" at 17,72 offset 0,-1.2 center tc rgb "red"
set label "S10" at 75,13 offset 0,-1.2 center tc rgb "red"
set label "S11" at 108,102 offset 0,-1.2 center tc rgb "red"
set label "S12" at 57,60 offset 0,-1.2 center tc rgb "red"
set label "S13" at 1,48 offset 0,-1.2 center tc rgb "red"
set label "S14" at 102,29 offset 0,-1.2 center tc rgb "red"
set label "S15" at 40,3 offset 0,-1.2 center tc rgb "red"
set label "S16" at 63,97 offset 0,-1.2 center tc rgb "red"
set label "S17" at 100,26 offset 0,-1.2 center tc rgb "red"
set label "S18" at 12,62 offset 0,-1.2 center tc rgb "red"
set label "S19" at 3,106 offset 0,-1.2 center tc rgb "red"
set label "S20" at 83,48 offset 0,-1.2 center tc rgb "red"
set label "B1" at 53,49 offset 0,-1.2 center tc rgb "red"
set label "B2" at 92,58 offset 0,-1.2 center tc rgb "red"
set label "B3" at 19,45 offset 0,-1.2 center tc rgb "red"
set label "B4" at 90,25 offset 0,-1.2 center tc rgb "red"
set label "B5" at 70,70 offset 0,-1.2 center tc rgb "red"
set label "B6" at 3,76 offset 0,-1.2 center tc rgb "red"
set label "B7" at 23,7 offset 0,-1.2 center tc rgb "red"

plot '-' with points pt 9 ps 1.5 notitle, \
     '-' with points pt 7 ps 1.5 notitle, \
     '-' with points pt 5 ps 1.5 notitle, \
     '-' with points pt 5 ps 2.0 notitle, \
     '-' with points pt 5 ps 2.5 notitle

34 92 # SRC
e
98 0 # T1
49 55 # T2
97 8 # T3
77 97 # T4
83 69 # T5
2 3 # T6
32 15 # T7
89 57 # T8
17 72 # T9
75 13 # T10
108 102 # T11
57 60 # T12
1 48 # T13
102 29 # T14
40 3 # T15
63 97 # T16
100 26 # T17
12 62 # T18
3 106 # T19
83 48 # T20
e
53 49 # B1
19 45 # B3
23 7 # B7
e
70 70 # B5
3 76 # B6
e
92 58 # B2
90 25 # B4
e
