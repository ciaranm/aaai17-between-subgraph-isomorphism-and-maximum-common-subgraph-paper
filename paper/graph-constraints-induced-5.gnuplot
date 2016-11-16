# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 3.2cm,4cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-constraints-induced-5.tex"

set xrange [0:1]
set yrange [0:1]
set border 3
set grid x y mx my
set xtics 1 nomirror
set ytics 1 nomirror
set mxtics 5
set mytics 5
set key off
set size square
set xlabel "Without"
set ylabel "With"
set xlabel offset character 0, character 1.5
set ylabel offset character 2, character 0

set title "$k = 5$"

plot \
    x w l lc 0, \
    "../experiments/new-gpgnode-results/constraints-sequential-d2-induced.data" u ($24/$23):($26/$25) ps 0.2 pt 1 lc 1

