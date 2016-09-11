# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,16cm font '\scriptsize' preamble '\usepackage{times,microtype}'
#set terminal tikz standalone color size 3.3in,5.6in font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-runtimes.tex"

set multiplot layout 2,1

set xrange [1:1e6]
set yrange [0:5725]

set xlabel "Runtime (ms)"
set ylabel "Number of instances"
set logscale x
set border 3
set grid x y
set xtics nomirror
set ytics nomirror add ('5725' 5725)
set key off

set format x '$10^{%T}$'

set title "Induced"

plot \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2-induced.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "$k=0$" at end lc 1, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2-induced.data" u 4:($4>=1e6?1e-10:1) smooth cumulative w steps ti "$k=1$" at end lc 2, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2-induced.data" u 5:($5>=1e6?1e-10:1) smooth cumulative w steps ti "$k=2$" at end lc 3, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2-induced.data" u 6:($6>=1e6?1e-10:1) smooth cumulative w steps ti "$k=3$" at end lc 4, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2-induced.data" u 7:($7>=1e6?1e-10:1) smooth cumulative w steps ti "$k=4$" at end lc 5, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2-induced.data" u 8:($8>=1e6?1e-10:1) smooth cumulative w steps ti "$k=5$" at end lc 6, \
    "../experiments/new-gpgnode-results/runtime-mcis-fc-induced.data" u ($3*1000):($3>=1e3?1e-10:1) smooth cumulative w steps ti "MCS" at end lc 7 dt '.', \
    "../experiments/faster-gpgnode-results/runtime-sequentialix-d2-induced.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "$k\\downarrow$" at end lc 8 dt '.'

set title "Non-Induced"

plot \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "$k=0$" at end lc 1, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2.data" u 4:($4>=1e6?1e-10:1) smooth cumulative w steps ti "$k=1$" at end lc 2, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2.data" u 5:($5>=1e6?1e-10:1) smooth cumulative w steps ti "$k=2$" at end lc 3, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2.data" u 6:($6>=1e6?1e-10:1) smooth cumulative w steps ti "$k=3$" at end lc 4, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2.data" u 7:($7>=1e6?1e-10:1) smooth cumulative w steps ti "$k=4$" at end lc 5, \
    "../experiments/faster-gpgnode-results/runtime-sequential-d2.data" u 8:($8>=1e6?1e-10:1) smooth cumulative w steps ti "$k=5$" at end lc 6, \
    "../experiments/faster-gpgnode-results/runtime-sequentialix-d2.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "$k\\downarrow$" at end lc 8 dt '.'
