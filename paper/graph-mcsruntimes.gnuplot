# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,7cm font '\scriptsize' preamble '\usepackage{times,microtype}'

set output "gen-graph-mcsruntimes.tex"

set xrange [1:1e6]
set yrange [0:4110]

set xlabel "Runtime (ms)"
set ylabel "Number of instances"
set logscale x
set border 3
set grid x y
set xtics nomirror
set ytics nomirror add ('$4110$' 4110)
set key off

set format x '$10^{%T}$'

plot \
    "../experiments/mcs-gpgnode-results/runtime-cp.data" u ($3*1000):($3>=1e3?1e-10:1) smooth cumulative w steps ti "FC" at end lc 2, \
    "../experiments/mcs-gpgnode-results/runtime-sequentialix-d2-induced.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "$k\\downarrow$" at end lc 1, \
    "../experiments/mcs-gpgnode-results/runtime-clique.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "Clique" at end lc 3


