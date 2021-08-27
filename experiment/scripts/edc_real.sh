#! /bin/bash

phis=(
    '0.01'
    '0.005'
    '0.0025'
)

for file in graphs/real/*.graph ; do
    name=$(basename $file | sed "s/\..*//")

    for phi in "${phis[@]}" ; do
        result_ans=$(mktemp /tmp/edc.ans.XXXXXXXX)
        result_time=$(mktemp /tmp/edc.time.XXXXXXXX)
        { timeout 20m time -p edc -phi=$phi < $file > "$result_ans"; } 2> "$result_time"
        if [ $? -ne 0 ]; then
            echo "Skipping $name $phi" >&2
            rm -f "$result_ans"
            rm -f "$result_time"
            break
        fi

        vertices=$(head -n 1 $file | awk '{print $1}')

        edges=$(head -n 1 $file | awk '{print $2}')
        edgesCut=$(head -n 1 $result_ans | awk '{print $1}')
        edgeRatio=$(python3 -c "print($edgesCut/$edges)")

        partitions=$(head -n 1 $result_ans | awk '{print $2}')

        seconds=$(head -n 1 $result_time | awk '{print $2}')

        echo "$name,real,$phi,$vertices,$edges,$edgesCut,$edgeRatio,$partitions,$seconds"

        rm -f "$result_ans"
        rm -f "$result_time"
    done
done
