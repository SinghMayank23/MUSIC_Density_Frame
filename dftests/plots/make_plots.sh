#!/bin/bash

folder1="DF_hydro"
label1="DF"
folder2="Default_MUSIC"
label2="Default MUSIC"
current_folder="$(pwd)"

echo "Start by preparing plotting data..."

g++ -o extract2D extract.cpp
cp extract2D ../$folder1/
cp extractlines_center.sh	../$folder1/
cp extract2D ../$folder2/
cp extractlines_center.sh	../$folder2/

rm extract2D

if [ ! -d "../$folder1" ] || [ ! -d "../$folder2" ]; then
	echo "One or both data folders not found. Please ensure ../$folder1 and ../$folder2 exist. Aborting."
	exit 1
fi

cd ../$folder1
	./extract2D
	./extractlines_center.sh
cd $current_folder

cd ../$folder2
	./extract2D
	./extractlines_center.sh
cd $current_folder

echo "Now creating plots..."

plots=("temperature" "Pi" "pixx" "pixy" "piyy")
template="plot_timeslice_1d.gp"

if [ ! -f "$template" ]; then
	echo "Template $template not found in $(pwd). Aborting."
	exit 1
fi

for plot in "${plots[@]}"; do
	mkdir -p "$plot"
	cp "$template" "$plot/plot_timeslice_1d.gp"


	# choose column and ylabel per-plot
    case "$plot" in
        "temperature")
            col=4
            ylabel="T (GeV)"
            ;;
        "Pi")
            col=8
            ylabel="{/Symbol P} (1/fm^{4})"
            ;;
        "pixx")
            col=12
            ylabel="{/Symbol p}^{xx}/({/Symbol e} + P)"
            ;;
        "pixy")
            col=13
            ylabel="{/Symbol p}^{xy}/({/Symbol e} + P)"
            ;;
        "piyy")
            col=14
            ylabel="{/Symbol p}^{yy}/({/Symbol e} + P)"
            ;;
        *)
            col=1
            ylabel="DOES NOT EXIST"
            ;;
    esac

		# Create the runner script inside each plot folder
		cat > "$plot/plot_${plot}.sh" <<EOF
#!/bin/bash
plot="$plot"
folder1="$folder1"
label1="$label1"
folder2="$folder2"
label2="$label2"
col="$col"
ylabel="$ylabel"

# change to script directory
cd "\$(dirname \"\${BASH_SOURCE[0]}\")" || true

# Generate EPS files
for ((n=1; n<=173; n+=10)); do
	tau=\$(echo "0.6 + 0.05 * \$n" | bc -l)
	sed \
		-e "s/set output \"plotlines1.eps\"/set output \"plotlines\${n}.eps\"/" \
		-e "s|../../after_LF_DF_conversion/1d_1.dat|../../${folder1}/1d_\${n}.dat|g" \
		-e "s|../../original_MUSIC/1d_1.dat|../../${folder2}/1d_\${n}.dat|g" \
		-e "s/title \"DF\"/title \"${label1}\"/" \
		-e "s/title \"Default MUSIC\"/title \"${label2}\"/" \
		-e "s/{\\/Symbol t} = [0-9.]\\+ fm/{\\/Symbol t} = \${tau} fm/" \
		-e "s/__COL__/\${col}/g" \
        -e "s|__YLABEL__|\${ylabel}|g" \
		plot_timeslice_1d.gp > plot_timeslice_1d.gp.tmp

	gnuplot plot_timeslice_1d.gp.tmp
	rm -f plot_timeslice_1d.gp.tmp

	echo "Created plotlines\${n}.eps (tau = \${tau} fm)"
done

# Convert EPS files to PDF (only if any EPS were created)
shopt -s nullglob
eps_files=(plotlines*.eps)
if [ \${#eps_files[@]} -gt 0 ]; then
	for eps_file in "\${eps_files[@]}"; do
		[ -e "\$eps_file" ] || continue
		epstopdf "\$eps_file"
	done
else
	echo "No EPS files generated; skipping conversion."
fi

# Merge PDF files in order to this plot folder
pdf_files=()
for ((n=1; n<=173; n+=10)); do
	pdf_files+=("plotlines\${n}.pdf")
done

if [ \${#pdf_files[@]} -gt 0 ]; then
	pdfunite "\${pdf_files[@]}" "./${plot}.pdf"
else
	echo "No PDF files to merge; skipping pdfunite."
fi

# Cleanup
rm -f plotlines*.eps plotlines*.pdf

echo "Done! Final output: ./${plot}.pdf"
EOF

	chmod +x "$plot/plot_${plot}.sh"
	echo "Created $plot/ with runner: $plot/plot_${plot}.sh"
done

echo "All plot folders and runners created. Run the runners inside each folder or from this directory: ./<folder>/plot_<folder>.sh"

echo "Doing so now..."

for plot in "${plots[@]}"; do
    echo "Running $plot/plot_${plot}.sh..."
    ( cd "$plot" && ./plot_${plot}.sh )
done

echo "Plotting complete! Now cleanup..."

cd ../$folder1
	rm 1d_*.dat
	rm plotdata*
cd $current_folder

cd ../$folder2
	rm 1d_*.dat
	rm plotdata*
cd $current_folder

echo "Cleanup complete!"