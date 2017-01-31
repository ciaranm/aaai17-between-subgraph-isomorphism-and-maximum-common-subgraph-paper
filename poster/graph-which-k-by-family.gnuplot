# vim: set et ft=gnuplot sw=4 :

#set terminal tikz standalone color size 40cm,61cm font '\HUGE' preamble '\usepackage{times,microtype}'
set terminal tikz color solid size 35cm, 14.5cm font '\small' 
set output "gen-graph-which-k-by-family.tex"

load "ylgnbu.pal"
set palette negative

set rmargin 3
set lmargin 3
set multiplot layout 1,2

set ylabel "Proportion of Instances Satisfiable" offset -7,0
set border 0
set grid nox y
set xtics nomirror scale 0 rotate by 315 offset character 0, character 0.5
set ytics nomirror scale 0

set boxwidth 0.8 absolute
set style data histogram
set style histogram rowstacked
set style fill solid border -1

set nocolorbox

set yrange [0:1]
set key outside right top spacing 5
#set key horizontal spacing 5 width 5 maxcols 3 #reverse samplen 1 width -4 maxrows 1 maxcols 12 
#set key at graph 1.3, graph 1.03 top
set title "Induced" offset 0,1.5
plot for [COL=3:9] "../experiments/final-gpgnode-results/family-which-k-sequential-d2-induced.data" u (column(COL)/$2):xticlabels(1) ti columnheader lt palette frac ((COL-3)/8.0)

unset ylabel
set ytics format ""
set key off
set title "Non-Induced" offset 0,1.5
plot for [COL=3:9] "../experiments/final-gpgnode-results/family-which-k-sequential-d2.data" u (column(COL)/$2):xticlabels(1) ti columnheader lt palette frac ((COL-3)/8.0)
