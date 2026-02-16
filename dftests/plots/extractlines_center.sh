
for ii in {0..173}
do
        sed -n '32897,33152p' plotdata$ii.dat > 1d_$ii.dat
done
