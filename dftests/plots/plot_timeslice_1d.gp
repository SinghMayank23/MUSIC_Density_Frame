plot "../../after_LF_DF_conversion/1d_1.dat" u 2:__COL__ w l lw 3 title "DF"
replot "../../original_MUSIC/1d_1.dat" u 2:__COL__ w l lw 3 title "Default MUSIC"

set terminal postscript eps color
set output "plotlines1.eps"

set xrange[-17:17]
set termoption enhanced
set xlabel "x (fm)"
set ylabel "__YLABEL__"
set label "{/Symbol t} = 0.65 fm" at screen 0.5, screen 0.95 center font ",16"

replot
