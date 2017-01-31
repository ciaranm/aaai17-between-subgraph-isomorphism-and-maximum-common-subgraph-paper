# vim: set et ft=gnuplot sw=4 :

#set terminal tikz standalone color size 17.6cm,5.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
#set terminal tikz standalone color size 3.3in,5.6in font '\scriptsize' preamble '\usepackage{times,microtype}'
set terminal tikz color solid size 50cm, 13.8cm font '\footnotesize'
set output "gen-graph-runtimes.tex"

set multiplot layout 1,3

set lmargin 20
set rmargin 20

set xrange [1:1e6]
set yrange [0:5725]

set xlabel "Runtime (ms)" offset 0, -3
set ylabel "Number of instances" offset -12, 0
set logscale x
set border 3
set grid x y
set xtics nomirror scale 0 offset 0, -0.95
set ytics nomirror add ('$5725$' 5725) scale 0
set key off

set format x '$10^{%T}$'

set title "Induced" offset 0,1.5

plot \
    "../experiments/final-gpgnode-results/runtime-sequential-d2-induced.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{1.5cm}{$k=0$}" at end lc 1, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2-induced.data" u 4:($4>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{1cm}{$k=1$}" at end lc 2, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2-induced.data" u 5:($5>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{0.5cm}{$k=2$}" at end lc 3, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2-induced.data" u 6:($6>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{0cm}{$k=3$}" at end lc 4, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2-induced.data" u 7:($7>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{-0.5cm}{$k=4$}" at end lc rgb '#88cc00', \
    "../experiments/final-gpgnode-results/runtime-sequential-d2-induced.data" u 8:($8>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{-1cm}{$k=5$}" at end lc 6, \
    "../experiments/final-gpgnode-results/runtime-mcis-fc-induced.data" u ($3*1000):($3>=1e3?1e-10:1) smooth cumulative w steps ti "FC" at end lc 2 dt '.', \
    "../experiments/final-gpgnode-results/runtime-clique.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "Clique" at end lc 3 dt '.', \
    "../experiments/final-gpgnode-results/runtime-sequentialix-d2-induced.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "$k\\downarrow$" at end lc 1 dt '.'

set title "Non-Induced" offset 0,1.5

plot \
    "../experiments/final-gpgnode-results/runtime-sequential-d2.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{1.5cm}{$k=0$}" at end lc 1, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2.data" u 4:($4>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{1.0cm}{$k=1$}" at end lc 2, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2.data" u 5:($5>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{0.5cm}{$k=2$}" at end lc 3, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2.data" u 6:($6>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{0.0cm}{$k=3$}" at end lc 4, \
    "../experiments/final-gpgnode-results/runtime-sequential-d2.data" u 7:($7>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{-0.5cm}{$k=4$}" at end lc rgb '#88cc00', \
    "../experiments/final-gpgnode-results/runtime-sequential-d2.data" u 8:($8>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{-1cm}{$k=5$}" at end lc 6, \
    "../experiments/final-gpgnode-results/runtime-sequentialix-d2.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "$k\\downarrow$" at end lc 8 dt '.'


set xrange [1:1e6]
set yrange [0:4110]

set xlabel "Runtime (ms)" offset 0, -3
set ylabel "Number of instances" offset -12,0
set logscale x
set border 3
set grid x y
set xtics nomirror
set ytics nomirror add ('' 4000) add ('$4110$' 4110)
set key off

set title "Induced, MCS Instances" offset 0, 1.5

set format x '$10^{%T}$'

plot \
    "../experiments/mcs-gpgnode-results/runtime-cp.data" u ($3*1000):($3>=1e3?1e-10:1) smooth cumulative w steps ti "\\raisebox{0cm}{FC}" at end lc 2, \
    "../experiments/mcs-gpgnode-results/runtime-sequentialix-d2-induced.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "\\raisebox{1cm}{$k\\downarrow$}" at end lc 1, \
    "../experiments/mcs-gpgnode-results/runtime-clique.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w steps ti "Clique" at end lc 3

