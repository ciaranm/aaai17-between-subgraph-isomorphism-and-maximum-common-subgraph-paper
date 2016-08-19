# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 18cm,9cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-constraints-induced.tex"

set multiplot layout 2,3

set xrange [0:1]
set yrange [0:1]

set xlabel "Without"
set ylabel "With"
set border 3
set grid x y
set xtics nomirror
set ytics nomirror
set key off
set size square

set title "Induced, $k = 0$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2-induced.data" u ($4/$3):($6/$5):2 every 10 lc var ps 0.2 pt 1

set title "Induced, $k = 1$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2-induced.data" u ($8/$7):($10/$9):2 every 10 lc var ps 0.2 pt 1

set title "Induced, $k = 2$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2-induced.data" u ($12/$11):($14/$13):2 every 10 lc var ps 0.2 pt 1

set title "Induced, $k = 3$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2-induced.data" u ($16/$15):($18/$17):2 every 10 lc var ps 0.2 pt 1

set title "Induced, $k = 4$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2-induced.data" u ($20/$19):($22/$21):2 every 10 lc var ps 0.2 pt 1

set title "Induced, $k = 5$"

plot \
    x w l lc 0, \
    "../experiments/gpgnode-results/constraints-sequential-d2-induced.data" u ($24/$23):($26/$25):2 every 10 lc var ps 0.2 pt 1

