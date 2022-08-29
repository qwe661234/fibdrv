set title "Fibonacci(n)"
set xlabel "n"
set ylabel "time(ns)"
set terminal png font " Times_New_Roman,6 "
set output "statistic.png"

plot \
"data.txt" using 1:2 with linespoints linewidth 1 title "fib sequence original", \
"data1.txt" using 1:2 with linespoints linewidth 1 title "fib sequence fast doubling recursive", \
"data2.txt" using 1:2 with linespoints linewidth 1 title "fib sequence fast doubling iterative", \
"data3.txt" using 1:2 with linespoints linewidth 1 title "fib sequence fast doubling iterative clz", \
"data4.txt" using 1:2 with linespoints linewidth 1 title "fib sequence stringAdd", \