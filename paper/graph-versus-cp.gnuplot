# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,16cm font '\scriptsize' preamble '\usepackage{times,microtype}'
#set terminal tikz standalone color size 3.3in,5.6in font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-versus-cp.tex"

set multiplot layout 2,1

set xrange [1:1e6]
set yrange [0:4531]

set xlabel "Runtime (ms)"
set ylabel "Number of instances"
set logscale x
set border 3
set grid x y
set xtics nomirror
set ytics nomirror add ('' 4500) ('4531' 4531)
set key off

set format x '$10^{%T}$'

set title "Induced"

plot \
    "../experiments/new-gpgnode-results/runtime-sequentialix-d2-induced.data" u 3:($3>=1e6?1e-10:($2==1||$2==2||$2==8||$2==10||$2==11||$2==12?1:1e-10)) smooth cumulative w steps ti "Iter" at end lc 1, \
    "../experiments/new-gpgnode-results/runtime-mcis-fc-induced.data" u ($3*1000):($3>=1e3?1e-10:($2==1||$2==2||$2==8||$2==10||$2==11||$2==12?1:1e-10)) smooth cumulative w steps ti "CP" at end lc 2

set xrange [1:1e6]
set yrange [1:1e6]

set xlabel "CP Runtime (ms)"
set ylabel "Our Runtime (ms)"
set logscale xy
set border 3
set grid x y
unset ytics
set xtics nomirror
set ytics nomirror
set key off
set size square

set format x '$10^{%T}$'
set format y '$10^{%T}$'

set title "Induced"

plot \
    "< join ../experiments/new-gpgnode-results/runtime-sequentialix-d2-induced.data ../experiments/new-gpgnode-results/runtime-mcis-fc-induced.data" u ($5>=1e3?1e6:($5*1000)):($3>=1e6?1e6:($2==1||$2==2||$2==8||$2==10||$2==11||$2==12?$3:NaN)) w p, x w l lc 0

