#! /bin/bash

phis=(
    '0.01'
    '0.001'
)

params=(
    'margulis -n=10 -k=100 -r=100'
    'margulis -n=10 -k=100 -r=500'
    'margulis -n=10 -k=100 -r=1000'
    'margulis -n=10 -k=1000 -r=1000'
    'margulis -n=10 -k=1000 -r=5000'
    'margulis -n=10 -k=1000 -r=10000'
    'margulis -n=30 -k=100 -r=100'
    'margulis -n=30 -k=100 -r=500'
    'margulis -n=30 -k=100 -r=1000'
    'margulis -n=30 -k=1000 -r=1000'
    'margulis -n=30 -k=1000 -r=5000'
    'margulis -n=30 -k=1000 -r=10000'
)

for param in "${params[@]}" ; do
    file=$(mktemp /tmp/edc.bench.margulis.XXXXXXXX)
    ./gen_graph.py --seed=0 $param > $file

    for phi in "${phis[@]}" ; do
        result_ans=$(mktemp /tmp/edc.bench.ans.XXXXXXXX)
        result_time=$(mktemp /tmp/edc.bench.time.XXXXXXXX)
        { timeout 60m time -p edc -phi=$phi < $file > "$result_ans"; } 2> "$result_time"
        if [ $? -ne 0 ]; then
            echo "Skipping $param" >&2
            continue
        fi

        vertices=$(head -n 1 $file | awk '{print $1}')

        edges=$(head -n 1 $file | awk '{print $2}')
        edgesCut=$(head -n 1 $result_ans | awk '{print $1}')
        edgeRatio=$(python3 -c "print($edgesCut/$edges)")

        partitions=$(head -n 1 $result_ans | awk '{print $2}')

        seconds=$(head -n 1 $result_time | awk '{print $2}')

        echo "margulis,margulis,$phi,$vertices,$edges,$edgesCut,$edgeRatio,$partitions,$seconds"

        rm -f "$result_ans"
        rm -f "$result_time"
    done
done
