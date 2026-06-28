all:  cs137.root  cs137_simple.root
	echo "Geant4 simulations done."

# Pattern rules for running Geant4 simulations.
%.out : %.mac
	if test -e $*.out; \
	then echo "Removing existing file $*.out"; rm $*.out; \
	fi
	UCCeBrA $*.mac 2>&1 | tee $*.log

# Pattern rule for sorting the simulation output into Root histograms
# and Radware ascii spectra
%.root : %.out
	./cs137_sim_sort.py $*.out 2>&1 | tee -a $*.log

clean:
	rm *.log *.out *.root *.wrl
