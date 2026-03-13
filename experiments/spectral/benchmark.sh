#!/bin/bash
RUNS=$1
rm -f log.txt
touch log.txt
current_percent=-1
total_runs=0
for i in $(seq 1 $RUNS);
do
    exit_code=42
    while [ $exit_code -eq 42 ]; do
        total_runs=$((total_runs + 1))
        out=$(./poc)
        exit_code=$?
        out=$(echo "$out" | tail -1)
        # if [ $exit_code -eq 42 ]; then
        #     echo reexec
        # fi
    done
    echo $out >> log.txt

    percent=$(( i * 100 / RUNS ))
    if [ $percent -gt $current_percent ]; then
        current_percent=$percent
        echo "$current_percent%" >&2
    fi
done

calc=$(awk '
{
    a[NR] = $1;       # Store each value in an array
    sum += $1;        # Sum of all values
    sumsq += ($1*$1); # Sum of squares of the values
}
END {
    n = NR;
    if (n == 0) {
        print "No data provided";
        exit;
    }

    # Manual bubble sort
    for (i = 1; i <= n; i++) {
        for (j = i+1; j <= n; j++) {
            if (a[i] > a[j]) {
                tmp = a[i];
                a[i] = a[j];
                a[j] = tmp;
            }
        }
    }

    # Median
    if (n % 2 == 1) {
        median = a[(n+1)/2];
    } else {
        median = (a[n/2] + a[n/2+1]) / 2;
    }

    mean = sum / n;
    if (n > 1)
        variance = (sumsq - n * (mean^2)) / (n - 1);
    else
        variance = 0;

    stddev = sqrt(variance);
    stderr = stddev / sqrt(n);

    printf "Median: %f\nMean: %f\nStandard Error: %f\n", median, mean, stderr;
}' log.txt)

echo "total runs: $total_runs" >> log.txt
echo "runs: $runs" >> log.txt
success_rate=$(( 100 * RUNS / total_runs ))
echo "sucess rate: $success_rate"
echo "sucess rate: $success_rate" >> log.txt
echo $calc
echo $calc >> log.txt
