# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,12.2cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-which-k-by-family.tex"

load "ylgnbu.pal"
set palette negative

set multiplot layout 2,1

set ylabel "Proportion of Instances Satisfiable"
set border 3
set grid nox y
set xtics nomirror scale 0 rotate by 315 offset character 0, character 0.5
set ytics nomirror scale 0
set key outside right top

set boxwidth 0.8 absolute
set style data histogram
set style histogram rowstacked
set style fill solid border -1

set nocolorbox

set yrange [0:1]

set title "Induced"
plot for [COL=3:9] "../experiments/faster-gpgnode-results/family-which-k-sequential-d2-induced.data" u (column(COL)/$2):xticlabels(1) ti columnheader lt palette frac ((COL-3)/8.0)

set title "Non-Induced"
plot for [COL=3:9] "../experiments/faster-gpgnode-results/family-which-k-sequential-d2.data" u (column(COL)/$2):xticlabels(1) ti columnheader lt palette frac ((COL-3)/8.0)
