 ./redis-benchmark -h 10.0.0.6 -p 6379 -c 1 -n 2000000 -t SET
 ./redis-benchmark -h 10.0.0.6 -p 6379 -c 2 -n 2000000 -t SET
 ./redis-benchmark -h 10.0.0.6 -p 6379 -c 4 -n 2000000 -t SET
 ./redis-benchmark -h 10.0.0.6 -p 6379 -c 8 -n 2000000 -t SET
 ./redis-benchmark -h 10.0.0.6 -p 6379 -c 16 -n 2000000 -t SET
 ./redis-benchmark -h 10.0.0.6 -p 6379 -c 32 -n 2000000 -t SET
 ./redis-benchmark -h 10.0.0.6 -p 6379 -c 64 -n 2000000 -t SET
