set xrange [0:110]
set yrange [0:110]

set label "SRC" at 106,21 offset 0,-1.2 center tc rgb "red"
set label "S1" at 10,46 offset 0,-1.2 center tc rgb "red"
set label "S2" at 56,64 offset 0,-1.2 center tc rgb "red"
set label "S3" at 7,11 offset 0,-1.2 center tc rgb "red"
set label "S4" at 110,108 offset 0,-1.2 center tc rgb "red"
set label "S5" at 3,46 offset 0,-1.2 center tc rgb "red"
set label "S6" at 102,92 offset 0,-1.2 center tc rgb "red"
set label "S7" at 67,21 offset 0,-1.2 center tc rgb "red"
set label "S8" at 85,109 offset 0,-1.2 center tc rgb "red"
set label "S9" at 74,87 offset 0,-1.2 center tc rgb "red"
set label "S10" at 39,32 offset 0,-1.2 center tc rgb "red"
set label "S11" at 110,65 offset 0,-1.2 center tc rgb "red"
set label "S12" at 81,50 offset 0,-1.2 center tc rgb "red"
set label "S13" at 48,54 offset 0,-1.2 center tc rgb "red"
set label "S14" at 20,55 offset 0,-1.2 center tc rgb "red"
set label "S15" at 59,40 offset 0,-1.2 center tc rgb "red"
set label "S16" at 34,4 offset 0,-1.2 center tc rgb "red"
set label "S17" at 77,27 offset 0,-1.2 center tc rgb "red"
set label "S18" at 77,4 offset 0,-1.2 center tc rgb "red"
set label "S19" at 94,103 offset 0,-1.2 center tc rgb "red"
set label "S20" at 47,69 offset 0,-1.2 center tc rgb "red"
set label "B1" at 81,36 offset 0,-1.2 center tc rgb "red"
set label "B2" at 42,52 offset 0,-1.2 center tc rgb "red"
set label "B3" at 53,15 offset 0,-1.2 center tc rgb "red"
set label "B4" at 64,57 offset 0,-1.2 center tc rgb "red"
set label "B5" at 85,80 offset 0,-1.2 center tc rgb "red"

plot '-' with points pt 9 ps 1.5 notitle, \
     '-' with points pt 7 ps 1.5 notitle, \
     '-' with points pt 5 ps 1.5 notitle, \
     '-' with points pt 5 ps 2.0 notitle, \
     '-' with points pt 5 ps 2.5 notitle

106 21 # SRC
e
10 46 # T1
56 64 # T2
7 11 # T3
110 108 # T4
3 46 # T5
102 92 # T6
67 21 # T7
85 109 # T8
74 87 # T9
39 32 # T10
110 65 # T11
81 50 # T12
48 54 # T13
20 55 # T14
59 40 # T15
34 4 # T16
77 27 # T17
77 4 # T18
94 103 # T19
47 69 # T20
e
e
e
81 36 # B1
42 52 # B2
53 15 # B3
64 57 # B4
85 80 # B5
e
