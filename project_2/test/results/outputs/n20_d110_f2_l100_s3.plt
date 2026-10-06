set xrange [0:110]
set yrange [0:110]

set label "SRC" at 30,75 offset 0,-1.2 center tc rgb "red"
set label "S1" at 49,94 offset 0,-1.2 center tc rgb "red"
set label "S2" at 99,8 offset 0,-1.2 center tc rgb "red"
set label "S3" at 29,24 offset 0,-1.2 center tc rgb "red"
set label "S4" at 75,5 offset 0,-1.2 center tc rgb "red"
set label "S5" at 107,60 offset 0,-1.2 center tc rgb "red"
set label "S6" at 20,97 offset 0,-1.2 center tc rgb "red"
set label "S7" at 47,77 offset 0,-1.2 center tc rgb "red"
set label "S8" at 60,80 offset 0,-1.2 center tc rgb "red"
set label "S9" at 69,16 offset 0,-1.2 center tc rgb "red"
set label "S10" at 77,1 offset 0,-1.2 center tc rgb "red"
set label "S11" at 33,70 offset 0,-1.2 center tc rgb "red"
set label "S12" at 91,60 offset 0,-1.2 center tc rgb "red"
set label "S13" at 19,66 offset 0,-1.2 center tc rgb "red"
set label "S14" at 69,107 offset 0,-1.2 center tc rgb "red"
set label "S15" at 74,8 offset 0,-1.2 center tc rgb "red"
set label "S16" at 110,19 offset 0,-1.2 center tc rgb "red"
set label "S17" at 50,81 offset 0,-1.2 center tc rgb "red"
set label "S18" at 1,85 offset 0,-1.2 center tc rgb "red"
set label "S19" at 29,81 offset 0,-1.2 center tc rgb "red"
set label "S20" at 70,60 offset 0,-1.2 center tc rgb "red"
set label "B1" at 39,24 offset 0,-1.2 center tc rgb "red"
set label "B2" at 82,14 offset 0,-1.2 center tc rgb "red"
set label "B3" at 67,72 offset 0,-1.2 center tc rgb "red"
set label "B4" at 29,72 offset 0,-1.2 center tc rgb "red"

plot '-' with points pt 9 ps 1.5 notitle, \
     '-' with points pt 7 ps 1.5 notitle, \
     '-' with points pt 5 ps 1.5 notitle, \
     '-' with points pt 5 ps 2.0 notitle, \
     '-' with points pt 5 ps 2.5 notitle

30 75 # SRC
e
49 94 # T1
99 8 # T2
29 24 # T3
75 5 # T4
107 60 # T5
20 97 # T6
47 77 # T7
60 80 # T8
69 16 # T9
77 1 # T10
33 70 # T11
91 60 # T12
19 66 # T13
69 107 # T14
74 8 # T15
110 19 # T16
50 81 # T17
1 85 # T18
29 81 # T19
70 60 # T20
e
e
e
39 24 # B1
82 14 # B2
67 72 # B3
29 72 # B4
e
