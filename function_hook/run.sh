#cat bin/function_hook | nc -c $ps4ip 5054
socat -u FILE:bin/function_hook TCP:192.168.2.2:5054
sleep 1
nc $ps4ip 5088
