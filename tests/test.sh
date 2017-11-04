#!/bin/bash

echo "Tests start"
echo ""
echo "Test 1 : small file transfert"

exec "./receiver" -f tests/output_small.dat ::1 12345 2> tests/receiver_small.log &
exec "./sender" -f tests/file_small.dat ::1 12345 2> tests/sender_small.log &
sleep 2

cmp --silent tests/output_small.dat tests/file_small.dat && echo 'Files are the same' || echo 'Files are different'

echo "Test 2 : medium file transfert"

exec "./receiver" -f tests/output_medium.dat ::1 12345 2> tests/receiver_medium.log &
exec "./sender" -f tests/file_medium.dat ::1 12345 2> tests/sender_medium.log &
sleep 2

cmp --silent tests/output_medium.dat tests/file_medium.dat && echo 'Files are the same' || echo 'Files are different'

echo "Test 3 : large file transfert"

exec "./receiver" -f tests/output_large.dat ::1 12345 2> tests/receiver_large.log &
exec "./sender" -f tests/file_large.dat ::1 12345 2> tests/sender_large.log &
sleep 2

cmp --silent tests/output_large.dat tests/file_large.dat && echo 'Files are the same' || echo 'Files are different'
