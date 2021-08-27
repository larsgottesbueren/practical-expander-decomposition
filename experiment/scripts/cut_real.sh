#! /bin/bash

# Run the cut-matching game multiple times on the same graph and print the
# results to stdout.
#
# Usage: sample_graph
#          PATH_TO_GRAPH
#          PHI
#          STRATEGY_TYPE[true=balanced,false=default]
#          TARGET_BALANCE
function run_with_graph {
    graph_path="$1"
    graph_type="$2"
    graph_name="$3"
    phi="$4"
    strategy="$5"
    target_balance="$6"

    if [ "$strategy" = "balanced" ] ; then
        strategy_bool="true"
    else
        strategy_bool="false"
    fi

    for trial in `seq 0 3` ; do
        result_ans=$(mktemp /tmp/cut.ans.XXXXXXXX)

        timeout 5m edc-cut \
                -phi="$phi" \
                -balanced_cut_strategy="$strategy_bool" \
                -min_balance="$target_balance" \
                -record_cut_matching_time < "$graph_path" > "$result_ans"
        if [ $? -ne 0 ]; then
            echo "Timeout with file=$graph_path conductance=$phi strategy=$strategy balance=$target_balance" >&2
            rm -f "$result_ans"
            continue
        fi

        vertices=$(head -n 1 "$graph_path" | awk '{print $1}')
        edges=$(head -n 1 "$graph_path" | awk '{print $2}')

        result=$(head -n 1 $result_ans | awk '{print $1}')
        if [ "$result" != "balanced_cut" ]; then
            echo "Expected balanced cut, got $result with graph $graph_name" >&2
            rm -f "$result_ans"
            continue
        fi

        iterations=$(head -n 1 $result_ans | awk '{print $2}')
        volume_left=$(head -n 1 $result_ans | awk '{print $3}')
        volume_right=$(head -n 1 $result_ans | awk '{print $4}')
        edges_cut=$(head -n 1 $result_ans | awk '{print $5}')

        conductance=$(python3 -c "print($edges_cut/min($volume_left,$volume_right))")

        elapsed_time=$(sed '4q;d' $result_ans)

        balance=$(python3 -c "print(min($volume_left,$volume_right)/($volume_left+$volume_right))")

        echo "$graph_type,$graph_name,$vertices,$edges,$phi,$strategy,$target_balance,$iterations,$elapsed_time,$volume_left,$volume_right,$balance,$conductance"

        rm -f "$result_ans"
    done
}

phi='0.005'

target_balances=(
    '0.0'
    '0.25'
    '0.45'
)

for file in graphs/real/*.graph ; do
    name=$(basename $file | sed "s/\..*//")

    for target_balance in "${target_balances[@]}" ; do
        run_with_graph "$file" "real" "$name" "$phi" "balanced" "$target_balance"
        run_with_graph "$file" "real" "$name" "$phi" "default" "$target_balance"
    done
done
