#! /bin/bash

phis=(
    '0.01'
    '0.001'
)

for file in graphs/walshaw/*.graph ; do
    name=$(basename $file | sed "s/\..*//")

    for phi in "${phis[@]}" ; do
        result_ans=$(mktemp /tmp/edc.bench.ans.XXXXXXXX)
        result_time=$(mktemp /tmp/edc.bench.time.XXXXXXXX)
        { timeout 60m time -p edc -chaco -phi=$phi < $file > "$result_ans"; } 2> "$result_time"
        if [ $? -ne 0 ]; then
            echo "Skipping $name" >&2
            continue
        fi

        vertices=$(head -n 1 $file | awk '{print $1}')

        edges=$(head -n 1 $file | awk '{print $2}')
        edgesCut=$(head -n 1 $result_ans | awk '{print $1}')
        edgeRatio=$(python3 -c "print($edgesCut/$edges)")

        partitions=$(head -n 1 $result_ans | awk '{print $2}')

        seconds=$(head -n 1 $result_time | awk '{print $2}')

        echo "$name,walshaw,$phi,$vertices,$edges,$edgesCut,$edgeRatio,$partitions,$seconds"

        rm -f "$result_ans"
        rm -f "$result_time"
    done
done
