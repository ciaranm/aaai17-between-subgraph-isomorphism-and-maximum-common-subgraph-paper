# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,6cm font '\scriptsize' preamble '\usepackage{times,microtype}'
#set terminal tikz standalone color size 3.3in,5.6in font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-ids.tex"

set xrange [1e-30:1]
set y2range [0:5725]

set xlabel "Search space size reduced to at least this proportion"
set y2label "Number of instances"
set logscale x
set border 1
set grid x y2
set xtics nomirror
set noytics
set y2tics nomirror add ('$5725$' 5725)
set key off

set format x '$10^{%T}$'

plot \
    "../experiments/final-gpgnode-results/ids-sequential-induced.data" u 3:(1) smooth cumulative w steps axes x1y2 ti "$k=0$" at beginning, \
    "../experiments/final-gpgnode-results/ids-sequential-induced.data" u 4:(1) smooth cumulative w steps axes x1y2 ti "$k=1$" at beginning, \
    "../experiments/final-gpgnode-results/ids-sequential-induced.data" u 5:(1) smooth cumulative w steps axes x1y2 ti "\\raisebox{1mm}{$k=2\\kern0.2mm$}" at beginning, \
    "../experiments/final-gpgnode-results/ids-sequential-induced.data" u 6:(1) smooth cumulative w steps axes x1y2 ti "\\raisebox{1mm}{$k=3$}" at beginning, \
    "../experiments/final-gpgnode-results/ids-sequential-induced.data" u 7:(1) smooth cumulative w steps axes x1y2 ti "\\raisebox{0mm}{$k=4\\kern0.1mm$}" at beginning lc rgb '#88cc00', \
    "../experiments/final-gpgnode-results/ids-sequential-induced.data" u 8:(1) smooth cumulative w steps axes x1y2 ti "\\raisebox{-1mm}{$k=5$}" at beginning

