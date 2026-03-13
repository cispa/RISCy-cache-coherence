#!/bin/bash

RUNS="$1"
CORE="$2"

if [ -z "$CORE" ]; then
    echo error: no core
    exit 1
fi

for method in {archsc,fr}; do
    CORRECT=0
    TIME=0
    # warmup
    echo "Warming up..."
    OUT=$(./aes_t_table_$method $CORE)
    rm -f runtime_$method.log
    touch runtime_$method.log

    for i in $(seq 1 $RUNS);
    do
        echo "> Run $i / $RUNS (./aes_t_table_$method $CORE)"
        exit_code=42
        while [ $exit_code -eq 42 ]; do
            OUT=$(./aes_t_table_$method $CORE)
            exit_code=$?
        done
        let is_correct=$(echo "$OUT" | grep -c CORRECT)
        let CORRECT=$CORRECT+$is_correct
        RTIME=$(echo "$OUT" | grep "Time: " | awk '{print $2}')
        let TIME=$TIME+$RTIME
        echo $RTIME >> runtime_$method.log

        echo "$CORRECT/$i $(( TIME / i )) $is_correct"

        if [ $is_correct -eq 1 ]; then
            cp hist.csv $method.csv
        fi
    done

    # compute stderr of runtime
    stderr=$(awk '{
        n++;
        sum += $1;
        sumsq += $1*$1
    }
    END {
        mean = sum/n;
        var = (sumsq/n - mean*mean);
        sd = (var>0 ? sqrt(var) : 0);
        stderr = (n>0 ? sd/sqrt(n) : 0);
        printf "stderr: %.6f\n", stderr
    }' runtime_$method.log)

    echo "------------------------"
    echo $CORRECT/$RUNS
    echo $(( TIME / RUNS ))
    echo "$stderr"
    echo "------------------------" >> runtime_$method.log
    echo $CORRECT/$RUNS >> runtime_$method.log
    echo $(( TIME / RUNS )) >> runtime_$method.log
    echo $stderr >> runtime_$method.log
done
