# vim: set et ft=gnuplot sw=4 :

set terminal tikz size 12.5in,6.8in
set output "gen-graph-phase-transition.tex"

set xrange [0:1]
set xlabel "Pattern density" offset character 0, character -5
set ylabel "Search cost" offset character -12, character 0
set logscale y
set format y '$\mathsf{10^{%T}}$'
set format x '$\mathsf{%.1f}$'
set key Left at graph 0.30, graph 0.867 maxrows 3 spacing 5 opaque width 15 samplen 5 box
set border 3
set grid
set xtics nomirror offset character 0, character -2 add ('0.54\hspace*{1cm}' 0.54)
set ytics nomirror

set title 'Non-induced, $\mathsf{G(20, x) \rightarrowtail G(150, 0.4)}$'

plot \
    "<sed -e '1~25!d' ../paper/data/ps20-ts150.non-induced.slice.plot" u ($4==0?$1:NaN):($5) w p notitle lc '#ffb948' pt 2 ps 2, \
    "<sed -e '1~25!d' ../paper/data/ps20-ts150.non-induced.slice.plot" u ($4==1?$1:NaN):($5) w p notitle lc '#009dec' pt 6 ps 2, \
    "../paper/data/ps20-ts150.non-induced.slice-averages.plot" u 1:3 w l ti 'Mean search' lc '#951272' lw 6, \
    "" u (NaN):(NaN) w p pt 6 ps 4 lc '#009dec' ti "Satisfiable", \
    "" u (NaN):(NaN) w p pt 2 ps 4 lc '#ffb948' ti "Unsatisfiable"

