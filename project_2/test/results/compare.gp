set terminal pngcairo size 1200,1300 font "Sans,11" background "#fcfcfb"
set output "/home/lin/CAD/project_2/test/results/compare.png"
set multiplot layout 3,1 title "Relative to naive greedy (= 100%, dashed line) — lower is better" font "Sans,13" textcolor rgb "#0b0b0b"
set style data histograms
set style histogram clustered gap 1
set style fill solid 1.0 border lc rgb "#fcfcfb"
set boxwidth 0.92
set border 3 lc rgb "#c3c2b7"
set tics textcolor rgb "#52514e"
set xtics nomirror scale 0 font "Sans,10"
set ytics nomirror
set grid ytics lc rgb "#e6e5e0" lw 1
set format y "%g%%"
set yrange [0:*]
set offsets 0, 0, graph 0.12, 0
set bmargin 3.5
set lmargin 9
set key top left horizontal textcolor rgb "#0b0b0b" samplen 1.5
set title "Score" textcolor rgb "#0b0b0b"
plot "/home/lin/CAD/project_2/test/results/compare.dat" using 2:xtic(1) title "best build" lw 2 lc rgb "#2a78d6", \
     "" using 3 title "+ local search" lw 2 lc rgb "#eb6834", \
     "" using ($0 + 1.0/6):3:(sprintf("%.0f%%", $3)) with labels offset 0,0.7 font "Sans,8" textcolor rgb "#52514e" notitle, \
     100 with lines dt 2 lw 1.5 lc rgb "#52514e" notitle
unset key
set title "Buffer cost" textcolor rgb "#0b0b0b"
plot "/home/lin/CAD/project_2/test/results/compare.dat" using 4:xtic(1) title "best build" lw 2 lc rgb "#2a78d6", \
     "" using 5 title "+ local search" lw 2 lc rgb "#eb6834", \
     "" using ($0 + 1.0/6):5:(sprintf("%.0f%%", $5)) with labels offset 0,0.7 font "Sans,8" textcolor rgb "#52514e" notitle, \
     100 with lines dt 2 lw 1.5 lc rgb "#52514e" notitle
unset key
set title "Skew" textcolor rgb "#0b0b0b"
plot "/home/lin/CAD/project_2/test/results/compare.dat" using 6:xtic(1) title "best build" lw 2 lc rgb "#2a78d6", \
     "" using 7 title "+ local search" lw 2 lc rgb "#eb6834", \
     "" using ($0 + 1.0/6):7:(sprintf("%.0f%%", $7)) with labels offset 0,0.7 font "Sans,8" textcolor rgb "#52514e" notitle, \
     100 with lines dt 2 lw 1.5 lc rgb "#52514e" notitle
unset multiplot
