set xrange [0:110]
set yrange [0:110]

set label "SRC" at 39,37 offset 0,-1.2 center tc rgb "red"
set label "S1" at 97,7 offset 0,-1.2 center tc rgb "red"
set label "S2" at 30,38 offset 0,-1.2 center tc rgb "red"
set label "S3" at 80,93 offset 0,-1.2 center tc rgb "red"
set label "S4" at 35,99 offset 0,-1.2 center tc rgb "red"
set label "S5" at 8,2 offset 0,-1.2 center tc rgb "red"
set label "S6" at 13,92 offset 0,-1.2 center tc rgb "red"
set label "S7" at 13,33 offset 0,-1.2 center tc rgb "red"
set label "S8" at 24,21 offset 0,-1.2 center tc rgb "red"
set label "S9" at 27,3 offset 0,-1.2 center tc rgb "red"
set label "S10" at 50,61 offset 0,-1.2 center tc rgb "red"
set label "S11" at 22,105 offset 0,-1.2 center tc rgb "red"
set label "S12" at 106,82 offset 0,-1.2 center tc rgb "red"
set label "S13" at 102,34 offset 0,-1.2 center tc rgb "red"
set label "S14" at 109,108 offset 0,-1.2 center tc rgb "red"
set label "S15" at 28,66 offset 0,-1.2 center tc rgb "red"
set label "S16" at 68,46 offset 0,-1.2 center tc rgb "red"
set label "S17" at 37,102 offset 0,-1.2 center tc rgb "red"
set label "S18" at 19,11 offset 0,-1.2 center tc rgb "red"
set label "S19" at 103,33 offset 0,-1.2 center tc rgb "red"
set label "S20" at 51,70 offset 0,-1.2 center tc rgb "red"
set label "B1" at 63,50 offset 0,-1.2 center tc rgb "red"
set label "B2" at 86,53 offset 0,-1.2 center tc rgb "red"
set label "B3" at 35,86 offset 0,-1.2 center tc rgb "red"
set label "B4" at 48,18 offset 0,-1.2 center tc rgb "red"
set label "B5" at 102,67 offset 0,-1.2 center tc rgb "red"
set label "B6" at 34,14 offset 0,-1.2 center tc rgb "red"

plot '-' with points pt 9 ps 1.5 notitle, \
     '-' with points pt 7 ps 1.5 notitle, \
     '-' with points pt 5 ps 1.5 notitle, \
     '-' with points pt 5 ps 2.0 notitle, \
     '-' with points pt 5 ps 2.5 notitle

39 37 # SRC
e
97 7 # T1
30 38 # T2
80 93 # T3
35 99 # T4
8 2 # T5
13 92 # T6
13 33 # T7
24 21 # T8
27 3 # T9
50 61 # T10
22 105 # T11
106 82 # T12
102 34 # T13
109 108 # T14
28 66 # T15
68 46 # T16
37 102 # T17
19 11 # T18
103 33 # T19
51 70 # T20
e
86 53 # B2
34 14 # B6
e
63 50 # B1
e
35 86 # B3
48 18 # B4
102 67 # B5
e
