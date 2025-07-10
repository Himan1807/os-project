#!/bin/bash

# Test OShell pipe latency (use full path to 'cat')
echo "=== Testing OShell Pipe Latency ==="
start_oshell=$(date +%s%N)
./oshell <<EOF
echo "test" | /bin/cat | /bin/cat | /bin/cat | /bin/cat | /bin/cat | /bin/cat | /bin/cat | /bin/cat | /bin/cat | /bin/cat > /dev/null
exit
EOF
end_oshell=$(date +%s%N)
oshell_latency=$(( (end_oshell - start_oshell) / 1000000 ))
echo "OShell latency: ${oshell_latency} ms"

# Test native terminal (Bash) latency
echo -e "\n=== Testing Native Terminal (Bash) Pipe Latency ==="
start_bash=$(date +%s%N)
bash -c 'echo "test" | cat | cat | cat | cat | cat | cat | cat | cat | cat | cat > /dev/null'
end_bash=$(date +%s%N)
bash_latency=$(( (end_bash - start_bash) / 1000000 ))
echo "Bash latency: ${bash_latency} ms"

# Results table
echo -e "\n=== Results ==="
printf "| %-15s | %-10s |\n" "Shell" "Latency (ms)"
printf "|%-17s|%-12s|\n" "-----------------" "------------"
printf "| %-15s | %-10s |\n" "OShell" "$oshell_latency"
printf "| %-15s | %-10s |\n" "Bash" "$bash_latency"