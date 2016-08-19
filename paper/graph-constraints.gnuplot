# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,9cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-constraints.tex"

set multiplot layout 2,2

set xrange [0:1]
set yrange [0:1]

set xlabel "Without"
set ylabel "With"
set border 1
set grid x y
set xtics nomirror
set ytics nomirror
set key off
set size square

set title "Non-Induced, $k = 0$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2.data" u ($4/$3):($6/$5) lc 1

set title "Non-Induced, $k = 1$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2.data" u ($8/$7):($10/$9) lc 1

set title "Non-Induced, $k = 2$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2.data" u ($12/$11):($14/$13) lc 1

set title "Non-Induced, $k = 3$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2.data" u ($16/$15):($18/$17) lc 1
