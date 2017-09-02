#cat bin/function_hook | nc $ps4ip 6054
socat -u FILE:bin/function_hook TCP:$ps4ip:6054
sleep 1
nc $ps4ip 5088
