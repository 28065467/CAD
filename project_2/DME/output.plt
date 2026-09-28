set xrange [0:110]
set yrange [0:100]

set label "SRC" at 97,38 offset 0,-1.2 center tc rgb "red"
set label "S1" at 49,62 offset 0,-1.2 center tc rgb "red"
set label "S2" at 17,25 offset 0,-1.2 center tc rgb "red"
set label "S3" at 56,82 offset 0,-1.2 center tc rgb "red"
set label "S4" at 72,27 offset 0,-1.2 center tc rgb "red"
set label "S5" at 9,91 offset 0,-1.2 center tc rgb "red"
set label "B1" at 21,40 offset 0,-1.2 center tc rgb "red"
set label "B2" at 21,69 offset 0,-1.2 center tc rgb "red"

plot '-' with points pt 9 ps 1.5 notitle, \
     '-' with points pt 7 ps 1.5 notitle, \
     '-' with points pt 5 ps 1.5 notitle, \
     '-' with points pt 5 ps 2.0 notitle, \
     '-' with points pt 5 ps 2.5 notitle

97 38 # SRC
e
49 62 # T1
17 25 # T2
56 82 # T3
72 27 # T4
9 91 # T5
e
e
21 40 # B1
e
21 69 # B2
e
