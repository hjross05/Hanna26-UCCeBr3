all:  $(patsubst %.mac,%.root,$(wildcard *.mac))
	echo "Geant4 simulations done."

# Pattern rules for running Geant4 simulations.
%.out : %.mac
	if test -e $*.out; \
	then echo "Removing existing file $*.out"; rm $*.out; \
	fi
	UCCeBrA $*.mac 2>&1 | tee $*.log

# Pattern rule for sorting the simulation output into Root histograms
%.root : %.out
	./co60_sim_sort.py $*.out 2>&1 | tee -a $*.log

clean:
	rm -f *.log *.out *.root *.wrl
